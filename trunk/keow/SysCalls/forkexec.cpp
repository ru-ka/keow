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
 * Handle fork() and exec() calls
 */

#pragma message("value that should match ProcessStub stack size in build properties")
#define PROCESS_STUB_STACK_SIZE 0x400000


#include "kernel.h"
#include "loadelf.h"
#include "forkexec.h"
#include "tib.h"

DWORD WINAPI SignalThreadMain(LPVOID param);


// These are used in ForkChildCopyFromParent and TransferControlToELF
static CONTEXT Fork_ContextParentMain;
static ADDR Fork_StackStart;
static DWORD Fork_StackSize;
static unsigned char Fork_ParentStack[PROCESS_STUB_STACK_SIZE];


/*
 * called by signal handler thread to set main thread context
 * see ForkChildCopyFromParent
 */
void SetForkChildContext()
{
	ktrace("attempting to set main thread context for fork child\n");

	//pause the thread, get its execution context,
	//update the context 
	//let the thread resume

	HANDLE hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, pProcessData->MainThreadID);

	if(SuspendThread(hMainThread)==-1)
	{
		ktrace("Terminating process, cannot suspend main thread to install context\n");
		ExitProcess(-11);
	}


	//copy the stack
	memcpy(Fork_StackStart, Fork_ParentStack, Fork_StackSize);

	//set context 
	if(!SetThreadContext(hMainThread, &Fork_ContextParentMain))
	{
		ktrace("Terminating process, cannot set thread context\n");
		ExitProcess(-11);
	}

	pProcessData->in_setup = false; //running for real now


	if(ResumeThread(hMainThread)==-1)
	{
		ktrace("Terminating process, cannot resume thread \n");
		ExitProcess(-11);
	}

	CloseHandle(hMainThread);
}


/*
 * Transfers control from startup to the Unix code
 * In theory this routine will never return
 */
void TransferControlToELFCode()
{
	//local variables so we don't need to do pProcessData->xxxx from _asm block
	ADDR phdr;
	DWORD phent;
	DWORD phnum;
	ADDR interpreter_base;
	ADDR interpreter_entry;
	ADDR program_entry;
	char **envp, **argv;
	int cntargv, cntenvp;
	int uid;
	int gid;
	char *progname;
	ADDR start_esp;

	ktrace("Transferring to ELF code\n");
	pProcessData->in_setup = false; //running for real now, even thought not in elf code just yet

	//record esp at point of jump to elf (for execve to reuse)
	__asm mov start_esp,esp
	pProcessData->elf_start_esp = start_esp;


	if(pProcessData->interpreter_entry == 0)
	{
		program_entry = pProcessData->program_entry;
		__asm {

			//need to clear registers, so to get correct jump
			//address, put it on stack and RET to it
			mov eax, program_entry
			push eax

			//init required registers
			mov eax,0
			mov ebx,0
			mov ecx,0
			mov edx,0
			mov esi,0
			mov edi,0
			mov ebp,0

			//start program
			//ret to value we put on the stack
			ret
		}
	}
	else
	{
		char * dummy_envp[1] = {NULL};
		char * dummy_argv[2] = {(char*)pProcessData->ProgramPath, NULL};
		phdr = pProcessData->phdr;
		phent = pProcessData->phent;
		phnum = pProcessData->phnum;
		interpreter_base = pProcessData->interpreter_base;
		interpreter_entry = pProcessData->interpreter_entry;
		program_entry = pProcessData->program_entry;
		uid=pProcessData->uid;
		gid=pProcessData->gid;
		progname = (char*)pProcessData->ProgramPath;
		envp = pProcessData->envp ? pProcessData->envp : dummy_envp;
		argv = pProcessData->argv ? pProcessData->argv : dummy_argv;
		//size of lists
		for(cntenvp=0; envp[cntenvp]; ++cntenvp)
			;
		for(cntargv=0; argv[cntargv]; ++cntargv)
			;

		__asm {
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

			*/

#define PUSH_LONG(val) \
			__asm mov eax, val   /*value we want*/ \
			__asm push eax		 /*push value*/ 

#define PUSH_AUX_VAL(id,val) \
			PUSH_LONG(val) \
			PUSH_LONG(id)

			//end marker 
			PUSH_LONG(0)

			//argv/env doesn't need to be on stack for us (will be elsewhere)

			//need padding?

			//auxv[...]
			PUSH_AUX_VAL(AT_NULL,	0)
			PUSH_AUX_VAL(AT_PAGESZ,	0x1000)
			PUSH_AUX_VAL(AT_PHDR,	phdr)
			PUSH_AUX_VAL(AT_PHENT,	phent)
			PUSH_AUX_VAL(AT_PHNUM,	phnum)
			PUSH_AUX_VAL(AT_BASE,	interpreter_base)
			PUSH_AUX_VAL(AT_ENTRY,	program_entry)
			PUSH_AUX_VAL(AT_UID,	uid)
			PUSH_AUX_VAL(AT_GID,	gid)

			//env - copy in reverse order
			mov edi, envp
			mov ecx, cntenvp
			shl ecx, 2
		copy_env:
			mov eax, [edi+ecx]
			push eax
			sub ecx, 4
			jae copy_env

			//argv[...] 
			mov edi, argv
			mov ecx, cntargv
			shl ecx, 2
		copy_argv:
			mov eax, [edi+ecx]
			push eax
			sub ecx, 4
			jae copy_argv

			//argc
			PUSH_LONG(cntargv)

			//need to clear registers, so to get correct jump
			//address, put it on stack and RET to it
			mov eax, interpreter_entry
			push eax

			//init required registers
			mov eax,0
			mov ebx,0
			mov ecx,0
			mov edx,0
			mov esi,0
			mov edi,0
			mov ebp,0

			//start interpreter
			//ret to value we put on the stack
			ret
		}
	}
}


