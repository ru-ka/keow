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

#include <windows.h>
#include <stdio.h>

#include "syscalls\kernel.h"
#include "syscalls\loadelf.h"


typedef void (*ARG_HANDLER)(const char *);

const char * InitProgram = "/sbin/init";
char * AutoMount = NULL;

char KernelInstance[MAX_PATH];
KernelSharedDataStruct * pKernelSharedData;

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
		StringCbPrintf(p,sizeof(p),"%s\\..\\pid%05d.trace", pKernelSharedData->LinuxFileSystemRoot, i);
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

	//We currentl depend on ASCII build, not UNICODE
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
				MountPointDataStruct& mp = pKernelSharedData->MountPoints[pKernelSharedData->NumCurrentMounts];
				StringCbPrintf(mp.Destination, MAX_PATH, "/mnt/%c", *pMntLetter);
				mp.DestinatinLen = strlen(mp.Destination); 
				strncpy(mp.Source, drive, MAX_PATH);
				mp.SourceLen = strlen(mp.Source); 
				mp.Flags = 0;
				mp.nFsHandler = KEOW_FS_INDEX;
				mp.Data[0] = 0;
				pKernelSharedData->NumCurrentMounts++;

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
		MountPointDataStruct& mp = pKernelSharedData->MountPoints[pKernelSharedData->NumCurrentMounts];
		strncpy(mp.Destination, "/proc", MAX_PATH);
		mp.DestinatinLen = strlen(mp.Destination); 
		strncpy(mp.Source, "/proc", MAX_PATH);
		mp.SourceLen = strlen(mp.Source); 
		mp.Flags = 0;
		mp.nFsHandler = PROC_FS_INDEX;
		mp.Data[0] = 0;
		pKernelSharedData->NumCurrentMounts++;

		//update /etc/mtab
		if(fMtab)
			fprintf(fMtab, "/proc /proc proc rw 0 0 \x0a");
	}

	if(fMtab)
		fclose(fMtab);
}


void InitKernelData()
{
	StringCbPrintf(KernelInstance, sizeof(KernelInstance), "KernelEmulationOnWindows-KernelDataStruct-(%ld)-%lx", GetCurrentProcessId(), GetTickCount());

	//shared memory
	HANDLE hKernelData = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(KernelSharedDataStruct), KernelInstance);

	pKernelSharedData = (KernelSharedDataStruct*)MapViewOfFile(hKernelData, FILE_MAP_ALL_ACCESS, 0,0,0);
	if(!pKernelSharedData)
	{
		printf("Failed to allocate shared memory\n");
		halt();
	}
	printf("Kernel shared memory size is %ld bytes\n", sizeof(KernelSharedDataStruct));

	//data was init as zero, so don't need to set much

	StringCbPrintf(pKernelSharedData->KernelInstanceName, sizeof(pKernelSharedData->KernelInstanceName), KernelInstance);

	//process stub is same dir as this exe
	GetModuleFileName(NULL, pKernelSharedData->ProcessStubFilename, MAX_PATH);
	//replace ending
	char *s = strrchr(pKernelSharedData->ProcessStubFilename, '\\');
	StringCbCopy(s, MAX_PATH-strlen(pKernelSharedData->ProcessStubFilename), "\\ProcessStub.exe");

	pKernelSharedData->LastAllocatedPID = 1; //we'll allocate that

	GetSystemTime(&pKernelSharedData->BootTime);

	//simple - probably doesn't match the kernel
	pKernelSharedData->BogoMips = 0;
	const DWORD msSample = 1000;
	DWORD dwEnd = GetTickCount() + msSample;
	while(GetTickCount() < dwEnd) {
		Sleep(0);
		pKernelSharedData->BogoMips++;
	}
	pKernelSharedData->BogoMips /= msSample;
	printf("keow bogomips: %ld\n", pKernelSharedData->BogoMips);

	//A dummy stub to let's the SysCalls dll init before we start using it
	Process_Init("INIT", 0, KernelInstance);
}


