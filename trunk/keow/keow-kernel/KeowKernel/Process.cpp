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

// Process.cpp: implementation of the Process class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "Process.h"
#include "IohNtConsole.h"
#include "SysCalls.h"



//handler shouldn't need the default 1MB stack, try a smaller one
const int Process::KERNEL_PROCESS_THREAD_STACK_SIZE = 32*1024;

const char * Process::KEOW_PROCESS_STUB = "Keow.exe";


//////////////////////////////////////////////////////////////////////

Process::Process()
{
	m_gid = 0;
	m_uid = 0;
	m_egid = 0;
	m_euid = 0;

	m_saved_uid = 0;
	m_saved_gid = 0;

	m_Pid = 0;
	m_ParentPid = 0;
	m_Environment = NULL;
	m_Arguments = NULL;
	m_ArgCnt = 0;
	m_EnvCnt = 0;

	m_ProcessGroupPID = 0;

	memset(&m_Win32PInfo, 0, sizeof(m_Win32PInfo));
	m_KernelThreadId = 0;
	m_KernelThreadHandle = NULL;

	memset(&m_ElfLoadData, 0, sizeof(m_ElfLoadData));
	memset(&m_ptrace, 0, sizeof(m_ptrace));

	memset(&m_OpenFiles, 0, sizeof(m_OpenFiles));

	InitSignalHandling();

	m_bStillRunning = true;
	m_dwExitCode = -SIGABRT; //in case not set later
	m_bCoreDumped = false;

	m_hWaitTerminatingEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hProcessStartEvent = m_hForkDoneEvent = NULL;

	memset(&SysCallAddr, 0, sizeof(SysCallAddr));
}

Process::~Process()
{
	//remove from the kernel process map
	g_pKernelTable->m_Processes.erase(this);

	//close all handles
	CloseHandle(m_Win32PInfo.hProcess);
	CloseHandle(m_Win32PInfo.hThread);

	CloseHandle(m_KernelThreadHandle);

	CloseHandle(m_hWaitTerminatingEvent);
}



//Create a new process using the given path to a program
//The pid is as assigned by the kernel
//The initial environment is as supplied
//
Process* Process::StartInit(PID pid, Path& path, char ** InitialArguments, char ** InitialEnvironment)
{
	ktrace("StartInit()\n");

	/*
	launch win32 process stub
	load elf code into the process
	set stack/cpu contexts
	let process run
	*/
	Process * NewP = new Process();
	NewP->m_Pid = pid;
	NewP->m_ParentPid = 0;
	NewP->m_ProcessFileImage = path;
	NewP->m_UnixPwd = Path("/");

	//add to the kernel
	g_pKernelTable->m_Processes.push_back(NewP);


	//env and args
	//StartNewImageRunning expects to be able to free these - so copy them - in the kernel still
	NewP->m_Arguments   = MemoryHelper::CopyStringListBetweenProcesses(GetCurrentProcess(), (ADDR)InitialArguments, GetCurrentProcess(), NULL, &NewP->m_ArgCnt, NULL);
	NewP->m_Environment = MemoryHelper::CopyStringListBetweenProcesses(GetCurrentProcess(), (ADDR)InitialEnvironment, GetCurrentProcess(), NULL, &NewP->m_EnvCnt, NULL);

	//events to co-ordinate starting
	NewP->m_hProcessStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	NewP->m_hForkDoneEvent = NULL; //not forking

	//need to start the process and debug it (to trap kernel calls etc)
	//the debugger thread
	NewP->m_KernelThreadHandle = CreateThread(NULL, KERNEL_PROCESS_THREAD_STACK_SIZE, KernelProcessHandlerMain, NewP, 0, &NewP->m_KernelThreadId);
	if(NewP->m_KernelThreadHandle==NULL)
	{
		ktrace("failed to start debug thread for process\n");
		delete NewP;
		return NULL;
	}

	//Wait for the thread to start the process, or die
	DWORD dwRet = WaitForSingleObject(NewP->m_hProcessStartEvent, INFINITE);
	CloseHandle(NewP->m_hProcessStartEvent);
	NewP->m_hProcessStartEvent=NULL;

	//check it's ok
	if(NewP->m_Win32PInfo.hProcess==NULL)
	{
		ktrace("failed to start the process\n");
		delete NewP;
		return NULL;
	}

	//running, this is it
	return NewP;
}

//Create a new process by forking an existing one
//The pid is allocated as next available pid
//Parent becomes the parent process
//Otherwise the new process is a clone
//
Process* Process::StartFork(Process * pParent)
{
	ktrace("StartFork()\n");

	/*
	launch win32 process stub
	copy over memory
	set stack/cpu contexts
	let process run
	*/
	Process * NewP = new Process();

	//allocate a new pid
	int cnt=0;
	while(++g_pKernelTable->m_LastPID)
	{
		if(++cnt > 10000)
		{
			ktrace("not pid's available for fork\n");
			delete NewP;
			return NULL;
		}

		//loop around pid's?
		if(g_pKernelTable->m_LastPID > 0xFFFF)
			g_pKernelTable->m_LastPID = 2; //MUST skip '1' which is only ever for init

		//free?
		if(g_pKernelTable->FindProcess(g_pKernelTable->m_LastPID) == NULL)
			break;
	}
	NewP->m_Pid = g_pKernelTable->m_LastPID;

	//parent is who?
	NewP->m_ParentPid = P->m_Pid;


	//add to the kernel
	g_pKernelTable->m_Processes.push_back(NewP);


	//events to co-ordinate starting
	NewP->m_hProcessStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	NewP->m_hForkDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//need to start the process and debug it (to trap kernel calls etc)
	//the debugger thread
	NewP->m_KernelThreadHandle = CreateThread(NULL, KERNEL_PROCESS_THREAD_STACK_SIZE, KernelProcessHandlerMain, NewP, 0, &NewP->m_KernelThreadId);
	if(NewP->m_KernelThreadHandle==NULL)
	{
		ktrace("failed to start debug thread for process\n");
		delete NewP;
		return NULL;
	}

	//Wait for the thread to start the process, or die
	DWORD dwRet = WaitForSingleObject(NewP->m_hProcessStartEvent, INFINITE);
	CloseHandle(NewP->m_hProcessStartEvent);
	NewP->m_hProcessStartEvent=NULL;

	//check it's ok
	if(NewP->m_Win32PInfo.hProcess==NULL)
	{
		ktrace("failed to start the process\n");
		delete NewP;
		return NULL;
	}


	//Wait for the new process to finish forking our state
	ktrace("Waiting for child to finish copying our state\n");
	dwRet = WaitForSingleObject(NewP->m_hForkDoneEvent, INFINITE);
	CloseHandle(NewP->m_hForkDoneEvent);
	NewP->m_hForkDoneEvent=NULL;


	//running, this is it
	return NewP;
}


