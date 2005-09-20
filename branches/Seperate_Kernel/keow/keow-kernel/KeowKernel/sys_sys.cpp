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

#include "includes.h"
#include "SysCalls.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 *  uname(struct utsname *name)
 */
void SysCalls::sys_uname(CONTEXT &ctx)
{
	linux::new_utsname * pU = (linux::new_utsname *)ctx.Ebx;
	linux::new_utsname U;

	DWORD siz = sizeof(U.sysname);
	GetComputerName(U.sysname, &siz);
	StringCbCopy(U.nodename, sizeof(U.nodename), U.sysname);
	StringCbCopy(U.release, sizeof(U.release), "2.4.20"); //specs we are using are this (Sep 2005)
	StringCbCopy(U.version, sizeof(U.version), "keow");
	StringCbCopy(U.machine, sizeof(U.machine), "i386");  //we need this value?
	U.domainname[0] = 0;

	P->WriteMemory((ADDR)pU, sizeof(U), &U);
	ctx.Eax = 0;
}


/*****************************************************************************/

/*
 * int getpid()
 */
void SysCalls::sys_getpid(CONTEXT &ctx)
{
	ctx.Eax = P->m_Pid;
}

/*****************************************************************************/

/*
 * int getppid()
 */
void SysCalls::sys_getppid(CONTEXT &ctx)
{
	ctx.Eax = P->m_ParentPid;
}


/*****************************************************************************/

/*
 * mode_t umask(mode_t mask)
 */
void SysCalls::sys_umask(CONTEXT &ctx)
{
	ctx.Eax = P->m_umask;
	P->m_umask = ctx.Ebx;
}


/*****************************************************************************/

/*
 * time_t time(time_t *t);
 */
void SysCalls::sys_time(CONTEXT &ctx)
{
	SYSTEMTIME st;
	FILETIME ft;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);

	ctx.Eax = FILETIME_TO_TIME_T(ft);

	if(ctx.Ebx)
		P->WriteMemory((ADDR)ctx.Ebx, sizeof(DWORD), &ctx.Eax);
}


/*****************************************************************************/

/*
 * long getrlimit(uint resource, rlimit* rlim)
 */
void SysCalls::sys_ugetrlimit(CONTEXT &ctx)
{
	linux::rlimit *pRLim = (linux::rlimit *)ctx.Ecx;
	linux::rlimit RLim;

	if(!pRLim)
	{
		ctx.Eax = -EFAULT;
		return;
	}

	ctx.Eax = 0;
	switch(ctx.Ebx)
	{
	case RLIMIT_CPU:		/* CPU time in ms */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_FSIZE:		/* Maximum filesize */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_DATA:		/* max data size */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_STACK:		/* max stack size */
		RLim.rlim_cur = RLim.rlim_max = P->m_KeowUserStackTop - P->m_KeowUserStackBase;
		break;
	case RLIMIT_CORE:		/* max core file size */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_RSS:		/* max resident set size */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_NPROC:		/* max number of processes */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_NOFILE:		/* max number of open files */
		RLim.rlim_cur = RLim.rlim_max = MAX_OPEN_FILES;
		break;
	case RLIMIT_MEMLOCK:	/* max locked-in-memory address space */
		RLim.rlim_cur = RLim.rlim_max = 0x70000000;
		break;
	case RLIMIT_AS:			/* address space limit */
		RLim.rlim_cur = RLim.rlim_max = 0x7000000;
		break;
	case RLIMIT_LOCKS:		/* maximum file locks held */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	default:
		ctx.Eax = -EINVAL;
		return;
	}

	P->WriteMemory((ADDR)pRLim, sizeof(RLim), &RLim);
}

/*
 * same as ugetrlimit except unsigned values
 */