HANDLE StartELFFile(const char * filename, const char * commandline, const char *startdir, const char ** env)
{
	PELF_Data pElf;
	const int InitPID = 1; //init is always PID 1

	ProcessDataStruct *p = &pKernelSharedData->ProcessTable[InitPID];


	pElf = (PELF_Data)calloc(sizeof(struct ELF_Data), 1);
	if(pElf==NULL)
	{
		perror("cannot allocate ELF_Data");
		return INVALID_HANDLE_VALUE;
	}

	//New a new process to start the code in
	//use RUN keyword in argv[0] - this is the first ever program (not using fork/exec)
	char CommandLine[100];
	StringCbPrintf(CommandLine, sizeof(CommandLine), "RUN %d %s", InitPID, KernelInstance);

	GetStartupInfo(&pElf->sinfo);
	if(!CreateProcess(pKernelSharedData->ProcessStubFilename, CommandLine, NULL, NULL, FALSE, CREATE_SUSPENDED|CREATE_NEW_PROCESS_GROUP, NULL, NULL, &pElf->sinfo, &pElf->pinfo))
	{
		DWORD err = GetLastError();
		fprintf(stderr,"error %ld starting %s\n", err, filename);
		free(pElf);
		return INVALID_HANDLE_VALUE;
	}

	if(LoadELFFile(pElf, InitProgram, false))
	{
		p->is_aout_format = false;
	}
	else
	{
		TerminateProcess(pElf->pinfo.hProcess,-1);
		CloseHandle(pElf->pinfo.hThread);
		CloseHandle(pElf->pinfo.hProcess);
		free(pElf);
		return INVALID_HANDLE_VALUE;
	}

	//initial process data
	p->in_setup = true;

	StringCbCopy(p->unix_pwd, sizeof(p->unix_pwd), startdir);
	StringCbCopy(p->ProgramPath, sizeof(p->ProgramPath), filename);

	p->program_base = pElf->program_base;
	p->program_max = pElf->program_max;
	p->interpreter_base = pElf->interpreter_base;
	p->interpreter_max = pElf->last_lib_addr;
	p->interpreter_entry = pElf->interpreter_start;
	p->phdr = pElf->phdr_addr;
	p->phent = pElf->hdr.e_phentsize;
	p->phnum = pElf->hdr.e_phnum;
	p->program_entry = pElf->start_addr;
	p->uid = p->gid = 0; //root
	p->euid = p->egid = 0; //root
	p->sav_uid = p->sav_gid = 0; //root
	p->brk = p->brk_base = pElf->brk;
	p->PID = 1;
	p->ParentPID = 0;
	p->ProcessGroupPID = 1;

	//register process in kernel data
	p->in_use = true;
	p->Win32PID = pElf->pinfo.dwProcessId;
	p->MainThreadID = pElf->pinfo.dwThreadId;
	p->SignalThreadID = -1; //not yet set

	//an initial environment
	//got this by: cat -x /proc/1/environ on linux
	char * initial_argv[] = {0,0};
	initial_argv[0] = p->ProgramPath;
	p->argv = CopyStringListToProcess(pElf->pinfo.hProcess, initial_argv);;
	p->envp = CopyStringListToProcess(pElf->pinfo.hProcess, InitialEnv);


	//start the process running
	ResumeThread(pElf->pinfo.hThread);


	//cleanup
	HANDLE hRet = pElf->pinfo.hProcess;
	CloseHandle(pElf->pinfo.hThread);

	free(pElf);

	return hRet;
}


BOOL WINAPI CtrlEventHandler(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		//ignore
		break;
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		//tell 'init' to shut down
		//in leu of the proper way use SIGTERM (need to research this)
		SendSignal(1,SIGTERM);
		break;
	}
	return TRUE; //always say we handled it
}



void arg_root(const char *arg)
{
	//ensure it exists
	if(!SetCurrentDirectory(arg))
	{
		perror("root dir does not exist");
		halt();
	}

	pKernelSharedData->LinuxFileSystemRootLen = 
		GetCurrentDirectory(sizeof(pKernelSharedData->LinuxFileSystemRoot)-1, pKernelSharedData->LinuxFileSystemRoot);

	printf("Using %s as filesystem root\n", pKernelSharedData->LinuxFileSystemRoot);
}

void arg_init(const char *arg)
{
	InitProgram = arg;
	printf("Using %s as 'init'\n", InitProgram);
}

void arg_debug(const char *arg)
{
	pKernelSharedData->KernelDebug = atoi(arg);
	printf("Kernel Debug: %d\n", pKernelSharedData->KernelDebug);
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




int main(int argc, char ** argv)
{
	int i;

	SetLastError(0);
	SetConsoleTitle("Console - Kernel Emulation on Windows");

	//80 columns, lots of history
	COORD c;
	c.X = 80;
	c.Y = 1000;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), c);
	//start output at the bottom of the buffer
	c.X = 0;
	c.Y--;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);

	SMALL_RECT sr;
	sr.Left = 0;
	sr.Top = 0;
	sr.Right = 80-1;
	sr.Bottom = 25-1;
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), true, &sr);

	//try for this more unix-like font (normal NT ones are too bold)
	//TODO: use Lucidia Console 12

	//SetConsoleCtrlHandler(CtrlEventHandler, TRUE);
	//SetConsoleCtrlHandler(NULL, TRUE);
	SetConsoleCtrlHandler(NULL, TRUE);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_OUTPUT);


	ValidateKernelTraps();

	printf("Kernel Booting\n");

	printf("Setting up kernel data structure\n");
	InitKernelData();

	printf("Processing arguments\n");
	//defaults
	for(i=1; i<argc; ++i)
	{
		char *n, *v;
		int h;

		printf("processing: %s\n", argv[i]);

		//try to get a name=value split. else value NULL
		n=argv[i];
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
			printf("unknown argument ignored");
	}
	printf("Finished argument processing\n");

	if(pKernelSharedData->LinuxFileSystemRootLen == 0)
	{
		printf("No root= specified\n");
		halt();
	}

	CleanUpTraceFiles();

	//AutoMount windows drive letters
	AutoMountDrives();
	
	
	//Load the initial executable
	printf("Loading 'init': %s\n", InitProgram);
	HANDLE hInit = StartELFFile(InitProgram, "", "/", NULL);

	if(hInit==INVALID_HANDLE_VALUE)
	{
		printf("Failed to load 'init'\n");
		halt();
	}


	printf("'init' started\n");

	//wait for it to exit
	MsgWaitForMultipleObjects(1, &hInit, FALSE, INFINITE, NULL);
	printf("'init' appears to have terminated, ");

	DWORD exitcode; 
	if(!GetExitCodeProcess(hInit, &exitcode))
		printf("exit code undetermined\n");
	else
		printf("exit code 0x%lx (%ld)\n", exitcode,exitcode);


	//allow exit
	printf("Halting kernel, press Ctrl-C to close\n");
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);
	SetConsoleCtrlHandler(NULL, FALSE);

	halt();
	return -1; //never get here
}
