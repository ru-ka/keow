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
	int i;

	m_gid = m_uid = 0;
	m_egid = m_euid = 0;
	m_saved_uid = m_saved_gid = 0;

	m_Pid = m_ParentPid = 0;
	m_ProcessGroupPID = 0;

	m_Environment = m_Arguments = NULL;

	memset(&m_Win32PInfo, 0, sizeof(m_Win32PInfo));
	memset(&m_ElfLoadData, 0, sizeof(m_ElfLoadData));

	memset(&m_ptrace, 0, sizeof(m_ptrace));

	memset(&m_OpenFiles, 0, sizeof(m_OpenFiles));

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

	m_dwExitCode = -SIGABRT; //in case not set later

	m_hWaitTerminatingEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	memset(&SysCallAddr, 0, sizeof(SysCallAddr));
}

Process::~Process()
{
	//remove from the kernel process map
	g_pKernelTable->m_Processes.erase(this);

	//close all handles
	CloseHandle(m_Win32PInfo.hProcess);
	CloseHandle(m_Win32PInfo.hThread);

	CloseHandle(m_hWaitTerminatingEvent);
}



//Create a new process using the given path to a program
//The pid is as assigned by the kernel
//The initial environment is as supplied
//
Process* Process::StartInit(PID pid, Path& path, char ** InitialArguments, char ** InitialEnvironment)
{
	/*
	launch win32 process stub
	load elf code into the process
	set stack/cpu contexts
	let process run
	*/
	P = new Process();
	P->m_Pid = pid;
	P->m_ParentPid = 0;
	P->m_ProcessFileImage = path;
	P->m_UnixPwd = Path("/");

	//add to the kernel
	g_pKernelTable->m_Processes.push_back(P);


	//env and args
	P->m_Arguments = (ADDR)InitialArguments;
	P->m_Environment = (ADDR)InitialEnvironment;

	P->m_bDoExec = P->m_bDoFork = false;

	//event to co-ordinate starting
	P->m_hProcessStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//need to start the process and debug it (to trap kernel calls etc)
	//the debugger thread
	HANDLE hThread = CreateThread(NULL, KERNEL_PROCESS_THREAD_STACK_SIZE, KernelProcessHandlerMain, P, 0, &P->m_KernelThreadId);
	if(hThread==NULL)
	{
		ktrace("failed to start debug thread for process\n");
		delete P;
		return NULL;
	}
	CloseHandle(hThread); //don't need it, just the id

	//Wait for the thread to start the process, or die
	DWORD dwRet = WaitForSingleObject(P->m_hProcessStartEvent, INFINITE);
	CloseHandle(P->m_hProcessStartEvent);
	//check it's ok
	if(P->m_Win32PInfo.hProcess==NULL)
	{
		ktrace("failed to start the process\n");
		delete P;
		return NULL;
	}

	//running, this is it
	return P;
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


	//start the debugging stuff
	P->DebuggerLoop();
	ktrace("process handler debug loop exitted\n");


	//cleanup

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
				HandleSignal(sig);
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

				ktrace("relay: %*s\n", evt.u.DebugString.nDebugStringLength, buf);

				delete buf;
			}
			break;

		default:
			ktrace("ignoring unhandled debug event code %ld\n", evt.dwDebugEventCode);
			break;
		}


		//continue running
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
	ctx.EFlags &= ~(0x100L); //8th bit is trap flag
	SetThreadContext(m_Win32PInfo.hThread, &ctx);


	if(m_bDoExec)
	{
		Process * pParent = g_pKernelTable->FindProcess(m_ParentPid);
		CopyProcessHandles(pParent);
		LoadImage(m_ProcessFileImage, false);
		return;
	}
	if(m_bDoFork)
	{
		Process * pParent = g_pKernelTable->FindProcess(m_ParentPid);
		ForkCopyOtherProcess(pParent);
		return;
	}


	//not fork or exec, must be 'init', the very first process
	LoadImage(m_ProcessFileImage, false);

	//std in,out,err
	m_OpenFiles[0] = new IOHNtConsole(g_pKernelTable->m_pMainConsole);
	m_OpenFiles[1] = new IOHNtConsole(g_pKernelTable->m_pMainConsole);
	m_OpenFiles[2] = new IOHNtConsole(g_pKernelTable->m_pMainConsole);

}