/*****************************************************************************/

/*
 * this routine is called from the client process in order to copy the parent context
 */
void ForkChildCopyFromParent()
{
	//who is our parent
	ProcessDataStruct * pParentProcess = &pKernelSharedData->ProcessTable[pProcessData->ParentPID];

	//validate that we have compatable stack
	//we should do because all processes are started with the same ProcessStub.exe
	//this is the only win32 defined data structure that must match
	//everything else we control
	if(pProcessData->original_stack_esp != pParentProcess->original_stack_esp)
	{
		ktrace("PANIC fork() child has incompatable stack!\n");
		ExitProcess((UINT)-SIGSEGV);
	}

	//we need a thread to handle signals
	//(because windows doesn't seem to have an interruption mechanism like unix signals)
	/*hSignalHandlerThread=*/ CreateThread(NULL, 0, SignalThreadMain, 0, 0, (DWORD*)&pProcessData->SignalThreadID);


	//need handle to parent
	HANDLE hParent = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pParentProcess->Win32PID);
	HANDLE hParentMain = OpenThread(THREAD_ALL_ACCESS, FALSE, pParentProcess->MainThreadID);

	//parent should be suspended, but maybe it didn't get there yet?
	if(!WaitForThreadToSuspend(hParentMain))
	{
		ktrace("fork child - parent gone?\n");
		ExitProcess((UINT)-SIGSEGV);
	}

	//parent context

	#define CONTEXT_CONTROL         (CONTEXT_i386 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP
	#define CONTEXT_INTEGER         (CONTEXT_i386 | 0x00000002L) // AX, BX, CX, DX, SI, DI
	#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 0x00000004L) // DS, ES, FS, GS
	#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 0x00000008L) // 387 state
	#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x00000010L) // DB 0-3,6,7
	#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L) // cpu specific extensions

	#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER |\
						  CONTEXT_SEGMENTS)

	Fork_ContextParentMain.ContextFlags = CONTEXT_FULL|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS|CONTEXT_EXTENDED_REGISTERS;
	if(!GetThreadContext(hParentMain, &Fork_ContextParentMain))
	{
		ktrace("fork() child cannot read parent context\n");
		ExitProcess((UINT)-SIGSEGV);
	}

	//take a copy of the parent's stack
	//Fork_StackStart = (ADDR)pProcessData->fork_context.Esp;
	Fork_StackStart = (ADDR)Fork_ContextParentMain.Esp;
	Fork_StackSize = pProcessData->original_stack_esp - Fork_StackStart;
	if(!ReadMemory(Fork_ParentStack, hParent, Fork_StackStart, Fork_StackSize))
	{
		ktrace("fork() failed to copy stack\n");
		ExitProcess((UINT)-11);
	}

	//copy relavent TIB data
	//Done in DoFork() now
	//TIB *pTIB;
	//__asm {
	//	mov eax, fs:[18h]
	//	mov pTIB, eax
	//}
	//pTIB->pvExcept    = (PEXCEPTION_REGISTRATION_RECORD)pParentProcess->fork_TIB.pvExcept;
	//pTIB->pvArbitrary = pParentProcess->fork_TIB.pvArbitrary;


	//copy process data
	//some of these are pointers and become invalid across processes
	//we will fix them up below but need to zero them here
	//this has already been done by parent: *pProcessData = *pParentProcess;
	ZeroMemory((void*)&pProcessData->FileHandlers, sizeof(pProcessData->FileHandlers));
	ZeroMemory((void*)&pProcessData->m_MemoryAllocationsHeader, sizeof(pProcessData->m_MemoryAllocationsHeader));


	//copy all memory allocations from the parent
	//and the data content
	MemoryAllocRecord MemAlloc;
	MemAlloc = *(MemoryAllocRecord*)&pParentProcess->m_MemoryAllocationsHeader;
	DWORD len4k;
	while(MemAlloc.next)
	{
		if(!ReadMemory((ADDR)&MemAlloc, hParent, (ADDR)MemAlloc.next, sizeof(MemAlloc)))
		{
			ktrace("fork() child copy memalloc from parent failed\n");
			ExitProcess((UINT)-SIGSEGV);
		}

		switch(MemAlloc.type)
		{
		case MemoryAllocRecord::RecType::Memory:
			AllocateMemAndProtect(MemAlloc.addr, MemAlloc.len, MemAlloc.protection);
			//align length of copy to 4k boundary
			len4k = (MemAlloc.len+0xFFF) & 0xFFFFF000;
			if(!ReadMemory(MemAlloc.addr, hParent, MemAlloc.addr, len4k))
			{
				ktrace("fork() child copy data from parent failed\n");
//				ExitProcess((UINT)-SIGSEGV);
			}
			break;
		case MemoryAllocRecord::RecType::MMap:
			ktrace("TODO copy mmap's from parent\n");
			break;
		default:
			ktrace("TODO mad memoryrec type in copy from parent\n");
			break;
		}
	}


	//copy all file handles from the parent
	//we'll need to peek at parent memory to get the correct data
	for(int i=0; i<MAX_OPEN_FILES; ++i)
	{
		pProcessData->FileHandlers[i] = NULL;

		if(pParentProcess->FileHandlers[i] != NULL)
		{
			//get copy of the iohandler from parent

			//initial read to find out size info
			char buf[sizeof(IOHandler)];
			IOHandler* iohbase = (IOHandler*)buf;
			if(!ReadMemory((ADDR)&buf, hParent, (ADDR)pParentProcess->FileHandlers[i], sizeof(buf)))
			{
				ktrace("fork() child copy iohandler basic from parent failed\n");
				ExitProcess((UINT)-SIGSEGV);
			}

			//read the real copy
			IOHandler* iohcopy = (IOHandler*)new char[iohbase->GetSize()];
			if(!ReadMemory((ADDR)iohcopy, hParent, (ADDR)pParentProcess->FileHandlers[i], iohbase->GetSize()))
			{
				ktrace("fork() child copy iohandler actual from parent failed\n");
				delete (char*)iohcopy;
				ExitProcess((UINT)-SIGSEGV);
			}

			//keep copy - but only if handle was marked as inheritable
			if(iohcopy->GetInheritable())
			{
				ktrace("inherited fd %d\n", i);
				pProcessData->FileHandlers[i] = iohcopy->Duplicate(GetCurrentProcess(), GetCurrentProcess());
			}
			else
			{
				ktrace("do not inherit fd %d\n", i);
			}
			iohcopy->Close();
			delete (char*)iohcopy;
		}
	}


	//Finished copying from parent - let them resume
	CloseHandle(hParent);
	//resume the parent thread now that we have copied everything
	ResumeThread(hParentMain);
	CloseHandle(hParentMain);



	//The last thing we need to do is restore the stack, TIB & context
	//However we are currently using them!
	//Copying now would corrupt us.
	//So get signal thread to do it

	//resume as if we were the parent (now that we look alike)
	//we can't set our own context, so use our signal thread to acheive it
	while(!PostThreadMessage(pProcessData->SignalThreadID, WM_KERNEL_SETFORKCONTEXT, 0, 0)
	&& GetLastError() == ERROR_INVALID_THREAD_ID)
		Sleep(10);
	//wait until signal thread changes us.
	//never exit because we get a new context
	while(1)
		Sleep(10000);


	//should never get here, ktrace probably fails with our temp stack anyway
	ktrace("fork() failed to resume correct thread context\n");
	ExitProcess((UINT)-11);
}


