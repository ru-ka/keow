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


#include "kernel.h"
#include "forkexec.h"
#include "tib.h"
#include <excpt.h>

ProcessDataStruct* pProcessData = NULL;


SYSCALL_HANDLER syscall_handlers[NR_syscalls];
const char* syscall_names[NR_syscalls];


//shared kernel data
struct KernelSharedDataStruct *pKernelSharedData = NULL;
HANDLE hKernelLock;

HANDLE WaitTerminatingEvent;
static char * WaitTerminatingEventName;


//fork/exec helper
static bool ktrace_should_append = false;

//unix time base
ULARGE_INTEGER Time1Jan1970;


DWORD WINAPI SignalThreadMain(LPVOID param);



void kernel_init()
{
	int i;
 
	// A file time is a 64-bit value that represents the number of 100-nanosecond intervals 
	// that have elapsed since 12:00 A.M. January 1, 1601 (UTC).
	// A unix time_t is time since the Epoch (00:00:00 UTC, January 1, 1970), in seconds.
	// Need a base time for calculations
	SYSTEMTIME st;
	FILETIME ft, ft2;
	st.wDayOfWeek = 0;
	st.wDay = 1;
	st.wMonth = 1;
	st.wYear = 1970;
	st.wHour = 0;
	st.wMinute = 0;
	st.wSecond = 0;
	st.wMilliseconds = 0;
	SystemTimeToFileTime(&st, &ft);
	st.wSecond = 1;
	SystemTimeToFileTime(&st, &ft2);
	Time1Jan1970.LowPart = ft.dwLowDateTime;
	Time1Jan1970.HighPart = ft.dwHighDateTime;
	//FileTimeToSystemTime(&fi.ftLastWriteTime, &st);


	//default all syscalls to unhandled
	for(i=0; i<NR_syscalls; ++i)
		syscall_handlers[i] = sys_unhandled;

	//
	// these come from include linux/asm/unistd.h
	//
#define DEF_SYSCALL(n) \
		syscall_handlers[__NR_##n]=sys_##n; \
		syscall_names[__NR_##n]=#n;

	DEF_SYSCALL(exit);
	DEF_SYSCALL(fork);
	DEF_SYSCALL(read);
	DEF_SYSCALL(write);
	DEF_SYSCALL(open);
	DEF_SYSCALL(close);
	DEF_SYSCALL(waitpid);
	DEF_SYSCALL(creat);
	DEF_SYSCALL(link);
	DEF_SYSCALL(unlink);
	DEF_SYSCALL(execve);
	DEF_SYSCALL(chdir);
	DEF_SYSCALL(time);
	DEF_SYSCALL(mknod);
	DEF_SYSCALL(chmod);
	DEF_SYSCALL(lchown);
	DEF_SYSCALL(lseek);
	DEF_SYSCALL(getpid);
	DEF_SYSCALL(mount);
	DEF_SYSCALL(umount);
	DEF_SYSCALL(setuid);
	DEF_SYSCALL(getuid);
	DEF_SYSCALL(stime);
	DEF_SYSCALL(ptrace);
	DEF_SYSCALL(alarm);
	DEF_SYSCALL(oldfstat);
	DEF_SYSCALL(pause);
	DEF_SYSCALL(utime);
	DEF_SYSCALL(stty);
	DEF_SYSCALL(gtty);
	DEF_SYSCALL(access);
	DEF_SYSCALL(nice);
	DEF_SYSCALL(ftime);
	DEF_SYSCALL(sync);
	DEF_SYSCALL(kill);
	DEF_SYSCALL(rename);
	DEF_SYSCALL(mkdir);
	DEF_SYSCALL(rmdir);
	DEF_SYSCALL(dup);
	DEF_SYSCALL(pipe);
	DEF_SYSCALL(times);
	DEF_SYSCALL(prof);
	DEF_SYSCALL(brk);
	DEF_SYSCALL(setgid);
	DEF_SYSCALL(getgid);
	DEF_SYSCALL(signal);
	DEF_SYSCALL(geteuid);
	DEF_SYSCALL(getegid);
	DEF_SYSCALL(acct);
	DEF_SYSCALL(umount2);
	DEF_SYSCALL(lock);
	DEF_SYSCALL(ioctl);
	DEF_SYSCALL(fcntl);
	DEF_SYSCALL(mpx);
	DEF_SYSCALL(setpgid);
	DEF_SYSCALL(ulimit);
	DEF_SYSCALL(oldolduname);
	DEF_SYSCALL(umask);
	DEF_SYSCALL(chroot);
	DEF_SYSCALL(ustat);
	DEF_SYSCALL(dup2);
	DEF_SYSCALL(getppid);
	DEF_SYSCALL(getpgrp);
	DEF_SYSCALL(setsid);
	DEF_SYSCALL(sigaction);
	DEF_SYSCALL(sgetmask);
	DEF_SYSCALL(ssetmask);
	DEF_SYSCALL(setreuid);
	DEF_SYSCALL(setregid);
	DEF_SYSCALL(sigsuspend);
	DEF_SYSCALL(sigpending);
	DEF_SYSCALL(sethostname);
	DEF_SYSCALL(setrlimit);
	DEF_SYSCALL(getrlimit);	/* Back compatible);Gig limited rlimit */
	DEF_SYSCALL(getrusage);
	DEF_SYSCALL(gettimeofday);
	DEF_SYSCALL(settimeofday);
	DEF_SYSCALL(getgroups);
	DEF_SYSCALL(setgroups);
	DEF_SYSCALL(select);
	DEF_SYSCALL(symlink);
	DEF_SYSCALL(oldlstat);
	DEF_SYSCALL(readlink);
	DEF_SYSCALL(uselib);
	DEF_SYSCALL(swapon);
	DEF_SYSCALL(reboot);
	DEF_SYSCALL(readdir);
	DEF_SYSCALL(mmap);
	DEF_SYSCALL(munmap);
	DEF_SYSCALL(truncate);
	DEF_SYSCALL(ftruncate);
	DEF_SYSCALL(fchmod);
	DEF_SYSCALL(fchown);
	DEF_SYSCALL(getpriority);
	DEF_SYSCALL(setpriority);
	DEF_SYSCALL(profil);
	DEF_SYSCALL(statfs);
	DEF_SYSCALL(fstatfs);
	DEF_SYSCALL(ioperm);
	DEF_SYSCALL(socketcall);
	DEF_SYSCALL(syslog);
	DEF_SYSCALL(setitimer);
	DEF_SYSCALL(getitimer);
	DEF_SYSCALL(stat);
	DEF_SYSCALL(lstat);
	DEF_SYSCALL(fstat);
	DEF_SYSCALL(olduname);
	DEF_SYSCALL(iopl);
	DEF_SYSCALL(vhangup);
	DEF_SYSCALL(idle);
	DEF_SYSCALL(vm86old);
	DEF_SYSCALL(wait4);
	DEF_SYSCALL(swapoff);
	DEF_SYSCALL(sysinfo);
	DEF_SYSCALL(ipc);
	DEF_SYSCALL(fsync);
	DEF_SYSCALL(sigreturn);
	DEF_SYSCALL(clone);
	DEF_SYSCALL(setdomainname);
	DEF_SYSCALL(uname);
	DEF_SYSCALL(modify_ldt);
	DEF_SYSCALL(adjtimex);
	DEF_SYSCALL(mprotect);
	DEF_SYSCALL(sigprocmask);
	DEF_SYSCALL(create_module);
	DEF_SYSCALL(init_module);
	DEF_SYSCALL(delete_module);
	DEF_SYSCALL(get_kernel_syms);
	DEF_SYSCALL(quotactl);
	DEF_SYSCALL(getpgid);
	DEF_SYSCALL(fchdir);
	DEF_SYSCALL(bdflush);
	DEF_SYSCALL(sysfs);
	DEF_SYSCALL(personality);
	DEF_SYSCALL(afs_syscall); /* Syscall for Andrew File System */
	DEF_SYSCALL(setfsuid);
	DEF_SYSCALL(setfsgid);
	DEF_SYSCALL(_llseek);
	DEF_SYSCALL(getdents);
	DEF_SYSCALL(_newselect);
	DEF_SYSCALL(flock);
	DEF_SYSCALL(msync);
	DEF_SYSCALL(readv);
	DEF_SYSCALL(writev);
	DEF_SYSCALL(getsid);
	DEF_SYSCALL(fdatasync);
	DEF_SYSCALL(_sysctl);
	DEF_SYSCALL(mlock);
	DEF_SYSCALL(munlock);
	DEF_SYSCALL(mlockall);
	DEF_SYSCALL(munlockall);
	DEF_SYSCALL(sched_setparam);
	DEF_SYSCALL(sched_getparam);
	DEF_SYSCALL(sched_setscheduler);
	DEF_SYSCALL(sched_getscheduler);
	DEF_SYSCALL(sched_yield);
	DEF_SYSCALL(sched_get_priority_max);
	DEF_SYSCALL(sched_get_priority_min);
	DEF_SYSCALL(sched_rr_get_interval);
	DEF_SYSCALL(nanosleep);
	DEF_SYSCALL(mremap);
	DEF_SYSCALL(setresuid);
	DEF_SYSCALL(getresuid);
	DEF_SYSCALL(vm86);
	DEF_SYSCALL(query_module);
	DEF_SYSCALL(poll);
	DEF_SYSCALL(nfsservctl);
	DEF_SYSCALL(setresgid);
	DEF_SYSCALL(getresgid);
	DEF_SYSCALL(prctl);
	DEF_SYSCALL(rt_sigreturn);
	DEF_SYSCALL(rt_sigaction);
	DEF_SYSCALL(rt_sigprocmask);
	DEF_SYSCALL(rt_sigpending);
	DEF_SYSCALL(rt_sigtimedwait);
	DEF_SYSCALL(rt_sigqueueinfo);
	DEF_SYSCALL(rt_sigsuspend);
	DEF_SYSCALL(pread);
	DEF_SYSCALL(pwrite);
	DEF_SYSCALL(chown);
	DEF_SYSCALL(getcwd);
	DEF_SYSCALL(capget);
	DEF_SYSCALL(capset);
	DEF_SYSCALL(sigaltstack);
	DEF_SYSCALL(sendfile);
	DEF_SYSCALL(getpmsg);	/* some people actually want streams */
	DEF_SYSCALL(putpmsg);	/* some people actually want streams */
	DEF_SYSCALL(vfork);
	DEF_SYSCALL(ugetrlimit);	/* SuS compliant getrlimit */
	DEF_SYSCALL(mmap2);
	DEF_SYSCALL(truncate64);
	DEF_SYSCALL(ftruncate64);
	DEF_SYSCALL(stat64);
	DEF_SYSCALL(lstat64);
	DEF_SYSCALL(fstat64);
	DEF_SYSCALL(lchown32);
	DEF_SYSCALL(getuid32);
	DEF_SYSCALL(getgid32);
	DEF_SYSCALL(geteuid32);
	DEF_SYSCALL(getegid32);
	DEF_SYSCALL(setreuid32);
	DEF_SYSCALL(setregid32);
	DEF_SYSCALL(getgroups32);
	DEF_SYSCALL(setgroups32);
	DEF_SYSCALL(fchown32);
	DEF_SYSCALL(setresuid32);
	DEF_SYSCALL(getresuid32);
	DEF_SYSCALL(setresgid32);
	DEF_SYSCALL(getresgid32);
	DEF_SYSCALL(chown32);
	DEF_SYSCALL(setuid32);
	DEF_SYSCALL(setgid32);
	DEF_SYSCALL(setfsuid32);
	DEF_SYSCALL(setfsgid32);
	DEF_SYSCALL(pivot_root);
	DEF_SYSCALL(mincore);
	DEF_SYSCALL(madvise);
	DEF_SYSCALL(madvise1);	/* delete when C lib stub is removed */
	DEF_SYSCALL(getdents64);
	DEF_SYSCALL(fcntl64);
	DEF_SYSCALL(security);	/* syscall for security modules */
	DEF_SYSCALL(gettid);
	DEF_SYSCALL(readahead);
	DEF_SYSCALL(setxattr);
	DEF_SYSCALL(lsetxattr);
	DEF_SYSCALL(fsetxattr);
	DEF_SYSCALL(getxattr);
	DEF_SYSCALL(lgetxattr);
	DEF_SYSCALL(fgetxattr);
	DEF_SYSCALL(listxattr);
	DEF_SYSCALL(llistxattr);
	DEF_SYSCALL(flistxattr);
	DEF_SYSCALL(removexattr);
	DEF_SYSCALL(lremovexattr);
	DEF_SYSCALL(fremovexattr);
	DEF_SYSCALL(tkill);
	DEF_SYSCALL(sendfile64);
	DEF_SYSCALL(futex);
	DEF_SYSCALL(sched_setaffinity);
	DEF_SYSCALL(sched_getaffinity);
	DEF_SYSCALL(set_thread_area);
	DEF_SYSCALL(get_thread_area);
	DEF_SYSCALL(io_setup);
	DEF_SYSCALL(io_destroy);
	DEF_SYSCALL(io_getevents);
	DEF_SYSCALL(io_submit);
	DEF_SYSCALL(io_cancel);
	DEF_SYSCALL(alloc_hugepages);
	DEF_SYSCALL(free_hugepages);
	DEF_SYSCALL(exit_group);

}


void kernel_term()
{
	if(pProcessData)
	{
		//not part of the exec process (pid sharing)? this is us right?
		if(pProcessData->Win32PID == GetCurrentProcessId())
		{
			pProcessData->in_use = false;

			//parent should get SIGCHLD when we exit
			if(pProcessData->PID != 1)
				SendSignal(pProcessData->ParentPID, SIGCHLD);

			//any children get inherited by init
			//also we are not ptracing any more
			for(int i=0; i<MAX_PROCESSES; i++)
			{
				if(pKernelSharedData->ProcessTable[i].ParentPID == pProcessData->PID)
					pKernelSharedData->ProcessTable[i].ParentPID = 1; //init
				if(pKernelSharedData->ProcessTable[i].ptrace_owner_pid == pProcessData->PID)
					pKernelSharedData->ProcessTable[i].ptrace_owner_pid = 0; //no-one
			}

			ktrace("Process exit [code=%d, sig=%d]\n", pProcessData->exitcode, pProcessData->killed_by_sig);
		}
		else
		{
			ktrace("Process exit [exec parent]\n");
		}
	}
	else
	{
		ktrace("Process exit [no pProcessData]\n");
	}
}


/*
 * This is the execption handler that allows us to intercept
 * and process kernel calls
 */
DWORD HandleExceptionInELF(DWORD ExceptionCode, LPEXCEPTION_POINTERS pEP)
{
	if(ExceptionCode==EXCEPTION_ACCESS_VIOLATION)
	{
		//is this an INT 80 linux SYSCALL?

		union u {
			struct {
				BYTE byte1;
				BYTE byte2;
				BYTE byte3;
				BYTE byte4;
			} b;
			struct {
				WORD word1;
				WORD word2;
			} w;
			DWORD dword;
		} *instruction;

		if(IsBadCodePtr((FARPROC)pEP->ExceptionRecord->ExceptionAddress))
		{
			//instruction not addressable
			ktrace("Jump to invalid execution address 0x%08lx\n", pEP->ExceptionRecord->ExceptionAddress);

			//try to get actual return address from the stack
			if(!IsBadReadPtr((void*)pEP->ContextRecord->Esp ,sizeof(DWORD))) 
			{
				DWORD addr = *((DWORD*)(pEP->ContextRecord->Esp));
				ktrace("Possibly from a call @ 0x%08lx\n", addr);
			}

			return EXCEPTION_EXECUTE_HANDLER;
		}

		instruction = (union u*)pEP->ExceptionRecord->ExceptionAddress;
		if(instruction->w.word1 == 0x80CD)
		{
			//handle it
			HandleSysCall(pEP->ContextRecord);

			//skip over the INT instruction
			pEP->ContextRecord->Eip += 2;

			//resume 
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		else
		{
			//just a regular exception, pass the the elf code
			SendSignal(pProcessData->PID, SIGSEGV);
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	//unhandled exception
	ktrace("Unhandled exception type 0x%08lx @ 0x%08lx\n", ExceptionCode, pEP->ExceptionRecord->ExceptionAddress);
	return EXCEPTION_EXECUTE_HANDLER;
}

EXCEPTION_DISPOSITION __cdecl my_except_handler (
    struct _EXCEPTION_RECORD *ExceptionRecord,
    void * EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    void * DispatcherContext
    )
{
	if(pProcessData)
	{
		pProcessData->ptrace_ctx = *ContextRecord;
		pProcessData->ptrace_ctx_valid = true;
	}

	EXCEPTION_POINTERS ep;
	EXCEPTION_DISPOSITION ret;

	ep.ContextRecord = ContextRecord;
	ep.ExceptionRecord = ExceptionRecord;

	switch( HandleExceptionInELF(ExceptionRecord->ExceptionCode, &ep) )
	{
	case EXCEPTION_CONTINUE_EXECUTION:
		ret = ExceptionContinueExecution;
		break;
	case EXCEPTION_EXECUTE_HANDLER:
	default:
		ret = ExceptionContinueSearch;
		break;
	}

	if(pProcessData)
		pProcessData->ptrace_ctx_valid = false;
	return ret;
}


static void SetupStdHandles()
{
	//init file handles prior to copying any that are required
	ZeroMemory(&pProcessData->FileHandlers, sizeof(pProcessData->FileHandlers));
	//standard unix file handles (stdin,stdout,stderr)
	pProcessData->FileHandlers[0] = new ConsoleIOHandler(CONSOLE0_NUM, true);
	pProcessData->FileHandlers[1] = pProcessData->FileHandlers[0]->Duplicate(GetCurrentProcess(), GetCurrentProcess());
	pProcessData->FileHandlers[2] = pProcessData->FileHandlers[0]->Duplicate(GetCurrentProcess(), GetCurrentProcess());
	pProcessData->FileHandlers[0]->SetInheritable(true);
	pProcessData->FileHandlers[1]->SetInheritable(true);
	pProcessData->FileHandlers[2]->SetInheritable(true);

}


extern "C" _declspec(dllexport) void Process_Init(const char* keyword, int pid, const char * KernelInstance)
{
	int i;
	
	ktrace_should_append = strcmp(keyword,"EXECVE")==0; //sharing a trace file when doing an exec

	ktrace("Process_Init:preinit\n");

	//shared kernel data
	HANDLE h = OpenFileMapping(GENERIC_READ|GENERIC_WRITE, FALSE, KernelInstance);
	pKernelSharedData = (KernelSharedDataStruct*)MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0,0,0);
	//leave handle open

	//synchronisation handles
	hKernelLock = CreateMutex(NULL, FALSE, KernelInstance);

	WaitTerminatingEventName = new char [strlen(KernelInstance)+20];
	StringCbPrintf(WaitTerminatingEventName, strlen(KernelInstance)+20, "%s.WaitTermEvent", KernelInstance);
	WaitTerminatingEvent = CreateEvent(NULL, TRUE, FALSE, WaitTerminatingEventName);
	
	//OUR data
	pProcessData = &pKernelSharedData->ProcessTable[pid];
	pProcessData->PID = pid;
	pProcessData->exitcode = -SIGSEGV; //in case no set later
	//need originater to set this or exec etc: pProcessData->MainThreadID = GetCurrentThreadId();
	pProcessData->hKernelDebugFile = INVALID_HANDLE_VALUE;

	//need to ensure process table has no-one with use as parent (we are a new process after all!)
	for(i=0; i<MAX_PROCESSES; i++)
	{
		if(pKernelSharedData->ProcessTable[i].ParentPID == pProcessData->PID)
			pKernelSharedData->ProcessTable[i].ParentPID = 0;
	}

	//can use krace after we set hKernelDebugFile
	ktrace("Process_Init: %s\n", keyword);
	ktrace("pid %d, win32 pid %d, main thread x%X\n", pid, GetCurrentProcessId(), GetCurrentThreadId());
	ktrace("%s\n", pProcessData->ProgramPath);
	ktrace("arguments: @ 0x%08lx\n", pProcessData->argv);
	if(pProcessData->argv)
	{
		for(i=0; pProcessData->argv[i]; ++i)
			ktrace("%5d %s\n", i, pProcessData->argv[i]);
	}
	ktrace("environment: @ 0x%08lx\n", pProcessData->envp);
	if(pProcessData->envp)
	{
		for(i=0; pProcessData->envp[i]; ++i)
			ktrace("%5d %s\n", i, pProcessData->envp[i]);
	}


	//DEBUG
//	if(pProcessData->PID == 2)
//		DebugBreak();


	//we need to know where the original stack is (before we do anything to play with it)
	//this is so that we know details for copying it between processes during fork
	ADDR stack;
	__asm mov stack,esp
	pProcessData->original_stack_esp = stack;
	//TIB* pTIB;
	//__asm {
	//	mov eax, fs:[18h]
	//	mov pTIB, eax
	//}
	//pProcessData->original_stack_esp = (ADDR)pTIB->pvStackUserTop;
	ktrace("Stack ESP is @ 0x%08lx\n", pProcessData->original_stack_esp);

	//need to set current directory for the process?
	if(strcmp(keyword, "INIT")!=0)
	{
		Path pwd;
		pwd.SetUnixPath(pProcessData->unix_pwd);
		SetCurrentDirectory(pwd.Win32Path());
	}


	//install exception handler for capturing INT 80h Linux kernel syscalls
	//need this installed early before stack diversifies (because of how we fork)
#undef USE_MSCV_TRY_EXCEPT
#ifdef USE_MSCV_TRY_EXCEPT
	__try 
	{
#else
	//install raw handler - not msvc one
	EXCEPTION_REGISTRATION_RECORD except_reg;
	DWORD tmp;
	__asm { //preserve old exception pointer
		mov eax, fs:[0]
		mov tmp, eax
	}
	except_reg.pNext = (PEXCEPTION_REGISTRATION_RECORD)tmp;
	except_reg.pfnHandler = (FARPROC)my_except_handler;
	__asm { //store new handler
		lea eax, except_reg
		mov fs:[0], eax
	}
#endif

		if(strcmp(keyword,"FORK") == 0)
		{
			//init file handles prior to copying any that are required
			ZeroMemory(&pProcessData->FileHandlers, sizeof(pProcessData->FileHandlers));

			ForkChildCopyFromParent();

			ktrace("FAIL! Fork should never get here\n");
			ExitProcess(-11);
		}

		if(strcmp(keyword, "RUN")==0	//when starting the first program
		|| strcmp(keyword, "INIT")==0	//just init dll, rung nothing
		|| strcmp(keyword, "EXECVE")==0) //exec
		{
			if(strcmp(keyword,"EXECVE") == 0)
				CopyParentHandlesForExec();
			else
				SetupStdHandles();	//defaults - no parent to copy from

			//default signal handling
			for(i=0; i<MAX_SIGNALS; ++i) {
				pProcessData->signal_action[i].sa_handler = SIG_DFL;
				pProcessData->signal_action[i].sa_flags = NULL;
				pProcessData->signal_action[i].sa_restorer = NULL;
				ZeroMemory(&pProcessData->signal_action[i].sa_mask, sizeof(pProcessData->signal_action[i].sa_mask));
			}
			ZeroMemory(&pProcessData->sigmask, sizeof(pProcessData->sigmask));
			ZeroMemory(&pProcessData->signal_blocked, sizeof(pProcessData->signal_blocked));
			pProcessData->signal_depth = 0;

			//we need a thread to handle signals
			//(because windows doesn't seem to have an interruption mechanism like unix signals)
			/*hSignalHandlerThread=*/ CreateThread(NULL, 0, SignalThreadMain, 0, 0, &pProcessData->SignalThreadID);


			//Process was started with some memory allocated, but not recorded
			//in our linked list.
			//Add that now (ELF and Loaded memory)
			ZeroMemory(&pProcessData->m_MemoryAllocationsHeader, sizeof(pProcessData->m_MemoryAllocationsHeader));
			if(strcmp(keyword, "INIT")!=0)
			{
				RecordMemoryAllocation(pProcessData->program_base, pProcessData->program_max-pProcessData->program_base, PAGE_EXECUTE_READWRITE);
				RecordMemoryAllocation(pProcessData->interpreter_base, pProcessData->interpreter_max-pProcessData->interpreter_base, PAGE_EXECUTE_READWRITE);
				//argv,envp have a preceeding MemBlock to tell their info
				MemBlock *pmb;
				pmb=(MemBlock*)((char*)pProcessData->argv - sizeof(MemBlock));
				RecordMemoryAllocation(pmb->addr, pmb->len, PAGE_EXECUTE_READWRITE);
				pmb=(MemBlock*)((char*)pProcessData->envp - sizeof(MemBlock));
				RecordMemoryAllocation(pmb->addr, pmb->len, PAGE_EXECUTE_READWRITE);
			}
		}
		else
		{
			ktrace("Invalid Process_Init keyword: %s\n", keyword);
			ExitProcess((UINT)-SIGSEGV);
		}

		//really starting?
		if(strcmp(keyword, "RUN")==0
		|| strcmp(keyword, "EXECVE")==0)
		{
			TransferControlToELFCode();
		}

#ifdef USE_MSCV_TRY_EXCEPT
	}
	__except( HandleExceptionInELF(GetExceptionCode(), GetExceptionInformation()) ) {
		ktrace("Exiting with unhandled SIGSEGV\n");
		ExitProcess((UINT)-11); //SIGSEGV
	}
#else
	//remove raw handler
	tmp = (DWORD)except_reg.pNext;
	__asm { //restore original except reg
		mov eax, tmp
		mov fs:[0], eax
	}
#endif
}


extern "C" _declspec(dllexport) void HandleSysCall( CONTEXT * pCtx )
{
	// eax is the syscall number
	// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
	// any more parameters and the caller just puts a struct pointer in one of these
	// eax is the return value
	SetLastError(0);
	DWORD syscall = pCtx->Eax;
	if(syscall < NR_syscalls)
	{
		ktrace("debug: syscall %d [%s] from @ 0x%08lx\n", pCtx->Eax, syscall_names[pCtx->Eax], pCtx->Eip);

		if(pProcessData->ptrace_owner_pid
		&& pProcessData->ptrace_request == PTRACE_SYSCALL )
		{
			ktrace("stopping for ptrace on syscall entry eax=%d\n", syscall);

			//entry to syscall has eax as -ENOSYS
			//original eax is available as saved value

			pProcessData->ptrace_saved_eax = syscall;
			pProcessData->ptrace_ctx.Eax = (DWORD)-ENOSYS; //this is what ptrace in the tracer sees as eax

			SendSignal(pProcessData->PID, SIGTRAP);
		}

		syscall_handlers[syscall](pCtx);
		
		if(pProcessData->ptrace_owner_pid
		&& pProcessData->ptrace_request == PTRACE_SYSCALL )
		{
			ktrace("stopping for ptrace on syscall exit eax=%d, orig=%d\n", pCtx->Eax, syscall);

			//return from syscall has eax as return value
			//original eax is available as saved value

			pProcessData->ptrace_ctx = *pCtx; //new state
			pProcessData->ptrace_saved_eax = syscall;

			SendSignal(pProcessData->PID, SIGTRAP);
		}
	}
	else
	{
		ktrace("debug: bad syscall %d [???] from @ 0x%08lx\n", syscall, pCtx->Eip);
		sys_unhandled(pCtx);
	}

	ktrace("debug: syscall return (Eax=0x%lx,%ld)\n", pCtx->Eax, pCtx->Eax);
}





/*
 * translate win32 gtlasterror value to linux errno
 */
int Win32ErrToUnixError(DWORD err)
{
	switch(err)
	{
	case ERROR_SUCCESS:
		return 0;

	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return ENOENT;

	case ERROR_TOO_MANY_OPEN_FILES:
		return EMFILE;

	case ERROR_ACCESS_DENIED:
		return EACCES;

	case ERROR_INVALID_HANDLE:
		return EBADF;

	case ERROR_ARENA_TRASHED:
	case ERROR_INVALID_BLOCK:
		return EFAULT;

	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return ENOMEM;

	case ERROR_INVALID_FUNCTION:
		return ENOSYS;

	case ERROR_BROKEN_PIPE:
		return EIO;

	default:
		ktrace("Unhandled Win32 Error code %ld\n", err);
		return EPERM; //generic
	}
}


/*
 * kernel debug tracing
 */
void ktrace(const char * format, ...)
{
	if(pKernelSharedData && pKernelSharedData->KernelDebug==0)
		return;

	int bufsize = strlen(format) + 1000;
	char * buf = new char [bufsize];

	SYSTEMTIME st;
	GetSystemTime(&st);

	va_list va;
	va_start(va, format);
	StringCbPrintf(buf, bufsize, "[%d : %ld] %d:%02d:%02d.%04d ",
					(pProcessData?pProcessData->PID:0), GetCurrentProcessId(),
					st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
	StringCbVPrintf(&buf[strlen(buf)], bufsize-strlen(buf), format, va);
	va_end(va);

	if(pKernelSharedData && pProcessData)
	{
		if(pProcessData->hKernelDebugFile==INVALID_HANDLE_VALUE)
		{
			char n[MAX_PATH];
			StringCbPrintf(n,sizeof(n),"%s\\..\\pid%05d.trace", pKernelSharedData->LinuxFileSystemRoot, pProcessData->PID);
			pProcessData->hKernelDebugFile = CreateFile(n, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, ktrace_should_append?OPEN_ALWAYS:CREATE_ALWAYS, 0, 0);
		}
		DWORD dw;
		SetFilePointer(pProcessData->hKernelDebugFile, 0,0, FILE_END);
		WriteFile(pProcessData->hKernelDebugFile, buf, strlen(buf), &dw ,0);
		//FlushFileBuffers(pProcessData->hKernelDebugFile);
	}

	OutputDebugString(buf);
	delete buf;
}


/*
 * find and allocate a free process table entry
 * -1 on error
 */
int AllocatePID()
{
	int pid = -1;

	WaitForSingleObject(hKernelLock, INFINITE);

	int i, cnt;
	i=pKernelSharedData->LastAllocatedPID+1;
	for(cnt=0; cnt<MAX_PROCESSES; cnt++)
	{
		if(i>=MAX_PROCESSES)
			i=1;
		if(!pKernelSharedData->ProcessTable[i].in_use)
		{
			memset(&pKernelSharedData->ProcessTable[i], 0, sizeof(pKernelSharedData->ProcessTable[i]));
			pKernelSharedData->ProcessTable[i].in_use = true;
			pKernelSharedData->ProcessTable[i].PID = i;
			pid = i;
			pKernelSharedData->LastAllocatedPID = i;
			break;
		}
	}

	ReleaseMutex(hKernelLock);
	return pid;
}

/*
 * find a free file descriptor entry
 * -1 on error
 */
int FindFreeFD()
{
	int fd;
	for(fd=0; fd<MAX_OPEN_FILES; ++fd)
	{
		if(pProcessData->FileHandlers[fd]==NULL)
			break;
	}
	if(fd>=MAX_OPEN_FILES)
		return -1;
	else
		return fd;
}


/* debug helper */
void MemoryDump(const char * msg, const void * from_addr, DWORD sz)
{
	char *buf;
	char *end;
	char *line, *chars;
	const unsigned char * a = (const unsigned char*)from_addr;
	size_t remaining;
	int col;
	DWORD idx;
	const char hex[] = "0123456789abcdef";
	int bufsize;

	bufsize = 200 + strlen(msg) + (sz * 5) + (sz/8 * 20);
	buf = new char [ bufsize ];
	StringCbPrintfEx(buf, bufsize, &end, &remaining, 0, "Memory dump [%s] @ 0x%08lx, size %ld\n", msg, a, sz);

	//dump in 8 byte blocks like this:
	//  0x1234abcd  00 01 02 03  f5 f2 f8 00   .... ....
	for(idx=0; idx<sz; )
	{
		//prepare a new row
		StringCbPrintfEx(end,remaining, &end, &remaining, 0, "   0x%08lx  ", a);
		line = end;
		StringCbCopyEx(end, remaining, "00 00 00 00  00 00 00 00  ", &end, &remaining, 0);
		chars = end;
		StringCbCopyEx(end, remaining, ".... ....\n", &end, &remaining, 0);

		//add data as available
		for(col=0; col<8; idx++,col++,a++)
		{
			if(idx<sz)
			{
				int c = *a;
				*line = hex[c>>4];  line++;
				*line = hex[c&0xF]; line++;
				if(ispunct(c) || isalnum(c))
					*chars = c;
				else
					*chars = '.';
				chars++;
			}
			else
			{
				*line  = ' ';
				*line  = ' ';
				*chars = ' ';
				line+=2;
				chars++;
			}

			line += (col==3 ? 2 : 1);
			chars += (col==3 ? 1 : 0);
		}
	}

	ktrace(buf);
	delete buf;
}



/*
 * test state of a thread
 */
bool IsThreadSuspended(HANDLE hThread)
{
	DWORD cnt = SuspendThread(hThread);
	if(cnt==-1)
		return false;
	ResumeThread(hThread);
	return cnt>0;
}

/*
 * 
 */
bool WaitForThreadToSuspend(HANDLE hThread)
{
	while(true)
	{
		DWORD cnt = SuspendThread(hThread);
		if(cnt == (DWORD)-1)
			return false; //failed - no thread?

		ResumeThread(hThread);
		if(cnt>0)
			return true;// was already suspended

		Sleep(10);
	}
}
