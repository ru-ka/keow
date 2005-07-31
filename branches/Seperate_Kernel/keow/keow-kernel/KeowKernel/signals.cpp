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

/*
 * Handling unix signals under win32
 */

#include "kernel.h"
#include "forkexec.h"


/*
 * this part of the signal handler runs in the elf code thread
 * runs with C calling convention - caller pops arguments from the stack
 */
static void _cdecl HandleSignal(int sig)
{
	ktrace("signal handler starting for %d (depth %d)\n", sig, KeowProcess()->signal_depth);

	KeowProcess()->current_signal = sig;

	//participate in ptrace()
	if(KeowProcess()->ptrace_owner_pid)
	{
		ktrace("stopping for ptrace in HandleSignal\n");
		KeowProcess()->ptrace_new_signal = sig;
		SendSignal(KeowProcess()->ptrace_owner_pid, SIGCHLD);
		SuspendThread(GetCurrentThread());

		ktrace("resumed from ptrace stop in HandleSignal\n");
		//did tracer alter the signal to deliver?
		if(sig != KeowProcess()->ptrace_new_signal)
		{
			sig = KeowProcess()->ptrace_new_signal;
			KeowProcess()->current_signal = sig;
			ktrace("ptrace replacement signal %d\n", sig);
			if(sig==0)
				return;
		}
	}

	//some signals cannot be caught
	switch(sig)
	{
	case SIGKILL:
		ktrace("killed - sigkill\n");
		KeowProcess()->killed_by_sig = sig;
		ExitProcess((UINT)-sig);
		return;
	case SIGSTOP:
		ktrace("stopping on sigstop\n");
		SuspendThread(GetCurrentThread());
		ktrace("resumed from sigstop\n");
		return;
	}

	//signal handler debugging
	//DebugBreak();

	//signal is blocked by this function?
	//if(KeowProcess()->signal_blocked[sig] > 0)
	//{
	//	ktrace("signal not delivered - currently blocked\n");
	//	KeowProcess()->current_signal = 0;
	//	return;
	//}

	//is this process ignoring this signal?
	linux::sigset_t mask = KeowProcess()->sigmask[KeowProcess()->signal_depth];
	if( (mask.sig[(sig-1)/_NSIG_BPW]) & (1 << (sig-1)%_NSIG_BPW) )
	{
		ktrace("signal not delivered - currently masked\n");
		KeowProcess()->current_signal = 0;
		return;
	}


	//nested
	KeowProcess()->signal_depth++;
	if(KeowProcess()->signal_depth >= MAX_PENDING_SIGNALS)
	{
		KeowProcess()->signal_depth = MAX_PENDING_SIGNALS-1;
		ktrace("WARN overflow in nested signal handling\n");
	}
	KeowProcess()->sigmask[KeowProcess()->signal_depth] = KeowProcess()->signal_action[sig].sa_mask;

	//is there a signal handler registered for this signal?
	//	KeowProcess()->signal_action[signum].sa_handler     = act->sa_handler;
	//	KeowProcess()->signal_action[signum].sa_flags       = act->sa_flags;
	//	KeowProcess()->signal_action[signum].sa_restorer    = act->sa_restorer;
	//	KeowProcess()->signal_action[signum].sa_mask.sig[0] = act->sa_mask;
	if(KeowProcess()->signal_action[sig].sa_handler == SIG_DFL)
	{
		//default action for this signal
		ktrace("executing default action for signal\n");
		switch(sig)
		{
		//ignore
		case SIGCHLD:
		case SIGURG:
			break;

		//special case - can ignore - aleady done in signal handler thread - can't wake ourselves anyway!
		case SIGCONT: 
			break;

		//stop
		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			SuspendThread(GetCurrentThread());
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
			KeowProcess()->killed_by_sig = sig;
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
			ExitProcess(-sig);
			break;

		//ptrace
		case SIGTRAP:
			if(KeowProcess()->ptrace_owner_pid)
				break; //ignore
			//else drop thru to core dump

		//core dump
		case SIGQUIT:
		case SIGILL:
		case SIGABRT:
		case SIGFPE:
		case SIGSEGV:
		case SIGBUS:
		case SIGSYS:
			KeowProcess()->killed_by_sig = sig;
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
			GenerateCoreDump();
			ExitProcess(-sig);
			break;

		default:
			KeowProcess()->killed_by_sig = sig;
			ktrace("IMPLEMENT default action for signal %d\n", sig);
			ktrace("Exiting using SIG_DFL for sig %d\n",sig);
			ExitProcess(-sig);
			break;
		}
	}
	else
	if(KeowProcess()->signal_action[sig].sa_handler == SIG_IGN)
	{
		//ignore the signal
		ktrace("ignoring signal - sig_ign\n");
	}
	else
	{
		ktrace("dispatching to signal handler @ 0x%08lx\n", KeowProcess()->signal_action[sig].sa_handler);

		//supposed to supress the signal whilst in the handler?
		//linux just sets to SIG_DFL? (see man 2 signal)
		linux::__sighandler_t handler = KeowProcess()->signal_action[sig].sa_handler;

		//restore handler?
		if((KeowProcess()->signal_action[sig].sa_flags & SA_ONESHOT)
		|| (KeowProcess()->signal_action[sig].sa_flags & SA_RESETHAND) )
			KeowProcess()->signal_action[sig].sa_handler = SIG_DFL;

		//dispatch to custom handler
		if(KeowProcess()->signal_action[sig].sa_flags & SA_SIGINFO)
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

			((void (_cdecl *)(int, linux::siginfo *, void *))handler)(sig, &si, /*&ct*/0);
		}
		else
		{
			((void (_cdecl *)(int))handler)(sig);
			ktrace("sig_handler(sig %d) returned\n",sig);
		}

		//use restorer
		//in linux this is a call to sys_sigreturn ?
		//to return to user-land?
		//documented as 'dont use'
		//if(KeowProcess()->signal_action[sig].sa_flags & SA_RESTORER)
		//{
		//	((void (_cdecl *)(void))KeowProcess()->signal_action[sig].sa_restorer)();
		//}

	}

		
	//un-nest
	KeowProcess()->signal_depth--;

	KeowProcess()->current_signal = 0;

	ktrace("signal handler ending for %d (depth %d)\n", sig, KeowProcess()->signal_depth);
}