/*
 * Fork this process.
 * This creates an identical copy of the current process
 * running under a new pid and new win32 process
 */
DWORD DoFork(CONTEXT *pCtx)
{
	//need a new pid
	int pid = AllocatePID();
	if(pid==-1)
	{
		ktrace("FAIL no available pid's\n");
		return -EAGAIN;
	}

	ProcessDataStruct *p = &pKernelSharedData->ProcessTable[pid];

	//we are busy fiddling with process stuff
	pProcessData->in_setup = true;


	PELF_Data pElf = (PELF_Data)calloc(sizeof(struct ELF_Data), 1);
	if(pElf==NULL)
	{
		p->in_use = false;
		pProcessData->in_setup = false; //done fiddling
		return -ENOMEM;
	}

	//a new process to start the code in
	//use FORK keyword in argv[0]
	char CommandLine[100];
	StringCbPrintf(CommandLine, sizeof(CommandLine), "FORK %d %s", pid, pKernelSharedData->KernelInstanceName);

	//inherit handles
	GetStartupInfo(&pElf->sinfo);
	if(!CreateProcess((char*)pKernelSharedData->ProcessStubFilename, CommandLine, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &pElf->sinfo, &pElf->pinfo))
	{
		DWORD err = GetLastError();
		ktrace("error %ld forking pid\n", err, pProcessData->PID);
		free(pElf);
		p->in_use = false;
		pProcessData->in_setup = false; //done fiddling
		return -EAGAIN;
	}


	//reserve memory in the new process
	//we don't copy over the data yet, just ensure that space cannot be used
	MemoryAllocRecord * pMemAlloc = &pProcessData->m_MemoryAllocationsHeader;
	while(pMemAlloc->next)
	{
		pMemAlloc = (MemoryAllocRecord*)pMemAlloc->next;

		switch(pMemAlloc->type)
		{
		case MemoryAllocRecord::RecType::Memory:
			AllocateMemAndProtectProcess(pElf->pinfo.hProcess, pMemAlloc->addr, pMemAlloc->len, pMemAlloc->protection);
			break;
		case MemoryAllocRecord::RecType::MMap:
			//reserve bt we'll need to free it later (in the child)
			VirtualAllocEx(pElf->pinfo.hProcess, pMemAlloc->addr, pMemAlloc->len, MEM_RESERVE, PAGE_NOACCESS);
			break;
		default:
			ktrace("Bad MemoryMapRecord\n");
			break;
		}
	}


	//initial process data
	*p = *pProcessData;

	p->in_setup = true; //setting up the child

	//overide certain values
	p->PID = pid;
	p->ParentPID = pProcessData->PID;
	p->ProcessGroupPID = pProcessData->ProcessGroupPID;
	p->hKernelDebugFile = INVALID_HANDLE_VALUE;

	p->in_use = true;
	p->Win32PID = pElf->pinfo.dwProcessId;
	p->MainThreadID = pElf->pinfo.dwThreadId;
	p->SignalThreadID = -1; //not yet set

	//new copy of args/env
	//these should already be ok because they were recorded as memory in the original process
	//p->argv = CopyStringListToProcess(pElf->pinfo.hProcess, pProcessData->argv); 
	//p->envp = CopyStringListToProcess(pElf->pinfo.hProcess, pProcessData->envp);

	//saved id is what effective id was at process start
	p->sav_uid = p->euid;
	p->sav_gid = p->egid;

	//not traced after fork
	ktrace("ptrace fork - ptrace state not copied (correct?)\n");
	p->ptrace_owner_pid = 0;

	p->fork_context = *pCtx;
	//copy more stack than context has
	DWORD my_esp;
	__asm mov my_esp, esp
	p->fork_esp = my_esp;

	//save Task Information Block data of ourselves
	TIB* pTIB;
	__asm {
		mov eax, fs:[18h]
		mov pTIB, eax
	}
	pProcessData->fork_TIB.pvExcept = (ADDR)pTIB->pvExcept;
	pProcessData->fork_TIB.pvArbitrary= (ADDR)pTIB->pvArbitrary;
	//same data for child
	p->fork_TIB = pProcessData->fork_TIB;


	//start the process running it will call ForkChildCopyFromParent
	ResumeThread(pElf->pinfo.hThread);

	//cleanup
	CloseHandle(pElf->pinfo.hThread);
	CloseHandle(pElf->pinfo.hProcess);

	free(pElf);

	//suspend ourselves. the child will resume us once
	//it has copied our process state
	SuspendThread(GetCurrentThread());


	//....
	//When we get here we may be either the parent or the child
	//....
	pProcessData->in_setup = false; //done fiddling


	//restore TEB data 
	//copy relavent TIB data
	__asm {
		mov eax, fs:[18h]
		mov pTIB, eax
	}
	pTIB->pvExcept    = (PEXCEPTION_REGISTRATION_RECORD)pProcessData->fork_TIB.pvExcept;
	pTIB->pvArbitrary = pProcessData->fork_TIB.pvArbitrary;


	//test and return the appropriate value
	if(pProcessData->PID == pid)
	{
		//we are the child
		return 0;
	}
	else
	{
		//we are the parent
		return pid;
	}
}