void SysCalls::sys_getrlimit(CONTEXT &ctx)
{
	linux::rlimit *pRLim = (linux::rlimit *)ctx.Ecx;
	linux::rlimit RLim;

	if(!pRLim)
	{
		ctx.Eax = -EFAULT;
		return;
	}

	ctx.Eax = 0;
	switch(ctx.Ebx)
	{
	case RLIMIT_CPU:		/* CPU time in ms */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_FSIZE:		/* Maximum filesize */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_DATA:		/* max data size */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_STACK:		/* max stack size */
		RLim.rlim_cur = RLim.rlim_max = P->m_KeowUserStackTop - P->m_KeowUserStackBase;
		break;
	case RLIMIT_CORE:		/* max core file size */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_RSS:		/* max resident set size */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_NPROC:		/* max number of processes */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	case RLIMIT_NOFILE:		/* max number of open files */
		RLim.rlim_cur = RLim.rlim_max = MAX_OPEN_FILES;
		break;
	case RLIMIT_MEMLOCK:	/* max locked-in-memory address space */
		RLim.rlim_cur = RLim.rlim_max = 0x70000000;
		break;
	case RLIMIT_AS:			/* address space limit */
		RLim.rlim_cur = RLim.rlim_max = 0x7000000;
		break;
	case RLIMIT_LOCKS:		/* maximum file locks held */
		RLim.rlim_cur = RLim.rlim_max = RLIM_INFINITY;
		break;
	default:
		ctx.Eax = -EINVAL;
		return;
	}

	P->WriteMemory((ADDR)pRLim, sizeof(RLim), &RLim);
}



/*****************************************************************************/

/*
 * int reboot(int magic, int magic2, int flag, void *arg);
 */
void SysCalls::sys_reboot(CONTEXT &ctx)
{
	ctx.Eax = -EINVAL;

	if(ctx.Ebx != LINUX_REBOOT_MAGIC1)
		return;

	if(ctx.Ecx != LINUX_REBOOT_MAGIC2
	&& ctx.Ecx != LINUX_REBOOT_MAGIC2A
	&& ctx.Ecx != LINUX_REBOOT_MAGIC2B)
		return;

	switch(ctx.Edx)
	{
	case LINUX_REBOOT_CMD_RESTART:
	case LINUX_REBOOT_CMD_HALT:
	case LINUX_REBOOT_CMD_CAD_ON:
	case LINUX_REBOOT_CMD_CAD_OFF:
	case LINUX_REBOOT_CMD_POWER_OFF:
	case LINUX_REBOOT_CMD_RESTART2:
		ktrace("Implement me: Dummy reboot() - no action taken\n");
		ctx.Eax = 0;
		break;
	}
}

/*****************************************************************************/


/*
 * int kill(pid, sig)
 */
void SysCalls::sys_kill(CONTEXT &ctx)
{
	PID pid = ctx.Ebx;
	unsigned int sig = ctx.Ecx;

	if(sig>=_NSIG)
	{
		ctx.Eax = -EINVAL;
		return;
	}

	//helper: sig 0 allows for error checking on pids. no signal actually sent

	// positive - send to 1 process
	if(pid>0)
	{
		ctx.Eax = -ESRCH;
		Process * pDest = g_pKernelTable->FindProcess(pid);
		if(pDest!=NULL)
		{
			if(sig!=0)
				pDest->SendSignal(sig);
			ctx.Eax = 0;
		}
		return;
	}

	// -1 all processes except init
	if(pid == -1)
	{
		KernelTable::ProcessList::iterator it;
		for(it=g_pKernelTable->m_Processes.begin(); it!=g_pKernelTable->m_Processes.end(); ++it)
		{
			if((*it)->m_Pid != 1)
			{
				if(sig!=0)
					(*it)->SendSignal(sig);
			}
		}
		ctx.Eax = 0;
		return;
	}


	//else send to a process group

	PID pgrp = 0;

	if(pid==0)
		pgrp = P->m_Pid; //our group
	else
		pgrp = -pid; //dest group


	KernelTable::ProcessList::iterator it;
	for(it=g_pKernelTable->m_Processes.begin(); it!=g_pKernelTable->m_Processes.end(); ++it)
	{
		if((*it)->m_Pid != 1
		&& (*it)->m_ProcessGroupPID == pgrp)
		{
			if(sig!=0)
				(*it)->SendSignal(sig);
		}
	}
	ctx.Eax = 0;

#undef DO_SIGNAL

}