//initial code injected into the ELF code so cause a jump to the signal handler in the correct thread context
static char * handler_entry_message = "HandleSignalEntry\n";
static char * handler_exit_message = "HandleSignalEntry exit\n";
__declspec(naked) static void HandleSignalEntry()
{
	//on entry stack has
	//   eip for return
	//   original eax
	//   signal
	__asm {
		//get values but preserve all registers
		push ebp
		mov ebp, esp
		pushfd
		pushad

		//ktrace on entry - to debug injection
		mov eax, handler_entry_message
		push eax
		call ktrace
		pop eax

		//call handler
		mov eax, [ebp+4]
		push eax //signal argument
		call HandleSignal
		pop eax //signal argument - _cdecl caller pops stack

		//ktrace on entry - to debug injection
		mov eax, handler_exit_message
		push eax
		call ktrace
		pop eax

		//restore
		popad
		popfd
		pop ebp
		pop eax //the signal - discard
		pop eax //the orignal eax
		ret		//use saved eip
	}
}


/*
 * this part of the signal handler runs in the signal handler thread
 * and dispatches the signal to the main thread
 */
static void DispatchSignal(int sig)
{
	//we want the signal handled in the main thread.
	//this is because on unix the process is interrupted
	//to handle signals, and signal handlers can use setjmp/longjmp
	//to get back to their code!
	ktrace("attempting to dispatch signal %d\n", sig);

	if(sig==SIGKILL)
	{
		//no arguing here - just die
		ktrace("killed - sigkill\n");
		KeowProcess()->killed_by_sig = sig;
		ExitProcess((UINT)-sig);
	}


	//pause the thread, get its execution context,
	//update the context to call to a signal handler
	//let the thread resume

	HANDLE hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, KeowProcess()->MainThreadID);

	//SIGCONT is special - continues suspended process
	if(sig==SIGCONT)
	{
		ktrace("resuming on sigcont\n");
		if(ResumeThread(hMainThread)==-1)
		{
			ktrace("WARN SIGCONT cannot resume thread \n");
		}
	}
	else
	{
		bool SavedInSetup = KeowProcess()->in_setup;
		KeowProcess()->in_setup = true; //because we are suspending and fiddling

		if(SuspendThread(hMainThread)==-1)
		{
			ktrace("Terminating process, cannot suspend main thread to install signal handler\n");
			ExitProcess(-SIGABRT);
		}

		CONTEXT Ctx;
		Ctx.ContextFlags = CONTEXT_CONTROL;
		if(!GetThreadContext(hMainThread, &Ctx))
		{
			ktrace("Terminating process, cannot get thread context\n");
			ExitProcess(-SIGABRT);
		}


		//debug for testing naked asm in HandleSignalEntry
		ktrace("injecting signal handler into elf\n");
		ktrace("was eip 0x%08lx, esp 0x%08lx\n", Ctx.Eip, Ctx.Esp);

		//save context info on the threads stack for restoring later
		//save EIP
		DWORD esp = Ctx.Esp;
		esp -= sizeof(DWORD);
		*((DWORD*)esp) = Ctx.Eip;
		//also push the signal we are dealing with
		esp -= sizeof(DWORD);
		*((DWORD*)esp) = sig;
		//update thread to run signal handler
		Ctx.Esp = (DWORD)esp;
		Ctx.Eip = (DWORD)&HandleSignalEntry;


		if(!SetThreadContext(hMainThread, &Ctx))
		{
			ktrace("Terminating process, cannot set thread context\n");
			ExitProcess(-SIGABRT);
		}
		if(ResumeThread(hMainThread)==-1)
		{
			ktrace("Terminating process, cannot resume thread \n");
			ExitProcess(-SIGABRT);
		}

		KeowProcess()->in_setup = SavedInSetup; //stopped fiddling

	}

	CloseHandle(hMainThread);
}