/*****************************************************************************/

/*
 * this routine duplicates handles from a parent process
 * we are sharing the ProcessDataStruct entry so need to be carefull with pointers
 */
void CopyParentHandlesForExec()
{
	//need handle to parent
	HANDLE hParent = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pProcessData->Win32PID);
	HANDLE hParentMain = OpenThread(THREAD_ALL_ACCESS, FALSE, pProcessData->MainThreadID);

	ktrace("execve child waiting for parent\n");

	//parent should be suspended, but maybe it didn't get there yet?
	if(!WaitForThreadToSuspend(hParentMain))
	{
		ktrace("execve child - parent gone?\n");
		ExitProcess((UINT)-SIGSEGV);
	}

	ktrace("execve child copying parent info\n");

	//copy all file handles from the parent
	//we'll need to peek at parent memory to get the correct data
	for(int i=0; i<MAX_OPEN_FILES; ++i)
	{
		if(pProcessData->FileHandlers[i] != NULL)
		{
			//get copy of the iohandler from parent

			//initial read to find out size info
			char buf[sizeof(IOHandler)];
			IOHandler* iohbase = (IOHandler*)buf;
			if(!ReadMemory((ADDR)&buf, hParent, (ADDR)pProcessData->FileHandlers[i], sizeof(buf)))
			{
				ktrace("execve() child copy iohandler basic from parent failed\n");
				ExitProcess((UINT)-SIGSEGV);
			}

			//read the real copy
			IOHandler* iohcopy = (IOHandler*)new char[iohbase->GetSize()];
			if(!ReadMemory((ADDR)iohcopy, hParent, (ADDR)pProcessData->FileHandlers[i], iohbase->GetSize()))
			{
				ktrace("execve() child copy iohandler actual from parent failed\n");
				delete (char*)iohcopy;
				ExitProcess((UINT)-SIGSEGV);
			}

			//keep copy - but only if handle was marked as inheritable
			if(iohcopy->GetInheritable())
			{
				ktrace("inherited fd %d\n", i);
				pProcessData->FileHandlers[i] = iohcopy->Duplicate(GetCurrentProcess(), GetCurrentProcess());
			}
			else
			{
				ktrace("do not inherit fd %d\n", i);
				pProcessData->FileHandlers[i] = NULL; 
			}
			iohcopy->Close();
			delete (char*)iohcopy;
		}
	}


	//update our data
	//do this before resuming the other process so that it knows we have taken over
	//and does not do bad things in kernel_term
	pProcessData->Win32PID = GetCurrentProcessId();
	pProcessData->MainThreadID = GetCurrentThreadId();


	//Finished copying from parent - let them resume
	CloseHandle(hParent);
	//resume the parent thread now that we have copied everything
	ResumeThread(hParentMain);
	CloseHandle(hParentMain);

	ktrace("execve child setup ok\n");


	//participate in ptrace() on a successfull exec
	if(pProcessData->ptrace_owner_pid)
	{
		ktrace("stopping for ptrace on successfull exec\n");

		pProcessData->in_setup = false; //TransferControlToELF code not called yet but we are done copying

		//need dummy context for this - fake a successfull execve return
		pProcessData->ptrace_ctx.Eax = 0;//success
		pProcessData->ptrace_saved_eax = __NR_execve; //syscall
		pProcessData->ptrace_ctx_valid = true;
		SendSignal(pProcessData->PID, SIGTRAP);
	}
}


