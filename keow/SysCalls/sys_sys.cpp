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

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 *  uname(struct utsname *name)
 */
void sys_uname(CONTEXT* pCtx)
{
	linux::new_utsname * u = (linux::new_utsname *)pCtx->Ebx;
	DWORD siz = __NEW_UTS_LEN;

	GetComputerName(u->sysname, &siz);
	StringCbCopy(u->nodename, sizeof(u->nodename), u->sysname);
	StringCbCopy(u->release, sizeof(u->release), "2.4.20");
	StringCbCopy(u->version, sizeof(u->version), "KernelEmulationOnWindows");
	StringCbCopy(u->machine, sizeof(u->machine), "i386");  //TODO: get actual processor, may alter syscalls made
	u->domainname[0] = 0;

	pCtx->Eax = 0;

}


/*****************************************************************************/

/*
 * int getpid()
 */
void  sys_getpid(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->PID;
}

/*****************************************************************************/

/*
 * int getppid()
 */
void  sys_getppid(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->ParentPID;
}


/*****************************************************************************/

/*
 * mode_t umask(mode_t mask)
 */
void  sys_umask(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->umask;
	pProcessData->umask = pCtx->Ebx;
}


/*****************************************************************************/

/*
 * time_t time(time_t *t);
 */
void  sys_time(CONTEXT* pCtx)
{
	SYSTEMTIME st;
	FILETIME ft;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);

	pCtx->Eax = FILETIME_TO_TIME_T(ft);

	if(pCtx->Ebx)
		*((linux::time_t*)pCtx->Ebx) = pCtx->Eax;
}


/*****************************************************************************/

/*
 * long getrlimit(uint resource, rlimit* rlim)
 */
void sys_ugetrlimit(CONTEXT* pCtx)
{
	linux::rlimit *rlim = (linux::rlimit *)pCtx->Ecx;

	if(!rlim)
	{
		pCtx->Eax = -EFAULT;
		return;
	}

	pCtx->Eax = 0;
	switch(pCtx->Ebx)
	{
	case RLIMIT_CPU:		/* CPU time in ms */
		rlim->rlim_cur = rlim->rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_FSIZE:		/* Maximum filesize */
		rlim->rlim_cur = rlim->rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_DATA:		/* max data size */
		rlim->rlim_cur = rlim->rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_STACK:		/* max stack size */
		rlim->rlim_cur = rlim->rlim_max = 0x100000;
		break;
	case RLIMIT_CORE:		/* max core file size */
		rlim->rlim_cur = rlim->rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_RSS:		/* max resident set size */
		rlim->rlim_cur = rlim->rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_NPROC:		/* max number of processes */
		rlim->rlim_cur = rlim->rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_NOFILE:		/* max number of open files */
		rlim->rlim_cur = rlim->rlim_max = MAX_OPEN_FILES;
		break;
	case RLIMIT_MEMLOCK:	/* max locked-in-memory address space */
		rlim->rlim_cur = rlim->rlim_max = 0x70000000;
		break;
	case RLIMIT_AS:			/* address space limit */
		rlim->rlim_cur = rlim->rlim_max = 0x7000000;
		break;
	case RLIMIT_LOCKS:		/* maximum file locks held */
		rlim->rlim_cur = rlim->rlim_max = RLIM_INFINITY;
		break;
	default:
		pCtx->Eax = -EINVAL;
		return;
	}

}

/*
 * same as ugetrlimit except unsigned values
 */
void sys_getrlimit(CONTEXT* pCtx)
{
	linux::rlimit *rlim = (linux::rlimit *)pCtx->Ecx;

	if(!rlim)
	{
		pCtx->Eax = -EFAULT;
		return;
	}

	sys_ugetrlimit(pCtx);

	//trunc?
	if(rlim->rlim_cur > 0x7FFFFFFF)
		rlim->rlim_cur = 0x7FFFFFFF;
	if(rlim->rlim_max > 0x7FFFFFFF)
		rlim->rlim_max = 0x7FFFFFFF;
}



/*****************************************************************************/

/*
 * int reboot(int magic, int magic2, int flag, void *arg);
 */
void  sys_reboot(CONTEXT* pCtx)
{
	pCtx->Eax = -EINVAL;

	if(pCtx->Ebx != LINUX_REBOOT_MAGIC1)
		return;

	if(pCtx->Ecx != LINUX_REBOOT_MAGIC2
	&& pCtx->Ecx != LINUX_REBOOT_MAGIC2A
	&& pCtx->Ecx != LINUX_REBOOT_MAGIC2B)
		return;

	switch(pCtx->Edx)
	{
	case LINUX_REBOOT_CMD_RESTART:
	case LINUX_REBOOT_CMD_HALT:
	case LINUX_REBOOT_CMD_CAD_ON:
	case LINUX_REBOOT_CMD_CAD_OFF:
	case LINUX_REBOOT_CMD_POWER_OFF:
	case LINUX_REBOOT_CMD_RESTART2:
		ktrace("Implement me: Dummy reboot() - no action taken\n");
		pCtx->Eax = 0;
		break;
	}
}

