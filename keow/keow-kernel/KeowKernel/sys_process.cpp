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

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 * exit process with a return code
 * EBX = exit code
 */
void sys_exit(CONTEXT* pCtx)
{
	ktrace("Process terminating, code 0x%08lx\n", pCtx->Ebx);
	KeowProcess()->exitcode = pCtx->Ebx;
	KeowProcess()->killed_by_sig = 0;
	exit(pCtx->Ebx);
}


/*****************************************************************************/


/*
 * int sigaction(int signum, const struct old_sigaction *act, struct old_sigaction *oldact);
 */
void  sys_sigaction(CONTEXT* pCtx)
{
	DWORD signum = pCtx->Ebx;
	linux::old_sigaction *act = (linux::old_sigaction*)pCtx->Ecx;
	linux::old_sigaction *oldact = (linux::old_sigaction*)pCtx->Edx;

	if(signum > MAX_SIGNALS)
	{
		pCtx->Eax = -EINVAL;
		return;
	}

	if(oldact)
	{
		oldact->sa_handler  = KeowProcess()->signal_action[signum].sa_handler;
		oldact->sa_flags    = KeowProcess()->signal_action[signum].sa_flags;
		oldact->sa_restorer = KeowProcess()->signal_action[signum].sa_restorer;
		oldact->sa_mask     = KeowProcess()->signal_action[signum].sa_mask.sig[0];
	}

	if(act)
	{
		KeowProcess()->signal_action[signum].sa_handler     = act->sa_handler;
		KeowProcess()->signal_action[signum].sa_flags       = act->sa_flags;
		KeowProcess()->signal_action[signum].sa_restorer    = act->sa_restorer;
		KeowProcess()->signal_action[signum].sa_mask.sig[0] = act->sa_mask;

		ktrace("set old_sigaction handler for signal %d : 0x%08lx\n", signum, act->sa_handler);
	}

	pCtx->Eax = 0;
}

/*****************************************************************************/

/*
 * int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
 */
void  sys_sigprocmask(CONTEXT* pCtx)
{
	linux::sigset_t *set = (linux::sigset_t*)pCtx->Ecx;
	linux::sigset_t *oldset = (linux::sigset_t*)pCtx->Edx;

	linux::sigset_t *currsigset = &KeowProcess()->sigmask[KeowProcess()->signal_depth];

	if(oldset)
	{
		*oldset = *currsigset;
		pCtx->Eax = 0;
		return;
	}

	if(set)
	{
		for(int i=0; i<_NSIG_WORDS; ++i)
		{
			switch(pCtx->Ebx)
			{
			case SIG_BLOCK:
				currsigset->sig[i] |= set->sig[i];
				pCtx->Eax = 0;
				break;
			case SIG_UNBLOCK:
				currsigset->sig[i] &= ~(set->sig[i]);
				pCtx->Eax = 0;
				break;
			case SIG_SETMASK:
				currsigset->sig[i] = set->sig[i];
				pCtx->Eax = 0;
				break;

			default:
				pCtx->Eax = -EINVAL;
				return;
			}
		}
	}
}



/*****************************************************************************/

/*
 * pid_t getpgrp()
 */
void  sys_getpgrp(CONTEXT* pCtx)
{
	pCtx->Eax = KeowProcess()->ProcessGroupPID;
}

/*****************************************************************************/

/*
 * pid_t getpgrp()
 */
void  sys_getpgid(CONTEXT* pCtx)
{
	pCtx->Eax = KeowProcess()->ProcessGroupPID;
}

/*****************************************************************************/


/*
 * pid_t setpgrp(int pid, int pgid)
 */
void  sys_setpgid(CONTEXT* pCtx)
{
	DWORD pid = pCtx->Ebx;
	DWORD pgid = pCtx->Ecx;

	if(pid==0)
		pid = KeowProcess()->PID;
	if(pgid==0)
		pgid = KeowProcess()->ProcessGroupPID;

	//validate pid
	if(pid<1 || pid>MAX_PROCESSES
	|| g_KernelData.ProcessTable[pid].in_use==false)
	{
		pCtx->Eax = -ESRCH;
		return;
	}
	
	g_KernelData.ProcessTable[pid].ProcessGroupPID = pgid;
	pCtx->Eax = 0;
}