//Handler Thread Entry
/*static*/ DWORD WINAPI Process::KernelProcessHandlerMain(LPVOID param)
{
	//need thread stuff
	g_pTraceBuffer = new char [KTRACE_BUFFER_SIZE];
	P = (Process*)param;

	P->m_bInWin32Setup = true;

	//Need COM
	CoInitialize(NULL);//Ex(NULL, COINIT_MULTITHREADED);

	//We run the debug loop at higher priority to hopefully receive and handle
	//child system calls faster. The child is doing all the normal processing
	//so this should not end up elevating it's priority and system impact
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	//Start the stub
	STARTUPINFO si;
	GetStartupInfo(&si);
	if(CreateProcess(NULL, (char*)KEOW_PROCESS_STUB, NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS, NULL, P->m_UnixPwd.GetWin32Path(), &si, &P->m_Win32PInfo) == FALSE)
	{
		//failed
		P->m_Win32PInfo.hProcess = NULL;
		SetEvent(P->m_hProcessStartEvent);
		return 0;
	}

	//started now
	SetEvent(P->m_hProcessStartEvent);


	//
	//the debugging loop
	//
	P->DebuggerLoop();
	ktrace("process handler debug loop exitted\n");


	//parent should get SIGCHLD when we exit
	if(P->m_Pid != 1)
	{
		Process * pParent = g_pKernelTable->FindProcess(P->m_ParentPid);
		if(pParent)
			pParent->SendSignal(SIGCHLD);
	}

	//any children get inherited by init
	//also we are not ptracing any more
	KernelTable::ProcessList::iterator it;
	for(it=g_pKernelTable->m_Processes.begin();
	    it!=g_pKernelTable->m_Processes.end();
		++it)
	{
		Process * p2 = *it;
		if(p2->m_ParentPid == P->m_Pid)
			p2->m_ParentPid = 1; //init
		if(p2->m_ptrace.OwnerPid == P->m_Pid)
			p2->m_ptrace.OwnerPid = 0; //no-one
	}

	//cleanup
	//leave the process in the kernel table - it's parent will clean it up

	//clean up thread local stuff
	delete g_pTraceBuffer;
	P=NULL;

	CoUninitialize();

	return 0;
}


void Process::DebuggerLoop()
{
	DEBUG_EVENT evt;
	for(;;)
	{
		WaitForDebugEvent(&evt, INFINITE);

		//any signals to dispatch first?
		for(int sig=0; sig<_NSIG; ++sig)
		{
			if(m_PendingSignals[sig])
			{
				m_CurrentSignal = sig;
				HandleSignal(sig);
				m_CurrentSignal = 0;
			}
		}

		//handle the real reason we are in the debugger
		switch(evt.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			CloseHandle(evt.u.CreateProcessInfo.hFile); //don't need this
			GetSystemTimeAsFileTime(&m_StartedTime);
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			m_dwExitCode = evt.u.ExitProcess.dwExitCode;
			m_bStillRunning = false; //parent will clean up the process table entry
			return; //no longer debugging


		case CREATE_THREAD_DEBUG_EVENT:
		case EXIT_THREAD_DEBUG_EVENT:
			break;


		case EXCEPTION_DEBUG_EVENT:
			if(m_bInWin32Setup)
			{
				switch(evt.u.Exception.ExceptionRecord.ExceptionCode)
				{
				case EXCEPTION_SINGLE_STEP:
					ConvertProcessToKeow();
					SetSingleStep(false,NULL); //DEBUG - keep it up
					break;
				case EXCEPTION_BREAKPOINT:
					//seems to get generated when kernel32 loads.
					//ignore it while we are still in setup
					break;
				default:
					//ignore?
					break;
				}
			}
			else
				HandleException(evt);
			break;


		case LOAD_DLL_DEBUG_EVENT:
		case UNLOAD_DLL_DEBUG_EVENT:
			break;

		case OUTPUT_DEBUG_STRING_EVENT:
			{
				int bufLen = evt.u.DebugString.nDebugStringLength*2;
				char * buf = new char[bufLen];

				ReadMemory(buf, (ADDR)evt.u.DebugString.lpDebugStringData, evt.u.DebugString.nDebugStringLength);
				buf[evt.u.DebugString.nDebugStringLength] = 0;

				ktrace("relay: %*s", evt.u.DebugString.nDebugStringLength, buf);

				delete buf;
			}
			break;

		default:
			ktrace("ignoring unhandled debug event code %ld\n", evt.dwDebugEventCode);
			break;
		}


		//continue running
		FlushInstructionCache(m_Win32PInfo.hProcess, 0, 0); //need this for our process modifications?
		ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, DBG_CONTINUE);

	}//for(;;)
}


//Process starts as a win32 stub, need to set up a unix-like process in it
//
void Process::ConvertProcessToKeow()
{
	//Get the info about the win32 side, before changing to keow/unix
	//The stub provides this via the loading of the KeowUserSysCalls dll
	//The info is passed as a pointer in Eax and causing an initial breakpoint 

	m_bInWin32Setup = false;

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(m_Win32PInfo.hThread, &ctx);

	//This is the details we use as win32 state for code injection etc
	//keep this seperate from the keow stuff that happens later (eg FS reg changes)
	m_BaseWin32Ctx = ctx;


	//read syscall data from dll (pointer in eax)
	ReadMemory(&SysCallAddr, (ADDR)ctx.Eax, sizeof(SysCallAddr));

#ifdef _DEBUG
	//ensure correct coding - all functions are populated
	for(int a=0; a<sizeof(SysCallAddr); a+=sizeof(DWORD))
	{
		DWORD * pa = (DWORD*)( ((LPBYTE)&SysCallAddr) + a );
		if(*pa == 0)
		{
			ktrace("Bad SysCallDll coding in dll\n");
			DebugBreak();
		}
	}
#endif

	//disable single-step that the stub started (to alert us)
	SetSingleStep(false, &ctx);
	SetThreadContext(m_Win32PInfo.hThread, &ctx);


	//are we forking or starting init?

	if(m_hForkDoneEvent!=NULL)
	{
		//forking

		Process * pParent = g_pKernelTable->FindProcess(m_ParentPid);
		ForkCopyOtherProcess(*pParent);

		SetEvent(m_hForkDoneEvent);
		return;
	}
	//starting init


	//want a new stack for the process
	// - at the TOP of the address space and top being like 0xbfffffff ???
	//[I got this value from gdb of /bin/ls on debian i386 linux]
	//also seems that some libpthread code relies on (esp|0x1ffff) being the top of the stack.
	//TODO: alloc dynamically within the contraints
	DWORD size=1024*1024L;  //1MB will do?
	ADDR stack_bottom = (ADDR)0x70000000 - size; //ok location?
	m_KeowUserStackBase = MemoryHelper::AllocateMemAndProtect(stack_bottom, size, PAGE_EXECUTE_READWRITE);
	m_KeowUserStackTop = m_KeowUserStackBase + size;
	//linux process start with a little bit of stack in use but overwritable?
	ctx.Esp = (DWORD)m_KeowUserStackTop - 0x200;
	SetThreadContext(m_Win32PInfo.hThread, &ctx);

	//not fork or exec, must be 'init', the very first process
	LoadImage(m_ProcessFileImage, false);

	//std in,out,err
	m_OpenFiles[0] = new IOHNtConsole(g_pKernelTable->m_pMainConsole);
	m_OpenFiles[1] = new IOHNtConsole(g_pKernelTable->m_pMainConsole);
	m_OpenFiles[2] = new IOHNtConsole(g_pKernelTable->m_pMainConsole);
	m_OpenFiles[0]->SetInheritable(true);
	m_OpenFiles[1]->SetInheritable(true);
	m_OpenFiles[2]->SetInheritable(true);
}