/*****************************************************************************/


/*
 * int kill(pid, sig)
 */
void sys_kill(CONTEXT* pCtx)
{
	int pid = pCtx->Ebx;
	unsigned int sig = pCtx->Ecx;

	if(sig>=_NSIG)
	{
		pCtx->Eax = -EINVAL;
		return;
	}

//helper: sig 0 allows for error checking on pids. no signal actually sent
#define DO_SIGNAL(p,sig) (sig==0 ? TRUE : SendSignal(p,sig))

	// positive - send to 1 process
	if(pid>0)
	{
		pCtx->Eax = -ESRCH;
		if(pKernelSharedData->ProcessTable[pid].in_use)
		{
			if(DO_SIGNAL(pid,sig))
				pCtx->Eax = 0;
		}
		return;
	}

	// -1 all processes except init
	if(pid == -1)
	{
		for(int i=2; i<MAX_PROCESSES; ++i)
		{
			if(pKernelSharedData->ProcessTable[i].in_use)
			{
				DO_SIGNAL(i, sig);
			}
		}
		pCtx->Eax = 0;
		return;
	}


	//else send to a process group

	int pgrp = 0;

	if(pid==0)
		pgrp = pProcessData->PID;
	else
		pgrp = -pid;


	for(int i=2; i<MAX_PROCESSES; ++i)
	{
		if(pKernelSharedData->ProcessTable[i].in_use
		&& pKernelSharedData->ProcessTable[i].ProcessGroupPID == pgrp)
		{
			DO_SIGNAL(i, sig);
		}
	}
	pCtx->Eax = 0;

#undef DO_SIGNAL

}

/*****************************************************************************/


/*
 * int gettimeofday(timeval* tv, timezone* tz)
 * same as time() but has tv_usec too;
 */
void sys_gettimeofday(CONTEXT* pCtx)
{
	linux::timeval * tv = (linux::timeval*)pCtx->Ebx;
	//never used for linux -   linux::timezone * tz = (linux::timezone*)pCtx->Ecx;

	SYSTEMTIME st;
	FILETIME ft;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st,&ft);

	tv->tv_sec = FILETIME_TO_TIME_T(ft);
	tv->tv_usec = st.wMilliseconds;

	pCtx->Eax = 0;
}

/*****************************************************************************/


/*
 * long ptrace(enum request, int pid, void* addr, void* data)
 */