DWORD WINAPI SignalThreadMain(LPVOID param)
{
	//we are important
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	//we handle signals and other events as thread messages,
	//so look for them in a message loop
	MSG msg;
	while(GetMessage(&msg, NULL, 0,0))
	{
		switch(msg.message)
		{
		case WM_KERNEL_SIGNAL:
			DispatchSignal(msg.wParam);
			break;

		case WM_KERNEL_SETFORKCONTEXT:
			//main thread has asked us to set it's thread context for child of a fork
			SetForkChildContext();
			break;

		default:
			DispatchMessage(&msg);
			break;
		}
	}

	return 0;
}


/*
 * helper for sending signals correctly
 */
bool SendSignal(int pid, int sig)
{
	ktrace("sending signal %d to pid %d\n", sig, pid);

//test because we now don't want to dispatch signals whilst in non-ELF code
#undef SELF_SIGNALS_SPECIAL
#ifdef SELF_SIGNALS_SPECIAL
	if(pid==KeowProcess()->PID
	&& KeowProcess()->MainThreadID==GetCurrentThreadId())
	{
		//signalling ourselves, just do it!
		HandleSignal(sig);
		return true;
	}
	else
#endif
	{
		//not interrupting the current thread, so must dispatch via handler thread
		return PostThreadMessage(g_KernelData.ProcessTable[pid].SignalThreadID, WM_KERNEL_SIGNAL, sig, 0) != 0;
	}
}