//This is either a fault to signal to the process,
//or it's a kernel call to intercept
//
void Process::HandleException(DEBUG_EVENT &evt)
{
	/* From Winnt.H for reference
		#define CONTEXT_CONTROL         (CONTEXT_i386 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP
		#define CONTEXT_INTEGER         (CONTEXT_i386 | 0x00000002L) // AX, BX, CX, DX, SI, DI
		#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 0x00000004L) // DS, ES, FS, GS
		#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 0x00000008L) // 387 state
		#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x00000010L) // DB 0-3,6,7
		#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L) // cpu specific extensions

		#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER |\
							  CONTEXT_SEGMENTS)
	*/

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS;
	GetThreadContext(m_Win32PInfo.hThread, &ctx);

	switch(evt.u.Exception.ExceptionRecord.ExceptionCode)
	{
	case EXCEPTION_SINGLE_STEP:
		DumpContext(ctx);
		SetSingleStep(true, &ctx); //keep it up (DEbug only?)
		break;

	case EXCEPTION_ACCESS_VIOLATION:
		{
			//is this an INT 80 linux SYSCALL?

			WORD instruction;
			ReadMemory(&instruction, (ADDR)evt.u.Exception.ExceptionRecord.ExceptionAddress, sizeof(instruction));

			if(instruction == 0x80CD)
			{
				//skip over the INT instruction
				//so that the process resumes on the next instruction
				ctx.Eip += 2;

				//handle int 80h
				SysCalls::HandleInt80SysCall(ctx);
			}
			else
			{
				ktrace("Access violation @ 0x%08lx\n", evt.u.Exception.ExceptionRecord.ExceptionAddress);

				DumpContext(ctx);
				DumpStackTrace(ctx);

				SendSignal(SIGSEGV); //access violation
			}
		}
		break;

	case EXCEPTION_FLT_DENORMAL_OPERAND:
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_FLT_INEXACT_RESULT:
	case EXCEPTION_FLT_INVALID_OPERATION:
	case EXCEPTION_FLT_OVERFLOW:
	case EXCEPTION_FLT_STACK_CHECK:
	case EXCEPTION_FLT_UNDERFLOW:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_OVERFLOW:
		ktrace("math exception\n");
		SendSignal(SIGFPE);
		break;

	case EXCEPTION_PRIV_INSTRUCTION:
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		ktrace("instruction exception\n");
		SendSignal(SIGILL);
		break;

	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
	case EXCEPTION_IN_PAGE_ERROR:
	case EXCEPTION_STACK_OVERFLOW:
	case EXCEPTION_GUARD_PAGE:
		ktrace("page access exception\n");
		SendSignal(SIGSEGV);
		break;

	case EXCEPTION_BREAKPOINT:
		DebugBreak();
		//TODO: if ptrace send to parent, else terminate signal
		break;

	default:
		ktrace("Unhandled exception type 0x%08lx @ 0x%08lx\n", evt.u.Exception.ExceptionRecord.ExceptionCode, evt.u.Exception.ExceptionRecord.ExceptionAddress);
		SendSignal(SIGSEGV);
		break;
	}


	//SetSingleStep(true); //DEBUG - keep it up

	//set any context changes
	SetThreadContext(m_Win32PInfo.hThread, &ctx);
}


// Clone an existing process into this one,
// except for a few things (eg ptrace, pid, ppid)
void Process::ForkCopyOtherProcess(Process &other)
{
	int i;
	ktrace("fork : Copying state from parent process\n");

	m_ProcessGroupPID = other.m_ProcessGroupPID;

	m_gid = other.m_gid;
	m_uid = other.m_uid;
	m_egid = other.m_egid;
	m_euid = other.m_euid;

	m_saved_uid = other.m_saved_uid;
	m_saved_gid = other.m_saved_gid;

	m_umask = other.m_umask;

	//use same thread (it'll be valid after we copy the other processes memory allocations)
	m_KeowUserStackBase = other.m_KeowUserStackBase;
	m_KeowUserStackTop = other.m_KeowUserStackTop;

	m_ProcessFileImage = other.m_ProcessFileImage;
	m_UnixPwd = other.m_UnixPwd;

	m_CommandLine = other.m_CommandLine;

	m_ElfLoadData = other.m_ElfLoadData;

	//DO NOT copy the ptrace state on a fork
	//m_ptrace;

	//these are valid after memory copy
	m_Environment = other.m_Environment;
	m_Arguments = other.m_Arguments;
	m_ArgCnt = other.m_ArgCnt;
	m_EnvCnt = other.m_EnvCnt;

	//signal handling
	memcpy(&m_PendingSignals, &other.m_PendingSignals, sizeof(m_PendingSignals));
	memcpy(&m_SignalMask, &other.m_SignalMask, sizeof(m_SignalMask));
	memcpy(&m_SignalAction, &other.m_SignalAction, sizeof(m_SignalAction));
	m_SignalDepth = other.m_SignalDepth;

	//copy memory allocations (except mmap)
	MemoryAllocationsList::iterator mem_it;
	for(mem_it = other.m_MemoryAllocations.begin();
	    mem_it != other.m_MemoryAllocations.end();
		++mem_it)
	{
		MemoryAlloc * pMemAlloc = *mem_it;

		ADDR p = MemoryHelper::AllocateMemAndProtect(pMemAlloc->addr, pMemAlloc->len, pMemAlloc->protection);
		MemoryHelper::TransferMemory(other.m_Win32PInfo.hProcess, pMemAlloc->addr, m_Win32PInfo.hProcess, p, pMemAlloc->len);
	}

	//clone files
	for(i=0; i<MAX_OPEN_FILES; ++i)
	{
		if(other.m_OpenFiles[i]==NULL)
			continue;

		m_OpenFiles[i] = other.m_OpenFiles[i]->Duplicate();
	}

	//copy mmap'd memory
	MmapList::iterator mmap_it;
	for(mmap_it = other.m_MmapList.begin();
	    mmap_it != other.m_MmapList.end();
		++mmap_it)
	{
		MMapRecord * pMmap = *mmap_it;

		DebugBreak(); //TODO: handle cloning mmaps 
	}

		
	//Clone the context
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS;
	GetThreadContext(other.m_Win32PInfo.hThread, &ctx);

	//we are the child - need zero return from the fork
	ctx.Eip += 2; //skip the Int 80
	ctx.Eax = 0; //we are the child

	SetThreadContext(m_Win32PInfo.hThread, &ctx);

	ktrace("fork : copy done\n");
}