void sys_ptrace(CONTEXT* pCtx)
{
	int request = pCtx->Ebx;
	int pid = pCtx->Ecx;
	void* addr = (void*)pCtx->Edx;
	void* data = (void*)pCtx->Esi;

	switch(request)
	{
	case PTRACE_TRACEME:
		ktrace("ptrace PTRACE_TRACEME\n");
		pProcessData->ptrace_owner_pid = pProcessData->ParentPID;
		pProcessData->ptrace_request = PTRACE_TRACEME;
		pCtx->Eax = 0;
		break;

	case PTRACE_SYSCALL:
		{
			ktrace("ptrace PTRACE_SYSCALL pid %d\n", pid);
			ProcessDataStruct * pChildData = &pKernelSharedData->ProcessTable[pid];
			pChildData->ptrace_request = PTRACE_SYSCALL;
			if((int)data!=0 && (int)data!=SIGSTOP)
				pChildData->ptrace_new_signal = (int)data;
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, pChildData->MainThreadID);
			ktrace("ptrace_syscall resuming pid %d\n",pid);
			ResumeThread(hThread);
			CloseHandle(hThread);
			pCtx->Eax = 0;
		}
		break;

	case PTRACE_CONT:
		{
			ktrace("ptrace PTRACE_CONT pid %d\n", pid);
			ProcessDataStruct * pChildData = &pKernelSharedData->ProcessTable[pid];
			pChildData->ptrace_request = 0;
			if((int)data!=0 && (int)data!=SIGSTOP)
				pChildData->ptrace_new_signal = (int)data;
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, pChildData->MainThreadID);
			ktrace("ptrace_cont resuming pid %d\n",pid);
			ResumeThread(hThread);
			CloseHandle(hThread);
			pCtx->Eax = 0;
		}
		break;

	case PTRACE_PEEKUSR:
		{
			ktrace("ptrace PTRACE_PEEKUSR pid %d addr 0x%lx (reg %ld) data 0x%08lx\n", pid, addr, (DWORD)addr>>2, data);

			if((unsigned long)addr > sizeof(linux::user)-3)
			{
				pCtx->Eax = -EFAULT;
				break;
			}

			//read data at offset 'addr' in the kernel 'user' struct (struct user in asm/user.h)
			//we don't keep one of those so make one here
			ProcessDataStruct * pChildData = &pKernelSharedData->ProcessTable[pid];
			CONTEXT *pChildCtx;
			CONTEXT TempCtx;
			if(pChildData->ptrace_ctx_valid)
				pChildCtx = &pChildData->ptrace_ctx; 
			else
			{
				ktrace("peekusr get child context\n");
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, pChildData->MainThreadID);
				//already suspended for ptrace SuspendThread(hThread);
				TempCtx.ContextFlags = CONTEXT_FULL;
				GetThreadContext(hThread, &TempCtx);
				//ResumeThread(hThread);
				CloseHandle(hThread);
				pChildCtx = &TempCtx;
			}

			linux::user usr;
			memset(&usr,0,sizeof(usr));
			usr.regs.eax  = pChildCtx->Eax;
			usr.regs.ebx  = pChildCtx->Ebx;
			usr.regs.ecx  = pChildCtx->Ecx;
			usr.regs.edx  = pChildCtx->Edx;
			usr.regs.esi  = pChildCtx->Esi;
			usr.regs.edi  = pChildCtx->Edi;
			usr.regs.ebp  = pChildCtx->Ebp;
			usr.regs.ds   = (unsigned short)pChildCtx->SegDs;
			usr.regs.__ds = 0;
			usr.regs.es   = (unsigned short)pChildCtx->SegEs;
			usr.regs.__es = 0;
			usr.regs.fs   = (unsigned short)pChildCtx->SegFs;
			usr.regs.__fs = 0;
			usr.regs.gs   = (unsigned short)pChildCtx->SegGs;
			usr.regs.__gs = 0;
			usr.regs.cs   = (unsigned short)pChildCtx->SegCs;
			usr.regs.__cs = 0;
			usr.regs.ss   = (unsigned short)pChildCtx->SegSs;
			usr.regs.__ss = 0;
			usr.regs.orig_eax = pChildData->ptrace_saved_eax;
			usr.regs.eip  = pChildCtx->Eip;
			usr.regs.eflags = pChildCtx->EFlags;
			usr.regs.esp  = pChildCtx->Esp;
			//usr.signal = SIGTRAP; //man ptrace says parent thinks child is in this state

			char * retaddr = ((char*)&usr) + (DWORD)addr;
			DWORD retdata = *((DWORD*)retaddr);
			*((DWORD*)data) = retdata;

			ktrace("ptrace [0x%x]=0x%x eax=0x%x orig_eax=0x%x\n", addr, retdata, usr.regs.eax, usr.regs.orig_eax);

			pCtx->Eax = 0;
		}
		break;

	case PTRACE_PEEKTEXT:
	case PTRACE_PEEKDATA:
		{
			ktrace("ptrace PTRACE_PEEKDATA pid %d addr 0x%lx data 0x%08lx\n", pid, addr, data);
			ProcessDataStruct * pChildData = &pKernelSharedData->ProcessTable[pid];
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pChildData->Win32PID);
			if(hProcess==INVALID_HANDLE_VALUE)
				pCtx->Eax = -1;
			else
			{
				if(ReadMemory((ADDR)data, hProcess, (ADDR)addr, sizeof(DWORD)))
					pCtx->Eax = 0;
				else
					pCtx->Eax = -EFAULT;
				CloseHandle(hProcess);
			}
		}
		break;

	case PTRACE_KILL:
		ktrace("ptrace PTRACE_KILL pid %d \n", pid);
		SendSignal(pid, SIGKILL);
		pCtx->Eax = 0;
		break;

	default:
		ktrace("IMPLEMENT ptrace request %lx\n", pCtx->Ebx);
		pCtx->Eax = -ENOSYS;
		break;
	}
}


/*****************************************************************************/


/*
 * int nanosleep(timespec*req, timespec*rem)
 */
void sys_nanosleep(CONTEXT* pCtx)
{
	linux::timespec * req = (linux::timespec*)pCtx->Ebx;
	linux::timespec * rem = (linux::timespec*)pCtx->Ecx;

	DWORD start = GetTickCount();
	DWORD msec = (req->tv_sec*1000) + (req->tv_nsec/1000000); //milliseconds to wait

	//signal interruptable wait
	if(WaitForSingleObject(WaitTerminatingEvent, msec) == WAIT_OBJECT_0)
	{
		ResetEvent(WaitTerminatingEvent);
		pCtx->Eax = -EINTR;
	}
	else
	{
		pCtx->Eax = 0;
	}

	if(rem)
	{
		DWORD end = GetTickCount();
		if(end < start)
		{
			//tick count wrapped!
			start -= (end+1);
			end -= (end+1);;
		}
		msec -= (end-start);
		rem->tv_sec = msec/1000;
		rem->tv_nsec = msec%1000 * 1000000;
	}
}