/*****************************************************************************/

/*
 * pid_t fork()
 */
void sys_fork(CONTEXT* pCtx)
{
	pCtx->Eax = DoFork(pCtx);
}

/*
 * int execve(filename, argv[], envp[])
 */
void sys_execve(CONTEXT* pCtx)
{
	pCtx->Eax = DoExecve((const char*)pCtx->Ebx, (char**)pCtx->Ecx, (char**)pCtx->Edx);
}

/*****************************************************************************/

/*
 * pid_t wait4(pid_t pid, int *status, int options, rusage *ru)
 */
void sys_wait4(CONTEXT* pCtx)
{
	int wait_pid = (int)pCtx->Ebx;
	int* status = (int*)pCtx->Ecx;
	DWORD options = pCtx->Edx;
	linux::rusage* ru = (linux::rusage*)pCtx->Esi;
	int result = -ECHILD;
	bool one_stopped = false;

	//there can be multiple handles to wait on 
	DWORD * pProcessWin32Pids = new DWORD[MAX_PROCESSES+1];
	HANDLE * pProcessHandles = new HANDLE[MAX_PROCESSES+1];
	HANDLE * pMainThreadHandles = new HANDLE[MAX_PROCESSES+1];
	DWORD * pPids = new DWORD[MAX_PROCESSES+1];
	int NumHandles = 0;


	//select children to wait on
	for(int i=0; i<MAX_PROCESSES; i++)
	{
		if(g_KernelData.ProcessTable[i].ParentPID == KeowProcess()->PID)
		{
			bool wait_this = false;

			if(wait_pid < -1  &&  g_KernelData.ProcessTable[i].ProcessGroupPID == wait_pid)
				wait_this=true;
			else
			if(wait_pid == -1)
				wait_this=true;
			else
			if(wait_pid == 0  &&  g_KernelData.ProcessTable[i].ProcessGroupPID == KeowProcess()->ProcessGroupPID)
				wait_this=true;
			else
			if(wait_pid > 0  &&  g_KernelData.ProcessTable[i].PID == wait_pid)
				wait_this=true;
			
			
			if(wait_this)
			{
				pProcessWin32Pids[NumHandles] = g_KernelData.ProcessTable[i].Win32PID;
				pProcessHandles[NumHandles] = OpenProcess(SYNCHRONIZE, FALSE, g_KernelData.ProcessTable[i].Win32PID);
				pMainThreadHandles[NumHandles] = NULL; //only open if required
				pPids[NumHandles] = g_KernelData.ProcessTable[i].PID;
				NumHandles++; 

				//process already exited, no need to wait?
				if(g_KernelData.ProcessTable[i].in_use==false)
				{
					result = i;
					break;
				}
			}
		}
	}

	if(NumHandles==0)
	{
		//no such child
		result = -ECHILD;
	}
	else
	{
		pProcessHandles[NumHandles] = WaitTerminatingEvent;
		//not increment NumHandles+

		//wait until interrupted or child terminates
		while(result < 0)
		{
			DWORD dwRet = WaitForMultipleObjects(NumHandles+1, pProcessHandles, FALSE, options&WNOHANG?0:50);
			if(dwRet>=WAIT_OBJECT_0 && dwRet<=WAIT_OBJECT_0+NumHandles)
			{
				if(dwRet==WAIT_OBJECT_0+NumHandles) //(NumHandles+1)-1
				{
					//received a signal that stops wait()
					result = -EINTR;
					break;
				}

				int pid = pPids[dwRet-WAIT_OBJECT_0];
				//did the process die? or was there something else to test
				if(g_KernelData.ProcessTable[pid].in_use==false)
				{
					result = pid;
					break;
				}
				//maybe died without updating table?
				HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, g_KernelData.ProcessTable[pid].Win32PID);
				if(hProc==NULL)
				{
					g_KernelData.ProcessTable[pid].in_use=false;
					result = pid;
					break;
				}
				else
				{
					DWORD dwCode;
					if(GetExitCodeProcess(hProc, &dwCode)==0
					|| dwCode != STILL_ACTIVE)
					{
						CloseHandle(hProc);
						g_KernelData.ProcessTable[pid].in_use=false;
						result = pid;
						break;
					}
					CloseHandle(hProc);
				}
				//else try again
			}
			/*
			else
			if(dwRet>=WAIT_ABANDONED_0 && dwRet<WAIT_ABANDONED_0+NumHandles)
			{
				int pid = pPids[dwRet-WAIT_ABANDONED_0];
				//did the process die? or was there something else to test
				if(g_KernelData.ProcessTable[pid].in_use==false)
				{
					result = pid;
					break;
				}
				//else try again
			}*/
			else
			if(dwRet==WAIT_FAILED)
			{
				result = -EINVAL;
				break;
			}

			if(options & WNOHANG)
				break;

			//check for stopped children
			//also handle fork/exec process usage
			for(i=0; i<NumHandles; ++i)
			{
				int pid = pPids[i];
				if(g_KernelData.ProcessTable[pid].in_use)
				{
					//does caller want stopped children?
					if((options & WUNTRACED)
					|| g_KernelData.ProcessTable[pid].ptrace_owner_pid==KeowProcess()->PID)
					{
						if(g_KernelData.ProcessTable[pid].in_setup)
						{
							ktrace("skipping wait for pid %d - in setup flag set\n", pid);
						}
						else
						{
							//open threads on demand only
							if(pMainThreadHandles[i]==NULL)
								pMainThreadHandles[i] = OpenThread(THREAD_ALL_ACCESS, FALSE, g_KernelData.ProcessTable[pid].MainThreadID);

							if(IsThreadSuspended(pMainThreadHandles[i]))
							{
								result = pid;
								one_stopped = true;
								break;
							}
						}
					}
				}
				else
				{
					//not in use - seems to have died since we started waiting
					result = pid;
					break;
				}

				if(pProcessWin32Pids[i] != g_KernelData.ProcessTable[pid].Win32PID)
				{
					CloseHandle(pProcessHandles[i]);
					pProcessWin32Pids[i] = g_KernelData.ProcessTable[pid].Win32PID;
					pProcessHandles[i] = OpenProcess(SYNCHRONIZE, FALSE, g_KernelData.ProcessTable[pid].Win32PID);
					if(pMainThreadHandles[i])
						CloseHandle(pMainThreadHandles[i]);
					pMainThreadHandles[i] = NULL;
				}
			}

			//Wait debugging
			//Sleep(10);
			//DebugBreak();

		}
	}

	if(result>0)
	{
		//a process was found
		ProcessDataStruct *p = &g_KernelData.ProcessTable[result];

		if(status)
		{
			*status = 0;

			if(one_stopped)
			{
				//stopped
				*status = 0x7f; 

				//signal that stopped it
				if(p->ptrace_owner_pid==KeowProcess()->PID && p->ptrace_request!=0)
					*status |= SIGTRAP << 8; //man ptrace says parent thinks child is in this state
				else
				//	*status |= SIGSTOP << 8;
					*status |= (p->current_signal&0xFF) << 8;
			}
			else
			{
				//exit status
				*status |= (p->exitcode & 0xFF) << 8;
			
				//terminating signal
				*status |= p->killed_by_sig & 0x7f;
				
				//core flag
				*status |= p->core_dumped?0x80:0x00;
			}
		}

		if(ru)
		{
			memset(ru, 0, sizeof(linux::rusage));
			ktrace("IMPLEMENT wait4 rusage\n");
		}

		//now forget about that child (unless it was a stopped one we returned)
		if(!one_stopped)
		{
			p->ParentPID = 0;
		}
	}

	//free
	for(i=0; i<NumHandles; i++)
	{
		CloseHandle(pProcessHandles[i]);
		if(pMainThreadHandles[i])
			CloseHandle(pMainThreadHandles[i]);
	}
	delete [] pProcessWin32Pids;
	delete [] pProcessHandles;
	delete [] pMainThreadHandles;
	delete [] pPids;
	ResetEvent(WaitTerminatingEvent);

	pCtx->Eax = result;


	ktrace("wait4(%d) returned %d [%s]\n", wait_pid, result, one_stopped?"stopped":"exited" );
}