//Load the process executable images
//and then set the process to run
//
DWORD Process::LoadImage(Path &img, bool LoadAsLibrary)
{
	DWORD rc;

	HANDLE hImg = CreateFile(m_ProcessFileImage.GetWin32Path().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if(hImg==INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	//Read the first chunk of the file
	int buflen=1024; //plenty enough to understand the file
	DWORD dwRead;
	BYTE * buf = new BYTE[buflen];
	memset(buf,0,buflen);
	ReadFile(hImg, buf, buflen, &dwRead, 0);

	//determine exe type
	struct linux::elf32_hdr * pElf; //ELF header
	pElf = (struct linux::elf32_hdr *)buf;
	if(pElf->e_ident[EI_MAG0] == ELFMAG0
	&& pElf->e_ident[EI_MAG1] == ELFMAG1
	&& pElf->e_ident[EI_MAG2] == ELFMAG2
	&& pElf->e_ident[EI_MAG3] == ELFMAG3)
	{
		rc = LoadElfImage(hImg, pElf, &m_ElfLoadData, LoadAsLibrary);
	}
	else
	if(buf[0]=='#'
	&& buf[1]=='!')
	{
		//yes - it is a shell script with a header
		// eg: #!/bin/sh
		//Use that program as the process and give it the args we specified
		char * space = strchr((char*)buf, ' ');
		char * nl = strchr((char*)buf, 0x0a);
		if(nl==NULL)
		{
			buf[buflen-1] = NULL;
			space = NULL;
		}
		else
		{
			*nl = NULL; //line ends here

			char * args;
			if(space==NULL || space>nl)
				args = NULL;
			else
				args = space+1;

			//skip #! and any whitespace
			char * interp = (char*)buf + 2;
			while(*interp == ' ')
				interp++;

			rc = LoadImage(Path(interp), LoadAsLibrary);

			if(args)
				m_CommandLine = args;
		}
	}
	else
	{
		//unhandled file format
		rc = ERROR_BAD_FORMAT;
	}


	//done with handles
	delete buf;
	CloseHandle(hImg);


	//make the stub execute the image
	StartNewImageRunning();

	return rc;
}


DWORD Process::LoadElfImage(HANDLE hImg, struct linux::elf32_hdr * pElfHdr, ElfLoadData * pElfLoadData, bool LoadAsLibrary)
{
	//Validate the ELF file
	//

	if(pElfHdr->e_type != ET_EXEC
	&& pElfHdr->e_type != ET_DYN)
	{
		ktrace("not an ELF executable type\n");
		return ERROR_BAD_FORMAT;
	} 
	if(pElfHdr->e_machine != EM_386)
	{
		ktrace("not for Intel 386 architecture (needed for syscall interception)\n");
		return ERROR_BAD_FORMAT;
	}
	if(pElfHdr->e_version != EV_CURRENT
	|| pElfHdr->e_ident[EI_VERSION] != EV_CURRENT )
	{
		ktrace("not version %d\n", EV_CURRENT);
		return ERROR_BAD_FORMAT;
	}


	//Load the image into the keow stub process
	//
	int i;
	linux::Elf32_Phdr * phdr; //Program header
	ADDR pMem, pWantMem, pAlignedMem;
	DWORD AlignedSize;
	DWORD protection;//, oldprot;
	DWORD loadok;
	ADDR pMemTemp = NULL;
	ADDR pBaseAddr = 0;

	if(pElfHdr->e_phoff == 0)
		return -1; //no header

	//what about these??
	if(LoadAsLibrary || pElfHdr->e_type==ET_DYN)
	{
		if(pElfLoadData->last_lib_addr == 0)
			pBaseAddr = (ADDR)0x40000000L; //linux 2.4 x86 uses this for start of libs?
		else
			pBaseAddr = pElfLoadData->last_lib_addr;
		//align to next 64k boundary
		pBaseAddr = (ADDR)( ((DWORD)pBaseAddr + (SIZE64k-1)) & (~(SIZE64k-1)) ); 
		pElfLoadData->interpreter_base = pBaseAddr;
	}
	ktrace("using base address 0x%08lx\n", pBaseAddr);

	
	pElfLoadData->Interpreter[0] = 0;
	pElfLoadData->start_addr = pElfHdr->e_entry + pBaseAddr;

	loadok=1;

	phdr = (linux::Elf32_Phdr*)new char[pElfHdr->e_phentsize];
	if(phdr==0)
		loadok=0;

	//load program header segments
	//initial load - needs writable memory pages
	DWORD dwRead;
	for(i=0; loadok && i<pElfHdr->e_phnum; ++i)
	{
		SetFilePointer(hImg, pElfHdr->e_phoff + (i*pElfHdr->e_phentsize), 0, FILE_BEGIN);
		ReadFile(hImg, phdr, pElfHdr->e_phentsize, &dwRead, 0);

		if(phdr->p_type == PT_LOAD)
		{
			//load segment into memory
			protection = ElfProtectionToWin32Protection(phdr->p_flags);
			pWantMem = phdr->p_vaddr + pBaseAddr;
			pAlignedMem = pWantMem;
			AlignedSize = phdr->p_memsz;
			if(phdr->p_align > 1)
			{
				//need to handle alignment
				//start addr rounds down
				//length rounds up

				pAlignedMem = (ADDR)( (DWORD)pAlignedMem & ~(phdr->p_align-1) );
				AlignedSize = (phdr->p_memsz + (pWantMem-pAlignedMem) + (phdr->p_align-1)) & ~(phdr->p_align-1);
			}

			ktrace("load segment file offset %d, file len %d, mem len %d (%d), to 0x%p (%p)\n", phdr->p_offset, phdr->p_filesz, phdr->p_memsz, AlignedSize, phdr->p_vaddr, pAlignedMem);

			pMem = MemoryHelper::AllocateMemAndProtect(pAlignedMem, AlignedSize, PAGE_EXECUTE_READWRITE);
			if(pMem!=pAlignedMem)
			{
				DWORD err = GetLastError();
				ktrace("error 0x%lx in load program segment @ 0x%lx (got 0x%lx)\n", err, pAlignedMem, pMem);
				loadok = 0;
				break;
			}

			//need to zero first? (alignment creates gaps needing filled with zeros)
			MemoryHelper::FillMem(P->m_Win32PInfo.hProcess, pAlignedMem, AlignedSize, 0);

			//need to load file into our memory, then copy to process
			if(phdr->p_filesz != 0)
			{
				pMemTemp = (ADDR)realloc(pMemTemp, phdr->p_filesz);
				SetFilePointer(hImg, phdr->p_offset, 0, FILE_BEGIN);
				ReadFile(hImg, pMemTemp, phdr->p_filesz, &dwRead, 0);

				if(dwRead != phdr->p_filesz)
				{
					DWORD err = GetLastError();
					ktrace("error 0x%lx in load program segment, read %d (got %d)\n", err, dwRead, phdr->p_filesz);
					loadok = 0;
					break;
				}

				//load into ask addr, not the aligned one
				if(!WriteMemory(pWantMem, phdr->p_filesz, pMemTemp))
				{
					DWORD err = GetLastError();
					ktrace("error 0x%lx in write program segment\n", err);
					loadok = 0;
					break;
				}
			}


			if(!LoadAsLibrary)
			{
				//keep track largest filled size and allocated size
				//between start_bss and last_bss is the bs section.
				if(pMem+phdr->p_filesz  > pElfLoadData->bss_start)
					pElfLoadData->bss_start = pMem + phdr->p_filesz;
				if(pAlignedMem+AlignedSize > pElfLoadData->brk)
					pElfLoadData->brk = pAlignedMem+AlignedSize;

				//keep track in min/max addresses
				if(pMem<pElfLoadData->program_base || pElfLoadData->program_base==0)
					pElfLoadData->program_base = pMem;
				if(pMem+phdr->p_memsz > pElfLoadData->program_max)
					pElfLoadData->program_max = pMem+phdr->p_memsz;
			}
			else
			{
				if(pMem+phdr->p_memsz  > pElfLoadData->last_lib_addr)
					pElfLoadData->last_lib_addr = pMem + phdr->p_memsz;
			}

		}
		else
		if(phdr->p_type == PT_PHDR)
		{
			//will be loaded later in a PT_LOAD
			pElfLoadData->phdr_addr = phdr->p_vaddr + pBaseAddr;

			pElfLoadData->phdr_phnum = pElfHdr->e_phnum;
			pElfLoadData->phdr_phent = pElfHdr->e_phentsize;
		}
		else
		if(phdr->p_type == PT_INTERP)
		{
			SetFilePointer(hImg, phdr->p_offset, 0, FILE_BEGIN);
			ReadFile(hImg, pElfLoadData->Interpreter, phdr->p_filesz, &dwRead, 0);
		}
	}

	//align brk to a page boundary
	//NO!! //pElf->brk = (void*)( ((DWORD)pElf->brk + (SIZE4k-1)) & (~(SIZE4k-1)) ); 

	//load interpreter?
	if(pElfLoadData->Interpreter[0]!=0)
	{
		if(strcmp(pElfLoadData->Interpreter, "/usr/lib/libc.so.1")==0
		|| strcmp(pElfLoadData->Interpreter, "/usr/lib/ld.so.1")==0 )
		{
			//IBCS2 interpreter? (tst from linux binfmt_elf.c)
			//I think these expect to use intel Call Gates 
			//we cannot (yet?) do these (only int80) so fail
			ktrace("Cannot handle IBCS executables");
			loadok = 0;
		}

		//need a copy of the load data
		//the interpreter uses this to know where it can load to
		//and then we use the interpreter load results to update
		//the real process load data.
		ElfLoadData LoadData2;
		LoadData2 = *pElfLoadData;

		Path InterpPath(pElfLoadData->Interpreter);
		HANDLE hInterpImg = CreateFile(InterpPath.GetWin32Path(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
		if(hInterpImg==INVALID_HANDLE_VALUE)
		{
			ktrace("failed to load interpreter: %s\n", InterpPath.GetUnixPath().c_str());
			loadok = 0;
		}
		else
		{
			struct linux::elf32_hdr hdr2;
			ReadFile(hInterpImg, &hdr2, sizeof(hdr2), &dwRead, 0);

			DWORD rc = LoadElfImage(hInterpImg, &hdr2, &LoadData2, true);
			if(rc!=0)
				loadok=0;

			m_ElfLoadData.interpreter_start = LoadData2.start_addr;
			m_ElfLoadData.interpreter_base = LoadData2.interpreter_base;
			m_ElfLoadData.last_lib_addr = LoadData2.last_lib_addr;

			CloseHandle(hInterpImg);
		}

	}

	//protect all the pages
	/* does not work?
	for(i=0; loadok && i<pElfHdr->e_phnum; ++i)
	{
		if(phdr->p_type == PT_LOAD
		|| phdr->p_type == PT_PHDR)
		{
			fseek(fElf, pElfHdr->e_phoff + (i*pElfHdr->e_phentsize), SEEK_SET);
			fread(phdr, pElfHdr->e_phentsize, 1, fElf);

			if(phdr->p_vaddr == 0
			|| phdr->p_memsz == 0 )
				continue;

			pMem = CalculateVirtualAddress(phdr, pBaseAddr);
			protection = ElfProtectionToWin32Protection(phdr->p_flags);
			if(!VirtualProtectEx(pElf->pinfo.hProcess, pMem, phdr->p_memsz, protection, &oldprot))
			{
				DWORD err = GetLastError();
				ktrace("error 0x%lx in protect program segment @ 0x%lx (got 0x%lx)\n", err,pWantMem, pMem);
				loadok = 0;
				break;
			}
		}
	}
	*/

	delete phdr;
	free(pMemTemp); //used 'realloc', not 'new'
	if(loadok)
		return 0;

	ktrace("program load failed\n");
	return -1;
}


// Transfers control to the image to run it
// This is only for 'new' images (exec() etc)
//
DWORD Process::StartNewImageRunning()
{
	//Place some ELF interpreter start info onto the stack
	//and then jump to the start of the program
	/*
	When the userspace receives control, the stack layout has a fixed format.
	The rough order is this:

		   <arguments> <environ> <auxv> <string data>

	The detailed layout, assuming IA32 architecture, is this (Linux kernel
	series 2.2/2.4):

	  position            content                     size (bytes) + comment
	  ------------------------------------------------------------------------
	  stack pointer ->  [ argc = number of args ]     4
						[ argv[0] (pointer) ]         4   (program name)
						[ argv[1] (pointer) ]         4
						[ argv[..] (pointer) ]        4 * x
						[ argv[n - 1] (pointer) ]     4
						[ argv[n] (pointer) ]         4   (= NULL)

						[ envp[0] (pointer) ]         4
						[ envp[1] (pointer) ]         4
						[ envp[..] (pointer) ]        4
						[ envp[term] (pointer) ]      4   (= NULL)

						[ auxv[0] (Elf32_auxv_t) ]    8
						[ auxv[1] (Elf32_auxv_t) ]    8
						[ auxv[..] (Elf32_auxv_t) ]   8
						[ auxv[term] (Elf32_auxv_t) ] 8   (= AT_NULL vector)

						[ padding ]                   0 - 16

						[ argument ASCIIZ strings ]   >= 0
						[ environment ASCIIZ str. ]   >= 0

	  (0xbffffffc)      [ end marker ]                4   (= NULL)

	  (0xc0000000)      < top of stack >              0   (virtual)
	  ------------------------------------------------------------------------

	When the runtime linker (rtld) has done its duty of mapping and resolving
	all the required libraries and symbols, it does some initialization work
	and hands over the control to the real program entry point afterwards. As
	this happens, the conditions are:

			- All required libraries mapped from 0x40000000 on
			- All CPU registers set to zero, except the stack pointer ($sp) and
			  the program counter ($eip/$ip or $pc). The ABI may specify
			  further initial values, the i386 ABI requires that %edx is set to
			  the address of the DT_FINI function.

	 ...

	On Linux, the runtime linker requires the following Elf32_auxv_t
	structures:

			AT_PHDR, a pointer to the program headers of the executeable
			AT_PHENT, set to 'e_phentsize' element of the ELF header (constant)
			AT_PHNUM, number of program headers, 'e_phnum' from ELF header
			AT_PAGESZ, set to constant 'PAGE_SIZE' (4096 on x86)
			AT_ENTRY, real entry point of the executeable (from ELF header)

	*/

	if(m_ElfLoadData.interpreter_start==0)
	{
		m_ElfLoadData.interpreter_base = m_ElfLoadData.program_base;
		m_ElfLoadData.interpreter_start = m_ElfLoadData.start_addr;
		ktrace("elf, no interpreter: entry @ 0x%08lx\n", m_ElfLoadData.interpreter_start);
	}

	//when a new process first starts, it's args etc are in the kernel
	//transfer them to the process
	DWORD dwEnvSize, dwArgsSize;
	ADDR pEnv = MemoryHelper::CopyStringListBetweenProcesses(GetCurrentProcess(), m_Environment, m_Win32PInfo.hProcess, this, &m_EnvCnt, &dwEnvSize);
	ADDR pArgs = MemoryHelper::CopyStringListBetweenProcesses(GetCurrentProcess(), m_Arguments, m_Win32PInfo.hProcess, this, &m_ArgCnt, &dwArgsSize);
	//strings are now in the other process
	//free kernel copy
	VirtualFree(m_Arguments, dwArgsSize, MEM_DECOMMIT);
	VirtualFree(m_Environment, dwEnvSize, MEM_DECOMMIT);
	//point to the process
	m_Environment = pEnv;
	m_Arguments = pArgs;
	ktrace("args @ %p\n", m_Arguments);
	ktrace("env @ %p\n", m_Environment);

	//stack data needed
	const int AUX_RESERVE = 2*sizeof(DWORD)*20; //heaps for what is below
	int stack_needed = sizeof(ADDR)*(m_EnvCnt+1+m_ArgCnt+1) + AUX_RESERVE + sizeof(ADDR)/*end marker*/;

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(m_Win32PInfo.hThread, &ctx);

	ADDR StackTop = (ADDR)ctx.Esp;

	//stack grows DOWN
	ctx.Esp -= stack_needed;

	//copy data to the stack
	//write so that first written are popped first
	ADDR addr = (ADDR)ctx.Esp;

	//argc
	WriteMemory(addr, sizeof(DWORD), &m_ArgCnt);
	addr += sizeof(DWORD);
	//argv[]: clone the array of pointers that are already in the target process
	//include the null end entry
	MemoryHelper::TransferMemory(m_Win32PInfo.hProcess, m_Arguments, m_Win32PInfo.hProcess, addr, (m_ArgCnt+1)*sizeof(ADDR));
	addr += (m_ArgCnt+1)*sizeof(ADDR);

	//envp[]: clone the array of pointers that are already in the target process
	//include the null end entry
	MemoryHelper::TransferMemory(m_Win32PInfo.hProcess, m_Environment, m_Win32PInfo.hProcess, addr, (m_EnvCnt+1)*sizeof(ADDR));
	addr += (m_EnvCnt+1)*sizeof(ADDR);

	//aux
	struct Elf32_auxv_t {
		DWORD type;	//pop this first
		DWORD val;	//2nd
	} Aux;

#define PUSH_AUX_VAL(t,v) \
		Aux.type = t; \
		Aux.val = v; \
		WriteMemory(addr, sizeof(Aux), &Aux); \
		addr += sizeof(Aux);

	PUSH_AUX_VAL(AT_GID,	m_gid)
	PUSH_AUX_VAL(AT_UID,	m_uid)
	PUSH_AUX_VAL(AT_ENTRY,	(DWORD)m_ElfLoadData.start_addr) //real start - ignoring 'interpreter'
	PUSH_AUX_VAL(AT_BASE,	(DWORD)m_ElfLoadData.interpreter_base)
	PUSH_AUX_VAL(AT_PHNUM,	m_ElfLoadData.phdr_phnum)
	PUSH_AUX_VAL(AT_PHENT,	m_ElfLoadData.phdr_phent)
	PUSH_AUX_VAL(AT_PHDR,	(DWORD)m_ElfLoadData.phdr_addr)
	PUSH_AUX_VAL(AT_PAGESZ,	0x1000) //4K
	PUSH_AUX_VAL(AT_NULL,	0)

#undef PUSH_AUX_VAL

	//end marker (NULL)
	DWORD zero=0;
	WriteMemory(addr, sizeof(DWORD), &zero);
	addr += sizeof(DWORD);


	//init required registers
	//(is this all we need?)
	ctx.Eax = 0;
	ctx.Ebx = 0;
	ctx.Ecx = 0;
	ctx.Edx = 0;
	ctx.Esi = 0;
	ctx.Edi = 0;
	ctx.Ebp = 0;

	//start here
	ctx.Eip = (DWORD)m_ElfLoadData.interpreter_start;

	//set
	SetThreadContext(m_Win32PInfo.hThread, &ctx);

	return 0;
}

void Process::SetSingleStep(bool set, CONTEXT * pCtx)
{
	const int SINGLE_STEP_BIT = 0x100L; //8th bit is trap flag
	CONTEXT TmpCtx;
	bool bDoSet = false;

	if(pCtx==NULL)
	{
		TmpCtx.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(m_Win32PInfo.hThread, &TmpCtx);

		pCtx = &TmpCtx;
	}

	if(set)
	{
		//enable single-step 
		pCtx->EFlags |= SINGLE_STEP_BIT; 
	}
	else
	{
		//disable single-step 
		pCtx->EFlags &= ~SINGLE_STEP_BIT;
	}

	if(bDoSet)
		SetThreadContext(m_Win32PInfo.hThread, &TmpCtx);

}


bool Process::ReadMemory(LPVOID pBuf, ADDR addr, DWORD len)
{
	return MemoryHelper::ReadMemory((ADDR)pBuf, m_Win32PInfo.hProcess, addr, len);
}

bool Process::WriteMemory(ADDR addr, DWORD len, const void * pBuf)
{
	return MemoryHelper::WriteMemory(m_Win32PInfo.hProcess, addr, len, (ADDR)pBuf);
}


void Process::SendSignal(int sig)
{
	//deal with it immediatly (it's on this kernel thread)
	//or schedule it for the correct thread

	if(this==P)
	{
		//we are the thread handling this process
		m_CurrentSignal = sig;
		HandleSignal(sig);
		m_CurrentSignal = 0;
		return;
	}

	//send the signal to this process
	m_PendingSignals[sig] = true;
	SetEvent(m_hWaitTerminatingEvent);

	//NOTE: this may be executing on a different thread than the one handling this process

	//because it may not be in the debugger
	SuspendThread(m_Win32PInfo.hThread);

	//make single-step
	//this forces the debugger to intervene and then it can see the pending signals
	SetSingleStep(true, NULL);

	ResumeThread(m_Win32PInfo.hThread);
}


void Process::HandleSignal(int sig)
{
	ktrace("signal handler starting for %d (depth %d)\n", sig, m_SignalDepth);

	//participate in ptrace()
	if(m_ptrace.OwnerPid)
	{
		ktrace("stopping for ptrace in HandleSignal\n");
		m_ptrace.new_signal = sig;
		g_pKernelTable->FindProcess(m_ptrace.OwnerPid)->SendSignal(SIGCHLD);
		SuspendThread(GetCurrentThread()); //the debugger stops here while parent is ptracing

		ktrace("resumed from ptrace stop in HandleSignal\n");
		//did tracer alter the signal to deliver?
		if(sig != m_ptrace.new_signal)
		{
			sig = m_ptrace.new_signal;
			ktrace("ptrace replacement signal %d\n", sig);
			if(sig==0)
				return;
			//sig is different now
		}
	}

	//some signals cannot be caught
	switch(sig)
	{
	case SIGKILL:
		ktrace("killed - sigkill\n");
		m_KilledBySig=sig;
		SysCallDll::exit(-sig);
		return;
	case SIGSTOP:
		ktrace("stopping on sigstop\n");
		SuspendThread(m_Win32PInfo.hThread);
		return;
	}

	//signal handler debugging
	//DebugBreak();

	//signal is blocked by this function?
	//if(pProcessData->signal_blocked[sig] > 0)
	//{
	//	ktrace("signal not delivered - currently blocked\n");
	//	pProcessData->current_signal = 0;
	//	return;
	//}

	//is this process ignoring this signal?
	linux::sigset_t &mask = m_SignalMask[m_SignalDepth];
	if( (mask.sig[(sig-1)/_NSIG_BPW]) & (1 << (sig-1)%_NSIG_BPW) )
	{
		ktrace("signal not delivered - currently masked\n");
		return;
	}


	//nested
	m_SignalDepth++;
	if(m_SignalDepth >= MAX_PENDING_SIGNALS)
	{
		m_SignalDepth= MAX_PENDING_SIGNALS-1;
		ktrace("WARN overflow in nested signal handling\n");
	}
	m_SignalMask[m_SignalDepth] = m_SignalAction[sig].sa_mask;

	//is there a signal handler registered for this signal?
	//	pProcessData->signal_action[signum].sa_handler     = act->sa_handler;
	//	pProcessData->signal_action[signum].sa_flags       = act->sa_flags;
	//	pProcessData->signal_action[signum].sa_restorer    = act->sa_restorer;
	//	pProcessData->signal_action[signum].sa_mask.sig[0] = act->sa_mask;
	if(m_SignalAction[sig].sa_handler == SIG_DFL)
	{
		//default action for this signal
		ktrace("executing default action for signal\n");
		switch(sig)
		{
		//ignore
		case SIGCHLD:
		case SIGURG:
			break;

		case SIGCONT: 
			ResumeThread(m_Win32PInfo.hThread);
			break;

		//stop
		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			SuspendThread(m_Win32PInfo.hThread);
			break;

		//terminate
		case SIGHUP:
		case SIGINT:
		case SIGPIPE:
		case SIGALRM:
		case SIGTERM:
		case SIGUSR1:
		case SIGUSR2:
		case SIGPOLL:
		case SIGPROF:
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
			m_KilledBySig=sig;
			SysCallDll::exit(-sig);
			break;

		//ptrace
		case SIGTRAP:
			if(m_ptrace.OwnerPid)
				break; //ignore when being ptraced
			//else drop thru to core dump

		//core dump
		case SIGQUIT:
		case SIGILL:
		case SIGABRT:
		case SIGFPE:
		case SIGSEGV:
		case SIGBUS:
		case SIGSYS:
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
			GenerateCoreDump();
			m_KilledBySig=sig;
			SysCallDll::exit(-sig);
			break;

		default:
			ktrace("IMPLEMENT default action for signal %d\n", sig);
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
			m_KilledBySig=sig;
			SysCallDll::exit(-sig);
			break;
		}
	}
	else
	if(m_SignalAction[sig].sa_handler == SIG_IGN)
	{
		//ignore the signal
		ktrace("ignoring signal - sig_ign\n");
	}
	else
	{
		ktrace("dispatching to signal handler @ 0x%08lx\n", m_SignalAction[sig].sa_handler);

		//supposed to supress the signal whilst in the handler?
		//linux just sets to SIG_DFL? (see man 2 signal)
		linux::__sighandler_t handler = m_SignalAction[sig].sa_handler;

		//restore handler?
		if((m_SignalAction[sig].sa_flags & SA_ONESHOT)
		|| (m_SignalAction[sig].sa_flags & SA_RESETHAND) )
			m_SignalAction[sig].sa_handler = SIG_DFL;


		//need to update user process
		CONTEXT ctx;
		ctx.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(P->m_Win32PInfo.hThread, &ctx);

		//dispatch to custom handler
		if(m_SignalAction[sig].sa_flags & SA_SIGINFO)
		{
			linux::siginfo si;
			si.si_signo = sig;
			si.si_errno = 0;
			si.si_code = SI_USER;
			ktrace("IMPLEMENT correct signal sa_action siginfo stuff\n");

			/*
			linux::ucontext ct;
			ct.uc_flags = ?;
			ct.uc_link = 0;
			stack_t		  uc_stack;
			struct sigcontext uc_mcontext;
			sigset_t	  uc_sigmask;	/* mask last for extensibility */
			ktrace("IMPLEMENT correct signal sa_action ucontext stuff\n");
			//copy the context to the process and pass as param


			//invoke: void _cdecl handler(int, linux::siginfo *, void *)

			//add new items to write onto the stack
			struct {
				struct {
					DWORD ReturnAddress;
					int signal;
					void * pSiginfo;
					void * pData;
				} call_data;
				linux::siginfo si;
			} stack;

			ctx.Esp -= sizeof(stack);

			stack.call_data.ReturnAddress = ctx.Eip;
			stack.call_data.signal = sig;
			stack.call_data.pSiginfo = (void*)(ctx.Esp - sizeof(stack) + sizeof(stack.call_data));
			stack.call_data.pData = 0;
			stack.si = si; //place a copy on the stack

			//update stack and then run the handler
			P->WriteMemory((ADDR)ctx.Esp, sizeof(stack), &stack);
			ctx.Eip = (DWORD)handler;
			SetThreadContext(P->m_Win32PInfo.hThread, &ctx);
		}
		else
		{
			//invoke: void _cdecl handler(int)

			//add new items to write onto the stack
			struct {
				DWORD ReturnAddress;
				int signal;
			} stack;

			ctx.Esp -= sizeof(stack);

			stack.ReturnAddress = ctx.Eip;
			stack.signal = sig;

			//update stack and then run the handler
			P->WriteMemory((ADDR)ctx.Esp, sizeof(stack), &stack);
			ctx.Eip = (DWORD)handler;
			SetThreadContext(P->m_Win32PInfo.hThread, &ctx);
		}

		//use restorer
		//in linux this is a call to sys_sigreturn ?
		//to return to user-land?
		//documented as 'dont use'
		//if(m_SignalAction[sig].sa_flags & SA_RESTORER)
		//{
		//	invoke: ((void (_cdecl *)(void))m_SignalAction[sig].sa_restorer)();
		//}

	}

		
	//un-nest
	m_SignalDepth--;
	if(m_SignalDepth<0)
		m_SignalDepth = 0;

	ktrace("signal handler ending for %d (depth %d)\n", sig, m_SignalDepth);
}


void Process::GenerateCoreDump()
{
	m_bCoreDumped = true;
	ktrace("Core Dump\n");
}

DWORD Process::InjectFunctionCall(void *func, void *pStackData, int nStackDataSize)
{
	//Expect that the func is declared as _stdcall calling convention.
	//(actually it ought to be from the SysCallDll::RemoteAddrInfo) 
	//This means we place args on the stack so that param1 pops first
	//Stack&register cleanup is irrelavent because HandleException() restores the correct state

	//Injection uses the original Win32 context (& stack etc) from when
	//the stub process started.

	ktrace("Injecting call to SysCallDll func @ 0x%08lx\n", func);

	CONTEXT InjectCtx = m_BaseWin32Ctx;

	InjectCtx.Eip = (DWORD)func;		//call this in the stub
	InjectCtx.Esp -= sizeof(ADDR);	//the return address
	InjectCtx.Esp -= nStackDataSize;	//the params

	SetThreadContext(m_Win32PInfo.hThread, &InjectCtx);


	ADDR addr = (ADDR)InjectCtx.Esp;

	//return address - we don't actually need it (exit trapped below) 
	DWORD dummy = 0;
	WriteMemory(addr, sizeof(DWORD), &dummy);
	addr += sizeof(DWORD);

	//parameters
	ADDR ParamAddr = addr;
	WriteMemory(addr, nStackDataSize, pStackData);
	addr += nStackDataSize;




	//resume the process to let it run the function
	//expect the stub to do a Break at the end
	//we'll capture the break and then return

	DEBUG_EVENT evt;
	for(;;) {
		//SetSingleStep(true); //debug

		FlushInstructionCache(m_Win32PInfo.hProcess, 0, 0); //need this for our process modifications?
		ContinueDebugEvent(m_Win32PInfo.dwProcessId, m_Win32PInfo.dwThreadId, DBG_CONTINUE);

		WaitForDebugEvent(&evt, INFINITE);

		if(evt.dwDebugEventCode==EXCEPTION_DEBUG_EVENT)
		{
			if(evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
				break; //exit loop - injected function finished

			if(evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) //for debug
			{
				CONTEXT ctx;
				ctx.ContextFlags = CONTEXT_FULL;
				GetThreadContext(m_Win32PInfo.hThread, &ctx);
				DumpContext(ctx);
				//single step is for debugging - keep it up if it was started
				P->SetSingleStep(true, &ctx);
				SetThreadContext(m_Win32PInfo.hThread, &ctx);
			}
			else
			{
				//not expecting an exception in the SysCallDll
				ktrace("unexpected exception from SysCallDll: 0x%8lx\n", evt.u.Exception.ExceptionRecord.ExceptionCode);
#ifdef _DEBUG
				CONTEXT ctx;
				ctx.ContextFlags = CONTEXT_FULL;
				GetThreadContext(m_Win32PInfo.hThread, &ctx);
				DumpContext(ctx);
				HandleException(evt);
#endif
			}
		}

		if(evt.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT)
		{
			m_dwExitCode = evt.u.ExitProcess.dwExitCode;
			m_bStillRunning = false;
			ExitThread(0); //no longer debugging
		}

		if(evt.dwDebugEventCode==OUTPUT_DEBUG_STRING_EVENT) //for debug info
		{
			int bufLen = evt.u.DebugString.nDebugStringLength*2;
			char * buf = new char[bufLen];

			ReadMemory(buf, (ADDR)evt.u.DebugString.lpDebugStringData, evt.u.DebugString.nDebugStringLength);
			buf[evt.u.DebugString.nDebugStringLength] = 0;

			ktrace("syscalldll relay: %*s", evt.u.DebugString.nDebugStringLength, buf);

			delete buf;
		}

		//ignore the rest
		//TODO is this ok?
	}

	//done calling it, stub is still in the function, 
	// but we can restore the stack and registers
	// first retreive any altered parameters

	addr = ParamAddr;

	ReadMemory(pStackData, addr, nStackDataSize);


	//only for debug - show exit point
	GetThreadContext(m_Win32PInfo.hThread, &InjectCtx);
	DWORD dwRet = InjectCtx.Eax;
	ktrace("Injection exit @ 0x%08lx\n", InjectCtx.Eip);


	//No need to set keow context back. We'll be resetting the context when we return back to HandleException()
	return dwRet;
}


// Output current process context
// and disassemble instructions
void Process::DumpContext(CONTEXT &ctx)
{
	BYTE buf[8];
	ReadMemory(buf, (ADDR)ctx.Eip, sizeof(buf));

	ktrace("0x%08lX %02X %02X %02X %02X  %02X %02X %02X %02X  %-12s   eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx eip=%08lx esp=%08lx ebp=%08lx flags=%08lx cs=%x ds=%x es=%x fs=%x gs=%x ss=%x\n",
		    ctx.Eip,buf[0],buf[1],buf[2],buf[3],
										 buf[4],buf[5],buf[6],buf[7],
															  "instr",
																	  ctx.Eax,  ctx.Ebx,  ctx.Ecx,  ctx.Edx,  ctx.Esi,  ctx.Edi,  ctx.Eip,  ctx.Esp,  ctx.Ebp,  ctx.EFlags, 
																																											ctx.SegCs,ctx.SegDs,ctx.SegEs,ctx.SegFs,ctx.SegGs,ctx.SegSs );
	//todo: disassemble
}

// Display a memory dump
void Process::DumpMemory(ADDR addr, DWORD len)
{
	const int BYTES_PER_LINE = 8;
	char hexbuf[5 * BYTES_PER_LINE + 1]; // "0x00 "... + null
	char charbuf[BYTES_PER_LINE + 1];    // "xxxxxxxx" + null
	int x;
	BYTE b;

	memset(charbuf, 0, sizeof(charbuf));

	ktrace("memory dump @ 0x%p, len %d\n", addr,len);
	x=0;
	for(DWORD i=0; i<len; ++i)
	{
		ReadMemory(&b, addr+x, 1); //byte-by-byte is slow but this is just a debug thing anyway
		StringCbPrintf(&hexbuf[x*5], sizeof(hexbuf)-(x*5), "0x%02x ", b);
		charbuf[x] = (isalnum(b)||ispunct(b)) ? b : '.';

		++x;
		if(x>=BYTES_PER_LINE)
		{
			ktrace("  %p: %s %s\n", addr, hexbuf, charbuf);
			addr+=BYTES_PER_LINE;
			x=0;
			memset(charbuf, 0, sizeof(charbuf));
		}
	}
	if(x>0)
		ktrace("  %p: %s %s\n", addr, hexbuf, charbuf);
}

//trace stack frames
void Process::DumpStackTrace(CONTEXT &ctx)
{
	//trace the stack back
	//assumes frame pointers exist
	ADDR esp = (ADDR)ctx.Esp;
	ADDR ebp = (ADDR)ctx.Ebp;
	ADDR retAddr = 0;
	while(esp>=(ADDR)ctx.Esp && esp<=m_KeowUserStackTop) //while data seems sensible
	{
		//edp is old stack pos
		esp=ebp;
		if(ebp==0)
			break;

		//stack is now ret addr
		esp+=4;
		ReadMemory(&retAddr, esp, sizeof(ADDR));

		//first pushed item was old ebp
		esp-=sizeof(ADDR);
		ReadMemory(&ebp, esp, sizeof(ADDR));


		ktrace(": from 0x%p\n", retAddr);
	}
}


// Find a free entry in the open files list
int Process::FindFreeFD()
{
	int fd;
	for(fd=0; fd<MAX_OPEN_FILES; ++fd)
		if(m_OpenFiles[fd] == NULL)
			return fd;

	return -1; //none
}

void Process::FreeResourcesBeforeExec()
{
	//unmap memory?
	MmapList::iterator mmap_it;
	for(mmap_it = m_MmapList.begin();
	    mmap_it != m_MmapList.end();
		++mmap_it)
	{
		MMapRecord * pMmap = *mmap_it;

		DebugBreak(); //TODO: handle cloning mmaps 
	}

	//close all files that we need to
	for(int i=0; i<MAX_OPEN_FILES; ++i)
	{
		if(m_OpenFiles[i]==NULL)
			continue;
		if(!m_OpenFiles[i]->GetInheritable())
		{
			m_OpenFiles[i]->Close();
			delete m_OpenFiles[i];
			m_OpenFiles[i] = NULL;
		}
	}

	//free other memory
	MemoryAllocationsList::iterator mem_it;
	for(mem_it = m_MemoryAllocations.begin();
	    mem_it != m_MemoryAllocations.end();
		++mem_it)
	{
		MemoryAlloc * pMemAlloc = *mem_it;

		//NOT the stack
		if(pMemAlloc->addr == m_KeowUserStackBase)
		{
			continue;
		}

		if(!VirtualFreeEx(m_Win32PInfo.hProcess, pMemAlloc->addr, pMemAlloc->len, MEM_DECOMMIT))
			ktrace("failed to free %p, len %d\n", pMemAlloc->addr, pMemAlloc->len);

		m_MemoryAllocations.erase(mem_it);
		//iterator is invalid now? reset?
		mem_it = m_MemoryAllocations.begin();
	}


	//signals are reset on exec
	InitSignalHandling();

	//Also reset the elf memory stuff
	memset(&m_ElfLoadData, 0, sizeof(m_ElfLoadData));
}

void Process::InitSignalHandling()
{
	int i;

	m_CurrentSignal = 0;
	m_KilledBySig = 0;

	memset(&m_PendingSignals, 0, sizeof(m_PendingSignals));

	memset(&m_SignalMask, 0, sizeof(m_SignalMask));

	m_SignalDepth = 0;

	//default signal handling
	for(i=0; i<_NSIG; ++i) {
		m_SignalAction[i].sa_handler = SIG_DFL;
		m_SignalAction[i].sa_flags = NULL;
		m_SignalAction[i].sa_restorer = NULL;
		ZeroMemory(&m_SignalAction[i].sa_mask, sizeof(m_SignalAction[i].sa_mask));
	}
}

bool Process::IsSuspended()
{
	//if either the process or it's kernel handler is suspended then true
	bool suspended = false;
	DWORD cnt;

	cnt = SuspendThread(m_Win32PInfo.hThread);
	if(cnt==-1)
	{
		//some error
	}
	else
	{
		suspended = (cnt>0);
		ResumeThread(m_Win32PInfo.hThread);
	}
	if(suspended)
		return suspended;

	cnt = SuspendThread(m_KernelThreadHandle);
	if(cnt==-1)
	{
		//some error
	}
	else
	{
		suspended = (cnt>0);
		ResumeThread(m_KernelThreadHandle);
	}
	return suspended;	
}
