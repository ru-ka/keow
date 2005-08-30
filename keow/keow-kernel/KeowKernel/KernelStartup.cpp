/*
 * Copyright 2005 Paul Walker
 *
 * GNU General Public License
 * 
 * This file is part of: Kernel Emulation on Windows (keow)
 *
 * Keow is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Keow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Keow; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// KernelStartup.cpp: implementation of the KernelStartup class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "KernelStartup.h"


KernelStartup::HandlerRec KernelStartup::s_ArgumentHandlers[] = {
	{"root", KernelStartup::arg_root},
	{"init", KernelStartup::arg_init},
	{"debug", KernelStartup::arg_debug},
	{"automount", KernelStartup::arg_automount},
	{NULL, NULL}
};

string KernelStartup::s_InitProgram("/sbin/init");
const char * KernelStartup::ms_pszAutoMount = NULL;


//////////////////////////////////////////////////////////////////////

void KernelStartup::ProcessCommandLine(LPSTR lpCmdLine)
{
	ktrace("Processing arguments\n");

	//some yucky code to process the command line

	list<string> args;

	char * pLineEnd = lpCmdLine + strlen(lpCmdLine);
	char * pArg = lpCmdLine;
	while(pArg<pLineEnd)
	{
		bool inQuote;
		string a;
		char *p;

		//start of the name
		while(isspace(*pArg))
			++pArg;

		//find end of argument, possibly double quoted
		p = pArg;
		inQuote = false;
		while((inQuote || !isspace(*p)) && p<pLineEnd)
		{
			//something is quoted?
			if(*p=='"')
			{
				inQuote = !inQuote;
			}
			else
			{
				a += *p;
			}

			++p;
		}

		//store
		args.push_back(a);

		//next arg
		pArg = p+1;
	}

	//process the found args
	list<string>::iterator it;
	for(it=args.begin(); it!=args.end(); ++it)
	{
		string a = *it;
		string name,val;

		ktrace("processing: %s\n", a.c_str());

		//split name=value?
		int eq = a.find('=');
		if(eq<0)
		{
			name = a;
			val  = "";
		}
		else
		{
			name = a.substr(0,eq);
			val  = a.substr(eq+1);
		}
		

		//find handler for arg
		for(int h=0; s_ArgumentHandlers[h].arg_name!=NULL; ++h)
		{
			if(name == s_ArgumentHandlers[h].arg_name)
			{
				(*s_ArgumentHandlers[h].handler)(val.c_str());
			}
		}
		if(s_ArgumentHandlers[h].arg_name!=NULL)
			ktrace("unknown argument ignored");

	}
	ktrace("Finished argument processing\n");
}


void KernelStartup::ValidateKernelTraps()
{
	//Test whether INT 80h is illegal in Windows
	//and will trigger our error handler
	bool trapped = false;
	__try {
		__asm int 80h
	}
	__except( EXCEPTION_EXECUTE_HANDLER ) {
		trapped = true;
	}
	if(!trapped)
	{
		ktrace("Cannot trap kernel calls.\n");
		halt();
	}

	//Check if call gates 7h and 27h are illegal
	//and will trigger our error handler
	//TODO:

	//We currently depend on ASCII build, not UNICODE
	if(sizeof(TCHAR) != sizeof(char))
	{
		ktrace("UNICODE build not supported\n");
		halt();
	}

	//Check pages sizes are as we assume (code depends on these)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if(si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL
	|| si.dwPageSize != SIZE4k
	|| si.dwAllocationGranularity  != SIZE64k)
	{
		ktrace("Architecture assumptions not met/not supported\n");
		halt();
	}
}


//AutoMount windows drive letters
//make mounts and generate initial /etc/mtab
void KernelStartup::AutoMountDrives()
{
	if(ms_pszAutoMount==0)
		ms_pszAutoMount = "CDEFGHIJKLMNOPRSTUVWXYZ";

	Path p;

	//files may no exist, if so don't create them
	FILE * fMtab = NULL;
	p.SetUnixPath("/etc/mtab");
	if(GetFileAttributes(p.GetWin32Path())!=INVALID_FILE_ATTRIBUTES)
		fMtab = fopen(p.GetWin32Path(), "wb");

	char drive[] = "X:\\";
	char mnt[] = "/mnt/X";
	char *pMntLetter = &mnt[5]; //the 'X'

	const char *letter = ms_pszAutoMount;
	while(*letter)
	{
		drive[0] = *pMntLetter = toupper(*letter);

		//is the drive valid to mount?
		if(GetFileAttributes(drive)!=INVALID_FILE_ATTRIBUTES)
		{
			//try to make the mount directory /mnt/X
			p.SetUnixPath(mnt);
			CreateDirectory(p.GetWin32Path(), NULL);
			if(GetFileAttributes(p.GetWin32Path())&FILE_ATTRIBUTE_DIRECTORY)
			{
				ktrace("automatic drive mount: %s on /mnt/%c\n", drive, *pMntLetter);

				//record the mount
				MountPoint * pMp = MountPoint::Mount(p, drive, new FilesystemKeow(), NULL, 0);

				//update /etc/mtab (because we're not using unix 'mount' which normally does it)
				if(fMtab!=NULL)
				{
					//eg: c:/ /mnt/c keow rw 0 0
					fprintf(fMtab, "%c:\\  /mnt/%c  keow rw 0 0 \x0a", *pMntLetter, *pMntLetter);
				}
			}
		}

		++letter;
	}

	//also want /proc mounted
	p.SetUnixPath("/proc");
	CreateDirectory(p.GetWin32Path(), NULL);
	if(GetFileAttributes(p.GetWin32Path())&FILE_ATTRIBUTE_DIRECTORY)
	{
		//record the mount
		MountPoint * pMp = MountPoint::Mount(p, "", new FilesystemProc(), NULL, 0);

		//update /etc/mtab
		if(fMtab)
			fprintf(fMtab, "/proc /proc proc rw 0 0 \x0a");
	}


	//close mtab
	if(fMtab)
		fclose(fMtab);
}

////////////////////////////////////////////////////////////////////////////////

void KernelStartup::arg_root(const char *arg)
{
	//ensure it exists
	//also ensure that the kernel starts from this directory
	if(!SetCurrentDirectory(arg))
	{
		perror("root dir does not exist");
		halt();
	}

	char root[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, root);
	g_pKernelTable->m_FilesystemRoot = root;
	Path pRoot("/", false);
	g_pKernelTable->m_pRootMountPoint = MountPoint::Mount(pRoot, g_pKernelTable->m_FilesystemRoot, new FilesystemKeow(), 0,0);

	ktrace("Using %s as filesystem root\n", g_pKernelTable->m_FilesystemRoot.c_str());
}

void KernelStartup::arg_init(const char *arg)
{
	s_InitProgram = arg;
	ktrace("Using %s as 'init'\n", arg);
}

void KernelStartup::arg_debug(const char *arg)
{
	g_pKernelTable->m_DebugLevel = atoi(arg);
	ktrace("Kernel Debug Level: %d\n", g_pKernelTable->m_DebugLevel);
}

void KernelStartup::arg_automount(const char *arg)
{
	ms_pszAutoMount = arg;
	ktrace("AutoMount: %s\n", ms_pszAutoMount);
}