//This is either a fault to signal to the process,
//or it's a kernel call to intercept
//
void Process::HandleException(DEBUG_EVENT &evt)
{
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(m_Win32PInfo.hThread, &ctx);

	switch(evt.u.Exception.ExceptionRecord.ExceptionCode)
	{
	case EXCEPTION_SINGLE_STEP:
		TraceContext(0,0,ctx);
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
				CONTEXT ctx;
				ctx.ContextFlags = CONTEXT_FULL;
				GetThreadContext(m_Win32PInfo.hThread, &ctx);
				ctx.Eip += 2;
				SetThreadContext(m_Win32PInfo.hThread, &ctx);

				//handle int 80h
				SysCalls::HandleInt80SysCall(ctx);

				//set any context changes
				SetThreadContext(m_Win32PInfo.hThread, &ctx);
			}
			else
			{
				ktrace("Access violation @ 0x%08lx\n", evt.u.Exception.ExceptionRecord.ExceptionAddress);
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
}

void Process::CopyProcessHandles(Process *pParent)
{
	DebugBreak();
}

void Process::ForkCopyOtherProcess(Process *pOther)
{
	DebugBreak();
}


//Load the process executable images
//and then set the process to run
//
DWORD Process::LoadImage(Path img, bool LoadAsLibrary)
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

			rc = LoadImage(interp, LoadAsLibrary);

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
	ADDR pMem, pWantMem;
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

		if(phdr->p_type == PT_LOAD
		|| phdr->p_type == PT_PHDR)
		{
			//load segment into memory
			pWantMem = phdr->p_vaddr + pBaseAddr;
			protection = ElfProtectionToWin32Protection(phdr->p_flags);
			//phdr->p_memsz = (phdr->p_memsz + (phdr->p_align-1)) & (~(phdr->p_align-1)); //round up to alignment boundary
			pMem = MemoryHelper::AllocateMemAndProtect(pWantMem, phdr->p_memsz, PAGE_EXECUTE_READWRITE);
			if(pMem!=pWantMem)
			{
				DWORD err = GetLastError();
				ktrace("error 0x%lx in load program segment @ 0x%lx (got 0x%lx)\n", err,pWantMem, pMem);
				loadok = 0;
				break;
			}
			//need to load into our memory, then copy to process
			if(phdr->p_filesz != 0)
			{
				pMemTemp = (ADDR)realloc(pMemTemp, phdr->p_filesz);
				SetFilePointer(hImg, phdr->p_offset, 0, FILE_BEGIN);
				ReadFile(hImg, pMemTemp, phdr->p_filesz, &dwRead, 0);

				if(!WriteMemory(pMem, phdr->p_filesz, pMemTemp))
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
				if(pMem+phdr->p_memsz  > pElfLoadData->brk)
					pElfLoadData->brk = pMem + phdr->p_memsz;

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

			//need program header location for possible interpreter
			if(phdr->p_type == PT_PHDR)
			{
				pElfLoadData->phdr_addr = pMem;

				pElfLoadData->phdr_phnum = pElfHdr->e_phnum;
				pElfLoadData->phdr_phent = pElfHdr->e_phentsize;
			}
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

	ADDR pEnv, pArgs;
	DWORD EnvCnt, ArgCnt;
	if(m_ParentPid==0)
	{
		pEnv = MemoryHelper::CopyStringListBetweenProcesses(GetCurrentProcess(), m_Environment, m_Win32PInfo.hProcess, &EnvCnt);
		pArgs = MemoryHelper::CopyStringListBetweenProcesses(GetCurrentProcess(), m_Arguments, m_Win32PInfo.hProcess, &ArgCnt);
	}
	else
	{
		Process * pParent = g_pKernelTable->FindProcess(m_ParentPid);
		pEnv = MemoryHelper::CopyStringListBetweenProcesses(pParent->m_Win32PInfo.hProcess, m_Environment, m_Win32PInfo.hProcess, &EnvCnt);
		pArgs = MemoryHelper::CopyStringListBetweenProcesses(pParent->m_Win32PInfo.hProcess, m_Arguments, m_Win32PInfo.hProcess, &ArgCnt);
	}

	//need an interpreter?
	ADDR InterpAddr = 0;
	if(m_ElfLoadData.Interpreter[0]!=0)
	{
//		++ArgCnt; //interpreter comes first
//		int len = strlen(m_ElfLoadData.Interpreter) + 1; //include the null
//		InterpAddr = MemoryHelper::AllocateMemAndProtect(0, len, PAGE_EXECUTE_READWRITE);
//		WriteMemory(InterpAddr, len, m_ElfLoadData.Interpreter);
	}

	//stack data needed
	const int AUX_RESERVE = 2*sizeof(DWORD)*20; //heaps for what is below
	int stack_needed = sizeof(ADDR)*(EnvCnt+1+ArgCnt+1) + AUX_RESERVE + sizeof(ADDR)/*end marker*/;

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
	WriteMemory(addr, sizeof(DWORD), &ArgCnt);
	addr += sizeof(DWORD);
	//additional first arg?
	if(InterpAddr)
	{
		WriteMemory(addr, sizeof(DWORD), &InterpAddr);
		addr += sizeof(DWORD);
		--ArgCnt; //so next bit sees the original size
	}
	//argv[]: clone the array of pointers that are already in the target process
	//include the null end entry
	MemoryHelper::TransferMemory(m_Win32PInfo.hProcess, pArgs, m_Win32PInfo.hProcess, addr, (ArgCnt+1)*sizeof(ADDR));
	addr += (ArgCnt+1)*sizeof(ADDR);

	//envp[]: clone the array of pointers that are already in the target process
	//include the null end entry
	MemoryHelper::TransferMemory(m_Win32PInfo.hProcess, pEnv, m_Win32PInfo.hProcess, addr, (EnvCnt+1)*sizeof(ADDR));
	addr += (EnvCnt+1)*sizeof(ADDR);

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

	PUSH_AUX_VAL(AT_ENTRY,	(DWORD)m_ElfLoadData.start_addr) //real start - ignoring 'interpreter'
	PUSH_AUX_VAL(AT_BASE,	(DWORD)m_ElfLoadData.interpreter_base)
	PUSH_AUX_VAL(AT_PHNUM,	m_ElfLoadData.phdr_phnum)
	PUSH_AUX_VAL(AT_PHENT,	m_ElfLoadData.phdr_phent)
	PUSH_AUX_VAL(AT_PHDR,	(DWORD)m_ElfLoadData.phdr_addr)
	PUSH_AUX_VAL(AT_PAGESZ,	0x1000) //4K
	PUSH_AUX_VAL(AT_NULL,	0)

#undef PUSH_AUX_VAL

	//end marker (NULL)
	ArgCnt=0;
	WriteMemory(addr, sizeof(DWORD), &ArgCnt);
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

	SetSingleStep(true); //DEBUG - keep it up

	return 0;
}

void Process::SetSingleStep(bool set)
{
	CONTEXT ctx;

	ctx.ContextFlags = CONTEXT_CONTROL;
	GetThreadContext(m_Win32PInfo.hThread, &ctx);

	const DWORD ss_bit = 0x100L; //8th bit is trap flag

	if(set)
	{
		//enable single-step 
		ctx.EFlags |= ss_bit; 
	}
	else
	{
		//disable single-step 
		ctx.EFlags &= ~ss_bit;
	}

	SetThreadContext(m_Win32PInfo.hThread, &ctx);
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
	//send the signal to this process
	m_PendingSignals[sig] = true;
	SetEvent(m_hWaitTerminatingEvent);

	//NOTE: this may be executing on a different thread than the one handling this process

	//because it may not be in the debugger
	SuspendThread(m_Win32PInfo.hThread);

	//make single-step
	//this forces the debugger to intervene and then it can see the pending signals
	SetSingleStep(true);

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
			SysCallDll::exit(-sig);
			break;

		default:
			ktrace("IMPLEMENT default action for signal %d\n", sig);
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
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

			DebugBreak();//TODO: inject into process
			((void (_cdecl *)(int, linux::siginfo *, void *))handler)(sig, &si, /*&ct*/0);
		}
		else
		{
			DebugBreak();//TODO: inject into process
			((void (_cdecl *)(int))handler)(sig);
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
	ktrace("Core Dump\n");
}

DWORD Process::InjectFunctionCall(void *func, void *pStackData, int nStackDataSize)
{
	//Expect that the func is declared as _stdcall calling convention.
	//(actually it ought to be from the SysCallDll::RemoteAddrInfo) 
	//This means we place args on the stack so that param1 pops first
	//also we don't need to clean up the stack. The called function does.

	ktrace("Injecting call to SysCallDll func @ 0x%08lx\n", func);

	CONTEXT OrigCtx;
	OrigCtx.ContextFlags = CONTEXT_FULL; //preserve everything
	GetThreadContext(m_Win32PInfo.hThread, &OrigCtx);

	CONTEXT TempCtx = OrigCtx;

	TempCtx.Eip = (DWORD)func;		//call this in the stub
	TempCtx.Esp -= sizeof(ADDR);	//the return address
	TempCtx.Esp -= nStackDataSize;	//the params

	SetThreadContext(m_Win32PInfo.hThread, &TempCtx);


	ADDR addr = (ADDR)TempCtx.Esp;

	//return address
	WriteMemory(addr, sizeof(DWORD), &OrigCtx.Eip);
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

		ContinueDebugEvent(m_Win32PInfo.dwProcessId, m_Win32PInfo.dwThreadId, DBG_CONTINUE);

		WaitForDebugEvent(&evt, INFINITE);

		if(evt.dwDebugEventCode==EXCEPTION_DEBUG_EVENT)
		{
			if(evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
				break; //the for()

			if(evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP)
			{
				CONTEXT ctx;
				ctx.ContextFlags = CONTEXT_FULL; //preserve everything
				GetThreadContext(m_Win32PInfo.hThread, &ctx);
				TraceContext(0,0, ctx);
			}
			else
			{
				//not expecting an exception in the SysCallDll
				ktrace("unexpected exception from SysCallDll: 0x%8lx\n", evt.u.Exception.ExceptionRecord.ExceptionCode);
#ifdef _DEBUG
				CONTEXT ctx;
				ctx.ContextFlags = CONTEXT_FULL;
				GetThreadContext(m_Win32PInfo.hThread, &ctx);
				TraceContext(0,0, ctx);
				HandleException(evt);
#endif
			}
		}

		if(evt.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT)
		{
			m_dwExitCode = evt.u.ExitProcess.dwExitCode;
			ExitThread(0); //no longer debugging
		}

		if(evt.dwDebugEventCode==OUTPUT_DEBUG_STRING_EVENT)
		{
			int bufLen = evt.u.DebugString.nDebugStringLength*2;
			char * buf = new char[bufLen];

			ReadMemory(buf, (ADDR)evt.u.DebugString.lpDebugStringData, evt.u.DebugString.nDebugStringLength);
			buf[evt.u.DebugString.nDebugStringLength] = 0;

			ktrace("syscalldll relay: %*s\n", evt.u.DebugString.nDebugStringLength, buf);

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

	GetThreadContext(m_Win32PInfo.hThread, &TempCtx);
	DWORD dwRet = TempCtx.Eax;
	ktrace("Injection exit @ 0x%08lx\n", TempCtx.Eip);

	//Eax will be set (if required) by the system call handler that requested the injection
	SetThreadContext(m_Win32PInfo.hThread, &OrigCtx);

	return dwRet;
}


// Output current process context
// and disassemble instructions
void Process::TraceContext(int LinesBefore, int LinesAfter, CONTEXT &ctx)
{
	BYTE buf[4];
	ReadMemory(buf, (ADDR)ctx.Eip, sizeof(buf));

	ktrace("0x%08lX %02X %02X %02X %02X  %-12s   eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx eip=%08lx esp=%08lx ebp=%08lx flags=%08lx \n",
		    ctx.Eip,buf[0],buf[1],buf[2],buf[3],
			                             "instr",
												 ctx.Eax,  ctx.Ebx,  ctx.Ecx,  ctx.Edx,  ctx.Esi,  ctx.Edi,  ctx.Eip,  ctx.Esp,  ctx.Ebp,  ctx.EFlags);

	/*
	ktrace("0x%08lX %02X %02X %02X %02X  %-12s\n", ctx.Eip, buf[0],buf[1],buf[2],buf[3], "instr");

	ktrace("eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx eip=%08lx esp=%08lx ebp=%08lx flags=%08lx \n",
			ctx.Eax,  ctx.Ebx,  ctx.Ecx,  ctx.Edx,  ctx.Esi,  ctx.Edi,  ctx.Eip,  ctx.Esp,  ctx.Ebp,  ctx.EFlags);
			*/
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
