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

// KeowKernel.cpp : Defines the entry point for the application.
//

#include "kernel.h"
#include "forkexec.h"

//the single 'console' terminal
Terminal g_Console;

//the kernel
KernelDataStruct g_KernelData;


//sometimes need an inheritable security attr
SECURITY_ATTRIBUTES g_InheritableSA;


typedef void (*ARG_HANDLER)(const char *);

const char * InitProgram = "/sbin/init";
char * AutoMount = NULL;


char * InitialEnv[] = {
	"OLDPWD=/",
	"HOME=/",
	"TERM=ansi80x25", //freebsd console (25-line ansi mode) -- easier to implement than the linux console -- implemented in ioh_console.cpp
	"PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin",
	"PWD=/",
	NULL
};




void halt()
{
	MSG msg;

	printf("Kernel halted.");

	while(GetMessage(&msg,0,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


void CleanUpTraceFiles()
{
	//clean up tracing files from previous runs
	char p[MAX_PATH];

	for(int i=0; i<MAX_PROCESSES; ++i)
	{
		StringCbPrintf(p,sizeof(p),"%s\\..\\pid%05d.trace", g_KernelData.LinuxFileSystemRoot, i);
		DeleteFile(p);
	}
}


void ValidateKernelTraps()
{
	//Test whether INT 80h is illegal in Windows
	//and will trigger our error handler

#pragma pack(push,1)
	struct {
		WORD limit;
		DWORD base;
	} idt_data;
#pragma pack(pop)

	__asm {
		sidt idt_data
	};

	//TODO: how do we read the IDT itself? Is itn't it physical memory?
	printf("todo: validate kernel traps\n");

	//if(fails)
	//	halt();

	//Check if call gates 7h and 27h are illegal
	//and will trigger our error handler

	//expect to be able to trap INT 80h kernel calls
	bool trapped = false;
	__try {
		__asm int 80h
	}
	__except( EXCEPTION_EXECUTE_HANDLER ) {
		trapped = true;
	}
	if(!trapped)
	{
		printf("Cannot trap kernel calls.\n");
		halt();
	}

	//We currently depend on ASCII build, not UNICODE
	if(sizeof(TCHAR) != sizeof(char))
	{
		printf("UNICODE build not supported\n");
		halt();
	}

	//Check pages sizes are as we assume (code depends on these)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if(si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL
	|| si.dwPageSize != SIZE4k
	|| si.dwAllocationGranularity  != SIZE64k)
	{
		printf("Architecture assumptions not met/not supported\n");
		halt();
	}
}


//AutoMount windows drive letters
//make mounts and generate initial /etc/mtab
void AutoMountDrives()
{
	if(AutoMount==0)
		AutoMount = strdup("CDEFGHIJKLMNOPRSTUVWXYZ");

	//expect to be in root dir
	//files may no exist, if so don't create them
	FILE * fMtab = NULL;
	const char * szMtab = "./etc/mtab";
	if(GetFileAttributes(szMtab)!=INVALID_FILE_ATTRIBUTES)
		fMtab = fopen(szMtab, "wb");

	char drive[] = "X:\\";
	char mnt[] = "./mnt/X";
	char *pMntLetter = &mnt[6]; //'X'

	char *letter = AutoMount;
	while(*letter)
	{
		drive[0] = *pMntLetter = toupper(*letter);

		if(GetFileAttributes(drive)!=INVALID_FILE_ATTRIBUTES)
		{
			CreateDirectory(mnt, NULL);
			if(GetFileAttributes(mnt)&FILE_ATTRIBUTE_DIRECTORY)
			{
				printf("automatic drive mount: %s on /mnt/%c\n", drive, *pMntLetter);

				//record the mount
				MountPointDataStruct& mp = g_KernelData.MountPoints[g_KernelData.NumCurrentMounts];
				StringCbPrintf(mp.Destination, MAX_PATH, "/mnt/%c", *pMntLetter);
				mp.DestinatinLen = strlen(mp.Destination); 
				strncpy(mp.Source, drive, MAX_PATH);
				mp.SourceLen = strlen(mp.Source); 
				mp.Flags = 0;
				mp.nFsHandler = KEOW_FS_INDEX;
				mp.Data[0] = 0;
				g_KernelData.NumCurrentMounts++;

				//update /etc/mtab
				if(fMtab)
				{
					//eg: c:/ /mnt/c keow rw 0 0
					fprintf(fMtab, "%c:\\  /mnt/%c  keow rw 0 0 \x0a", *pMntLetter, *pMntLetter);
				}
			}
		}

		++letter;
	}

	//also want /proc mounted
	CreateDirectory("proc", NULL);
	if(GetFileAttributes("proc")&FILE_ATTRIBUTE_DIRECTORY)
	{
		//record the mount
		MountPointDataStruct& mp = g_KernelData.MountPoints[g_KernelData.NumCurrentMounts];
		strncpy(mp.Destination, "/proc", MAX_PATH);
		mp.DestinatinLen = strlen(mp.Destination); 
		strncpy(mp.Source, "/proc", MAX_PATH);
		mp.SourceLen = strlen(mp.Source); 
		mp.Flags = 0;
		mp.nFsHandler = PROC_FS_INDEX;
		mp.Data[0] = 0;
		g_KernelData.NumCurrentMounts++;

		//update /etc/mtab
		if(fMtab)
			fprintf(fMtab, "/proc /proc proc rw 0 0 \x0a");
	}

	if(fMtab)
		fclose(fMtab);
}



void InitKernelData()
{
	printf("Kernel data size is %ld bytes\n", sizeof(KernelDataStruct));

	//init data as zero, so don't need to set much
	memset(&g_KernelData, 0, sizeof(g_KernelData));

	//process stub is same dir as this exe
	GetModuleFileName(NULL, g_KernelData.ProcessStubFilename, MAX_PATH);
	//replace ending
	char *s = strrchr(g_KernelData.ProcessStubFilename, '\\');
	StringCbCopy(s, MAX_PATH-strlen(g_KernelData.ProcessStubFilename), "\\ProcessStub.exe");

	g_KernelData.LastAllocatedPID = 1; //we'll allocate that for 'init'

	GetSystemTime(&g_KernelData.BootTime);

	//simple - probably doesn't match the kernel
	g_KernelData.BogoMips = 0;
	const DWORD msSample = 1000;
	DWORD dwEnd = GetTickCount() + msSample;
	while(GetTickCount() < dwEnd) {
		Sleep(0);
		g_KernelData.BogoMips++;
	}
	g_KernelData.BogoMips /= msSample;
	printf("keow bogomips: %ld\n", g_KernelData.BogoMips);

}



void arg_root(const char *arg)
{
	//ensure it exists
	if(!SetCurrentDirectory(arg))
	{
		perror("root dir does not exist");
		halt();
	}

	g_KernelData.LinuxFileSystemRootLen = 
		GetCurrentDirectory(sizeof(g_KernelData.LinuxFileSystemRoot)-1, g_KernelData.LinuxFileSystemRoot);

	printf("Using %s as filesystem root\n", g_KernelData.LinuxFileSystemRoot);
}

void arg_init(const char *arg)
{
	InitProgram = arg;
	printf("Using %s as 'init'\n", InitProgram);
}

void arg_debug(const char *arg)
{
	g_KernelData.KernelDebug = atoi(arg);
	printf("Kernel Debug: %d\n", g_KernelData.KernelDebug);
}

void arg_automount(const char *arg)
{
	AutoMount = strdup(arg);
	printf("AutoMount: %s\n", AutoMount);
}


struct {
	const char * arg_name;
	ARG_HANDLER handler;
} argument_handlers[] = {
	{"root", arg_root},
	{"init", arg_init},
	{"debug", arg_debug},
	{"automount", arg_automount},
	{NULL, NULL}
};



// Start a console handler process
// to manage the unix system console
//
bool CreateConsole()
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	memset(&si,0,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	if(!CreatePipe(&si.hStdInput, &g_Console.hTerminalWrite, &g_InheritableSA, 0))
		return false;
	if(!CreatePipe(&g_Console.hTerminalRead, &si.hStdOutput, &g_InheritableSA, 0))
		return false;


	if(!CreateProcess("KeowConsole.exe", "Console (tty0) - Kernel Emulation on Windows", NULL,NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
		return false;


	g_Console.dwTerminalProcess = pi.dwProcessId;

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return true;

}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	g_InheritableSA.nLength = sizeof(g_InheritableSA);
	g_InheritableSA.bInheritHandle = TRUE;
	g_InheritableSA.lpSecurityDescriptor = NULL;

	//Need a console window handler process
	if(!CreateConsole()) {
		return -1;
	}



	ValidateKernelTraps();

	g_Console.printf("Kernel Booting\n");

	g_Console.printf("Setting up kernel data structure\n");
	InitKernelData();

	g_Console.printf("Processing arguments\n");
	char * arg = strtok(lpCmdLine," \t");
	for(arg=strtok(lpCmdLine," \t"); arg; arg=strtok(NULL," \t"))
	{
		char *n, *v;
		int h;

		g_Console.printf("processing: %s\n", arg);

		//try to get a name=value split. else value NULL
		n=arg;
		v=strchr(n,'=');
		if(v!=NULL)
		{
			*v = 0;
			v++;
		}

		//find handler for arg
		for(h=0; argument_handlers[h].arg_name!=NULL; ++h)
		{
			if(strcmp(argument_handlers[h].arg_name, n) == 0)
			{
				(*argument_handlers[h].handler)(v);
			}
		}
		if(argument_handlers[h].arg_name!=NULL)
			g_Console.printf("unknown argument ignored");
	}
	g_Console.printf("Finished argument processing\n");

	if(g_KernelData.LinuxFileSystemRootLen == 0)
	{
		g_Console.printf("No root= specified\n");
		halt();
	}

	CleanUpTraceFiles();

	//AutoMount windows drive letters
	AutoMountDrives();
	
	
	//Load the initial executable
	g_Console.printf("Loading 'init': %s\n", InitProgram);
	HANDLE hInit = StartNewProcess(1, InitProgram, "", "/", NULL);

	if(hInit==INVALID_HANDLE_VALUE)
	{
		g_Console.printf("Failed to load 'init'\n");
		halt();
	}


	g_Console.printf("'init' started\n");

	//wait for it to exit
	MsgWaitForMultipleObjects(1, &hInit, FALSE, INFINITE, NULL);
	g_Console.printf("'init' appears to have terminated, ");

	DWORD exitcode; 
	if(!GetExitCodeProcess(hInit, &exitcode))
		g_Console.printf("exit code undetermined\n");
	else
		g_Console.printf("exit code 0x%lx (%ld)\n", exitcode,exitcode);


	//allow exit
	g_Console.printf("Halting kernel, press Ctrl-C to close\n");
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);
	SetConsoleCtrlHandler(NULL, FALSE);

	halt();
	return -1; //never get here
}