/*****************************************************************************/


/*
 * int gettimeofday(timeval* tv, timezone* tz)
 * same as time() but has tv_usec too;
 */
void SysCalls::sys_gettimeofday(CONTEXT &ctx)
{
	linux::timeval * pTv = (linux::timeval*)ctx.Ebx;
	//never used for linux -   linux::timezone * tz = (linux::timezone*)pCtx->Ecx;
	linux::timeval tv;

	SYSTEMTIME st;
	FILETIME ft;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st,&ft);

	tv.tv_sec = FILETIME_TO_TIME_T(ft);
	tv.tv_usec = st.wMilliseconds;

	P->WriteMemory((ADDR)pTv, sizeof(tv), &tv);
	ctx.Eax = 0;
}

/*****************************************************************************/


/*
 * long ptrace(enum request, int pid, void* addr, void* data)
 */
void SysCalls::sys_ptrace(CONTEXT &ctx)
{
	int request = ctx.Ebx;
	int pid = ctx.Ecx;
	void* addr = (void*)ctx.Edx;
	void* data = (void*)ctx.Esi;

	Process * pTraced = g_pKernelTable->FindProcess(pid);

	switch(request)
	{
	case PTRACE_TRACEME:
		ktrace("ptrace PTRACE_TRACEME\n");
		P->m_ptrace.OwnerPid = P->m_ParentPid;
		P->m_ptrace.Request = PTRACE_TRACEME;
		ctx.Eax = 0;
		break;

	case PTRACE_SYSCALL:
		{
			ktrace("ptrace PTRACE_SYSCALL pid %d\n", pid);
			pTraced->m_ptrace.Request = PTRACE_SYSCALL;
			if((int)data!=0 && (int)data!=SIGSTOP)
				pTraced->m_ptrace.new_signal = (int)data;
			ktrace("ptrace_syscall resuming pid %d\n",pid);
			ResumeThread(pTraced->m_Win32PInfo.hThread);
			ctx.Eax = 0;
		}
		break;

	case PTRACE_CONT:
		{
			ktrace("ptrace PTRACE_CONT pid %d\n", pid);
			pTraced->m_ptrace.Request = 0;
			if((int)data!=0 && (int)data!=SIGSTOP)
				pTraced->m_ptrace.new_signal = (int)data;
			ktrace("ptrace_cont resuming pid %d\n",pid);
			ResumeThread(pTraced->m_Win32PInfo.hThread);
			ctx.Eax = 0;
		}
		break;

	case PTRACE_PEEKUSR:
		{
			ktrace("ptrace PTRACE_PEEKUSR pid %d addr 0x%lx (reg %ld) data 0x%08lx\n", pid, addr, (DWORD)addr>>2, data);

			if((unsigned long)addr > sizeof(linux::user)-3)
			{
				ctx.Eax = -EFAULT;
				break;
			}

			//read data at offset 'addr' in the kernel 'user' struct (struct user in asm/user.h)
			//we don't keep one of those so make one here
			CONTEXT *pTracedCtx;
			CONTEXT TempCtx;
			if(pTraced->m_ptrace.ctx_valid)
				pTracedCtx = &pTraced->m_ptrace.ctx; 
			else
			{
				ktrace("peekusr get Traced context\n");
				//already suspended for ptrace, no need for SuspendThread(hThread);
				TempCtx.ContextFlags = CONTEXT_FULL;
				GetThreadContext(pTraced->m_Win32PInfo.hThread, &TempCtx);
				//dont ResumeThread;
				pTracedCtx = &TempCtx;
			}

			linux::user usr;
			memset(&usr,0,sizeof(usr));
			usr.regs.eax  = pTracedCtx->Eax;
			usr.regs.ebx  = pTracedCtx->Ebx;
			usr.regs.ecx  = pTracedCtx->Ecx;
			usr.regs.edx  = pTracedCtx->Edx;
			usr.regs.esi  = pTracedCtx->Esi;
			usr.regs.edi  = pTracedCtx->Edi;
			usr.regs.ebp  = pTracedCtx->Ebp;
			usr.regs.ds   = (unsigned short)pTracedCtx->SegDs;
			usr.regs.__ds = 0;
			usr.regs.es   = (unsigned short)pTracedCtx->SegEs;
			usr.regs.__es = 0;
			usr.regs.fs   = (unsigned short)pTracedCtx->SegFs;
			usr.regs.__fs = 0;
			usr.regs.gs   = (unsigned short)pTracedCtx->SegGs;
			usr.regs.__gs = 0;
			usr.regs.cs   = (unsigned short)pTracedCtx->SegCs;
			usr.regs.__cs = 0;
			usr.regs.ss   = (unsigned short)pTracedCtx->SegSs;
			usr.regs.__ss = 0;
			usr.regs.orig_eax = pTraced->m_ptrace.Saved_Eax;
			usr.regs.eip  = pTracedCtx->Eip;
			usr.regs.eflags = pTracedCtx->EFlags;
			usr.regs.esp  = pTracedCtx->Esp;
			//usr.signal = SIGTRAP; //man ptrace says parent thinks Traced is in this state

			char * wanted = ((char*)&usr) + (DWORD)addr;
			P->WriteMemory((ADDR)data, sizeof(DWORD), wanted);

			ktrace("ptrace [0x%x]=0x%x eax=0x%x orig_eax=0x%x\n", addr, *((DWORD*)wanted), usr.regs.eax, usr.regs.orig_eax);

			ctx.Eax = 0;
		}
		break;

	case PTRACE_PEEKTEXT:
	case PTRACE_PEEKDATA:
		{
			ktrace("ptrace PTRACE_PEEKDATA pid %d addr 0x%lx data 0x%08lx\n", pid, addr, data);
			DWORD tmp;
			if(pTraced->ReadMemory(&tmp, (ADDR)addr, sizeof(tmp)))
				ctx.Eax = -EFAULT;
			else
				ctx.Eax = tmp;
		}
		break;

	case PTRACE_KILL:
		ktrace("ptrace PTRACE_KILL pid %d \n", pid);
		pTraced->SendSignal(SIGKILL);
		ctx.Eax = 0;
		break;

	default:
		ktrace("IMPLEMENT ptrace request %lx\n", ctx.Ebx);
		ctx.Eax = -ENOSYS;
		break;
	}
}


/*****************************************************************************/


/*
 * int nanosleep(timespec*req, timespec*rem)
 */
void SysCalls::sys_nanosleep(CONTEXT &ctx)
{
	linux::timespec * pReq = (linux::timespec*)ctx.Ebx;
	linux::timespec * pRem = (linux::timespec*)ctx.Ecx;
	linux::timespec req;
	linux::timespec rem;

	P->ReadMemory(&req, (ADDR)pReq, sizeof(req));

	DWORD start = GetTickCount();
	DWORD msec = (req.tv_sec*1000) + (req.tv_nsec/1000000); //milliseconds to wait

	//signal interruptable wait
	if(WaitForSingleObject(P->m_hWaitTerminatingEvent, msec) == WAIT_OBJECT_0)
	{
		ctx.Eax = -EINTR;
	}
	else
	{
		ctx.Eax = 0;
	}

	if(pRem)
	{
		DWORD end = GetTickCount();
		if(end < start)
		{
			//tick count wrapped!
			start -= (end+1);
			end -= (end+1);;
		}
		msec -= (end-start);
		rem.tv_sec = msec/1000;
		rem.tv_nsec = msec%1000 * 1000000;

		P->WriteMemory((ADDR)pRem, sizeof(rem), &rem);
	}
}