/*
 * Replace current process with the new process.
 * Inherits open handles (files etc).
 * Inherits the same pid (and hence kernel ProcessDataStruct entry)
 */
DWORD DoExecve(const char * filename, char* argv[], char* envp[])
{
	//Start a new win32 process. We need to do this to correctly handle
	//memory allocations because our win32 memory use (malloc new etc)
	//may have used the memory areas the new executable needs
	//Safest to start again and just copy over the required handles

	//we are busy fiddling with process stuff
	pProcessData->in_setup = true;

	ktrace("execve of %s\n", filename);

	ProcessDataStruct *p = pProcessData; //we reuse our PID

	PELF_Data pElf = (PELF_Data)calloc(sizeof(struct ELF_Data), 1);
	if(pElf==NULL)
	{
		return -ENOMEM;
	}

	//New a new process to start the code in
	//use EXECVE keyword in argv[0]
	char CommandLine[100];
	StringCbPrintf(CommandLine, sizeof(CommandLine), "EXECVE %d %s", p->PID, pKernelSharedData->KernelInstanceName);

	GetStartupInfo(&pElf->sinfo);
	if(!CreateProcess(pKernelSharedData->ProcessStubFilename, CommandLine, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &pElf->sinfo, &pElf->pinfo))
	{
		DWORD err = GetLastError();
		fprintf(stderr,"error %ld starting %s\n", err, filename);
		free(pElf);
		return Win32ErrToUnixError(err);
	}

	if(!LoadELFFile(pElf, filename, false))
	{
		TerminateProcess(pElf->pinfo.hProcess,-1);
		CloseHandle(pElf->pinfo.hThread);
		CloseHandle(pElf->pinfo.hProcess);
		free(pElf);
		return -ENOEXEC;
	}

	//initial process data (overwrite what is different from caller)
	p->in_setup = true;

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
	p->brk = p->brk_base = pElf->brk;

	//leave win32 pid because new process wants to know who we are - will be changed later
	//p->Win32PID
	//p->MainThreadID
	p->SignalThreadID = -1; //not yet set

	//put new environment and args into the new process
	p->envp = CopyStringListToProcess(pElf->pinfo.hProcess, envp);
	p->argv = CopyStringListToProcess(pElf->pinfo.hProcess, argv);

	//start the process running
	ResumeThread(pElf->pinfo.hThread);

	//cleanup
	HANDLE hRet = pElf->pinfo.hProcess;
	CloseHandle(pElf->pinfo.hThread);

	free(pElf);


	//we are have been replaced by the new program
	//but first wait for new process to copy our handles etc
	ktrace("execve original caller pausing for process copy\n");
	SuspendThread(GetCurrentThread());
	ktrace("execve original caller terminating\n");
	ExitProcess(0);
}
