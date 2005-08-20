// Process.cpp: implementation of the Process class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "Process.h"

#include "SysCalls.h"



//handler shouldn't need the default 1MB stack, try a smaller one
const int Process::KERNEL_PROCESS_THREAD_STACK_SIZE = 32*1024;

const char * Process::KEOW_PROCESS_STUB = "Keow.exe";

DWORD Process::ms_DummyStubParam = 0;

//////////////////////////////////////////////////////////////////////

Process::Process()
{
	int i;

	m_gid = m_uid = 0;
	m_Pid = m_ParentPid = 0;

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

}

Process::~Process()
{
	//remove from the kernel process map
	g_pKernelTable->m_Processes.erase(this);

	//close all handles
	CloseHandle(m_Win32PInfo.hProcess);
	CloseHandle(m_Win32PInfo.hThread);
}



//Create a new process using the given path to a program
//The pid is as assigned by the kernel
//The initial environment is as supplied
//
Process* Process::StartInit(PID pid, Path& path, char ** InitialEnvironment)
{
	/*
	launch win32 process stub
	load elf code into the process
	set stack/cpu contexts
	let process run
	*/
	Process * P = new Process();
	P->m_Pid = pid;
	P->m_ParentPid = 0;
	P->m_ProcessFileImage = path;
	P->m_UnixPwd = Path("/");

	//add to the kernel
	g_pKernelTable->m_Processes.push_back(P);


	//env and args
	P->m_Arguments = 0; //no args for init?
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
	g_pKernelThreadLocals = new KernelThreadLocals();
	g_pKernelThreadLocals->pProcess = (Process*)param;

	g_pKernelThreadLocals->pProcess->m_bInWin32Setup = true;

	//Need COM
	CoInitialize(NULL);//Ex(NULL, COINIT_MULTITHREADED);

	//Start the stub
	STARTUPINFO si;
	GetStartupInfo(&si);
	if(CreateProcess(NULL, (char*)KEOW_PROCESS_STUB, NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS, NULL, g_pKernelThreadLocals->pProcess->m_UnixPwd.GetWin32Path().c_str(), &si, &g_pKernelThreadLocals->pProcess->m_Win32PInfo) == FALSE)
	{
		//failed
		g_pKernelThreadLocals->pProcess->m_Win32PInfo.hProcess = NULL;
		SetEvent(g_pKernelThreadLocals->pProcess->m_hProcessStartEvent);
		return 0;
	}

	//started now
	SetEvent(g_pKernelThreadLocals->pProcess->m_hProcessStartEvent);


	//start the debugging stuff
	g_pKernelThreadLocals->pProcess->DebuggerLoop();
	ktrace("process handler debug loop exitted\n");


	//cleanup

	//delete thread stuff
	delete g_pKernelThreadLocals;

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
			if(evt.u.DebugString.fUnicode)
			{
				int bufLen = evt.u.DebugString.nDebugStringLength*2;
				char * buf = new char[bufLen];

				WideCharToMultiByte(CP_ACP, 0, (wchar_t*)evt.u.DebugString.lpDebugStringData, evt.u.DebugString.nDebugStringLength, buf, bufLen, 0, NULL);
				ktrace("relay: %*s\n", bufLen, buf);
			}
			else
			{
				ktrace("relay: %*s\n", evt.u.DebugString.nDebugStringLength, evt.u.DebugString.lpDebugStringData);
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
	//The stub provides this info in Eax prior to causing an initial breakpoint 
	m_bInWin32Setup = false;
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(m_Win32PInfo.hThread, &ctx);
	ReadMemory(&m_StubFunctionsInfo, (ADDR)ctx.Eax, sizeof(m_StubFunctionsInfo));

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
		ktrace("single step, eip 0x%08lx\n", ctx.Eip);
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
				SysCalls::HandleInt80SysCall(*this, ctx);

				//set any context changes
				SetThreadContext(m_Win32PInfo.hThread, &ctx);
			}
			else
			{
				ktrace("Access violation (not: int 80h)\n");
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
	char Interpreter[MAX_PATH] = "";
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
			protection = ConstantMapping::ElfProtectionToWin32Protection(phdr->p_flags);
			//phdr->p_memsz = (phdr->p_memsz + (phdr->p_align-1)) & (~(phdr->p_align-1)); //round up to alignment boundary
			pMem = MemoryHelper::AllocateMemAndProtectProcess(m_Win32PInfo.hProcess, pWantMem, phdr->p_memsz, PAGE_EXECUTE_READWRITE);
			if(pMem!=pWantMem)
			{
				DWORD err = GetLastError();
				ktrace("error 0x%lx in load program segment @ 0x%lx (got 0x%lx)\n", err,pWantMem, pMem);
				loadok = 0;
				break;
			}
			//need to load into our memory, then copy to process
			//seems you cannot write across a 4k boundary, so write in blocks
			if(phdr->p_filesz != 0)
			{
				//int offset, size, write;

				pMemTemp = (ADDR)realloc(pMemTemp, phdr->p_filesz);
				SetFilePointer(hImg, phdr->p_offset, 0, FILE_BEGIN);
				ReadFile(hImg, pMemTemp, phdr->p_filesz, &dwRead, 0);

				if(!WriteMemory(pMem, phdr->p_filesz, pMemTemp))
				{
					DWORD err = GetLastError();
					ktrace("error 0x%lx in write program segment\n", err);
					//loadok = 0;
					//break;
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
			ReadFile(hImg, Interpreter, phdr->p_filesz, &dwRead, 0);
		}
	}

	//align brk to a page boundary
	//NO!! //pElf->brk = (void*)( ((DWORD)pElf->brk + (SIZE4k-1)) & (~(SIZE4k-1)) ); 

	//load interpreter?
	if(Interpreter[0]!=0)
	{
		if(strcmp(Interpreter,"/usr/lib/libc.so.1") == 0
		|| strcmp(Interpreter,"/usr/lib/ld.so.1") == 0 )
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

		Path InterpPath(Interpreter);
		HANDLE hInterpImg = CreateFile(InterpPath.GetWin32Path().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
		if(hInterpImg==INVALID_HANDLE_VALUE)
		{
			ktrace("failed to load interpreter: %s\n", InterpPath.GetUnixPath().c_str());
			loadok = 0;
		}
		else
		{
			struct linux::elf32_hdr hdr2;
			ReadFile(hInterpImg, &hdr2, sizeof(hdr2), &dwRead, 0);

			loadok = LoadElfImage(hInterpImg, &hdr2, &LoadData2, true);

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

	//stack data needed
	const int AUX_RESERVE = 2*sizeof(DWORD)*20; //heaps for what is below
	int stack_needed = sizeof(ADDR)*(EnvCnt+ArgCnt) + AUX_RESERVE + sizeof(ADDR)/*end marker*/;

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(m_Win32PInfo.hThread, &ctx);

	ADDR StackTop = (ADDR)ctx.Esp;

	//stack grows DOWN
	ctx.Esp -= stack_needed;

	//copy data to the stack
	ADDR addr = (ADDR)ctx.Esp;

	//argc
	WriteMemory(addr, sizeof(DWORD), &ArgCnt);
	addr += sizeof(DWORD);
	//argv[]: clone the array of pointers
	MemoryHelper::TransferMemory(m_Win32PInfo.hProcess, pArgs, m_Win32PInfo.hProcess, addr, (ArgCnt+1)*sizeof(ADDR));
	addr += (ArgCnt+1)*sizeof(ADDR);

	//envp[]: clone the array of pointers
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
	ctx.Eip = (DWORD)(m_ElfLoadData.interpreter_start ? m_ElfLoadData.interpreter_start : m_ElfLoadData.start_addr);

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
	return 0==MemoryHelper::ReadMemory((ADDR)pBuf, m_Win32PInfo.hProcess, addr, len);
}

bool Process::WriteMemory(ADDR addr, DWORD len, LPVOID pBuf)
{
	return 0==MemoryHelper::WriteMemory(m_Win32PInfo.hProcess, addr, len, (ADDR)pBuf);
}


void Process::SendSignal(int sig)
{
	//send the signal to this process
	m_PendingSignals[sig] = true;

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
		{
			DWORD s = -sig;
			InvokeStubFunction(m_StubFunctionsInfo.ExitProcess, s);
		}
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
			{
				DWORD s = -sig;
				InvokeStubFunction(m_StubFunctionsInfo.ExitProcess, s);
			}
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
			{
				DWORD s = -sig;
				InvokeStubFunction(m_StubFunctionsInfo.ExitProcess, s);
			}
			break;

		default:
			ktrace("IMPLEMENT default action for signal %d\n", sig);
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
			{
				DWORD s = -sig;
				InvokeStubFunction(m_StubFunctionsInfo.ExitProcess, s);
			}
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


void Process::InvokeStubFunction(StubFunc func, DWORD &param1, DWORD &param2, DWORD &param3, DWORD &param4)
{
	//StubFunc is declared as _stdcall calling convention.
	//This means we place args on the stack so that param1 pops first
	//also we don't need to clean up the stack. The called function does

	CONTEXT OrigCtx;
	OrigCtx.ContextFlags = CONTEXT_FULL; //preserve everything
	GetThreadContext(m_Win32PInfo.hThread, &OrigCtx);

	CONTEXT TempCtx = OrigCtx;

	TempCtx.Eip = (DWORD)func;				//call this in the stub
	TempCtx.Esp -= sizeof(DWORD)*5; //4 params and the return address

	SetThreadContext(m_Win32PInfo.hThread, &TempCtx);


	ADDR addr = (ADDR)TempCtx.Esp;
	ADDR ParamAddr = addr;

	WriteMemory(addr, sizeof(DWORD), &param1);
	addr += sizeof(DWORD);
	WriteMemory(addr, sizeof(DWORD), &param2);
	addr += sizeof(DWORD);
	WriteMemory(addr, sizeof(DWORD), &param3);
	addr += sizeof(DWORD);
	WriteMemory(addr, sizeof(DWORD), &param4);
	addr += sizeof(DWORD);

	WriteMemory(addr, sizeof(DWORD), &OrigCtx.Eip); //return address



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
				ktrace("single step, eip 0x%08lx\n", ctx.Eip);
			}
		}

		if(evt.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT)
		{
			m_dwExitCode = evt.u.ExitProcess.dwExitCode;
			ExitThread(0); //no longer debugging
		}

		//ignore the rest
		//TODO is this ok?
	}

	//done calling it, stub is still in the function, 
	// but we can restore the stack and registers
	// first retreive any altered parameters


	addr = ParamAddr;

	ReadMemory(&param1, addr, sizeof(DWORD));
	addr += sizeof(DWORD);
	ReadMemory(&param2, addr, sizeof(DWORD));
	addr += sizeof(DWORD);
	ReadMemory(&param3, addr, sizeof(DWORD));
	addr += sizeof(DWORD);
	ReadMemory(&param4, addr, sizeof(DWORD));

	SetThreadContext(m_Win32PInfo.hThread, &OrigCtx);
}

void Process::GenerateCoreDump()
{
	DebugBreak();
}