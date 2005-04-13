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

#ifndef KEOW_KERNEL_H
#define KEOW_KERNEL_H

//want Win2k functions
#define _WIN32_WINNT 0x500


typedef unsigned char * ADDR;


#include "linux_includes.h"
#include "iohandler.h"
#include "memory.h"
#include "util.h"
#include "path.h"

#include <stdio.h>



#define MAX_SIGNALS			_NSIG
#define MAX_OPEN_FILES		1024
#define MAX_PROCESSES		1024
#define MAX_PENDING_SIGNALS	128

#define NUM_CONSOLE_TERMINALS	1
#define NUM_SERIAL_TERMINALS	1
#define NUM_PTY_TERMINALS		255
#define CONSOLE0_NUM	0
#define SERIAL0_NUM		(NUM_CONSOLE_TERMINALS)
#define PTY0_NUM		(SERIAL0_NUM+NUM_SERIAL_TERMINALS)
#define MAX_TERMINALS	(PTY0_NUM+NUM_PTY_TERMINALS)


class MemoryAllocRecord : public LinkedListItem {
public:
	ADDR addr;
	DWORD len;
	DWORD protection;
	
	enum RecType {
		Memory = 'm',
		MMap   = 'p'
	} type;

	//mmap data
	HANDLE hFileMapping, hOriginalFile;
	DWORD offset;
};


//
//Process specific data
//Put eveything that needs to survive a fork() here
//
struct ProcessDataStruct {
	DWORD Win32PID;
	DWORD MainThreadID;
	DWORD SignalThreadID;
	bool in_use;
	bool in_setup;				//still initialising process
	int exitcode;
	int killed_by_sig, current_signal;
	bool core_dumped;

	int uid, gid;				//real uid/gid
	int euid, egid;				//effective uid/gid
	int sav_uid, sav_gid;		//saved uid/gid

	char ProgramPath[MAX_PATH];
	char ** argv;				//argv and...
	char ** envp;				//... envp data locations
	ADDR phdr;					//ELF phdr 
	DWORD phent, phnum;			//ELF phdr fields
	ADDR program_base;			//base address is mem main program loaded at
	ADDR program_max;			//max address loaded for main program
	ADDR interpreter_base;		//base address is mem interpreter loaded at
	ADDR interpreter_max;		//max address loaded for interpreter
	ADDR interpreter_entry;		//interpreter start address
	ADDR program_entry;			//address of start of executable

	ADDR original_stack_esp;	//stack location (ESP)
	ADDR elf_start_esp;			//ESP when first transfer to elf code occurs
	CONTEXT fork_context;		//helper for fork - cpu registers
	DWORD fork_esp;				//helper for fork - current stack pointer
	struct {
		ADDR pvExcept;
		ADDR pvArbitrary;
	} fork_TIB;					//helper for fork - copy of some of the TIB

	int umask;

	ADDR brk_base;				//Program break - original
	ADDR brk;					//Program break - current

	char unix_pwd[MAX_PATH];

	linux::pid_t PID;
	linux::pid_t ParentPID;
	linux::pid_t ProcessGroupPID;

	linux::pid_t ptrace_owner_pid;	//who is ptrace()ing us (or zero)
	int ptrace_request;				//what tracer wants us to do
	bool ptrace_ctx_valid;			//next field has valid data right now?
	CONTEXT ptrace_ctx;				//for ptrace to have syscall contexts, a copy not a pointer
	DWORD ptrace_saved_eax;			//eax on entry to last syscall 
	int ptrace_new_signal;

	linux::sigaction signal_action[MAX_SIGNALS];
	linux::sigset_t sigmask[MAX_PENDING_SIGNALS];
	int signal_blocked[MAX_SIGNALS];
	int signal_depth;

	FILETIME StartedTime;

	//If you add or alter fields, also update all in forkexec.cpp and kernel.cpp


	//-------------------------------------------------------------------------

	//Data below this line cannot be safely duplicated (it is local pointers)
	//It is recorded here to allow fork() child process to use ReadMemory to get it

	IOHandler* FileHandlers[MAX_OPEN_FILES];

	HANDLE hKernelDebugFile;

	MemoryAllocRecord m_MemoryAllocationsHeader;

	//If you add or alter fields, also update all in forkexec.cpp


	//char PadingTo4k[1]; //we should do this and implement memory protection on the data;
};

extern ProcessDataStruct* pProcessData;


//
// info needed to be kept for terminal devices
//
struct TerminalDeviceDataStruct {
	linux::termios	TermIOs;
	DWORD			ProcessGroup;
	DWORD			InputState; //for shared escape code handling
	DWORD			OutputState; //for shared escape code handling
	BYTE			InputStateData[32];
	BYTE			OutputStateData[32];
};


//
// mount point information
//
#define MAX_MOUNTS 16
#define MAX_MOUNT_DATA 256

struct MountPointDataStruct {
	char Source[MAX_PATH];
	char Destination[MAX_PATH];
	int DestinatinLen, SourceLen;
	DWORD Flags;
	char Data[MAX_MOUNT_DATA]; //typically options
	int nFsHandler;	//index into FileSystemHandler::FileSystemTable
};


//
//shared kernel data
//this will be in shared memory across all processes
//
struct KernelSharedDataStruct {
	ProcessDataStruct ProcessTable[MAX_PROCESSES];

	int LastAllocatedPID;

	SYSTEMTIME BootTime;
	DWORD BogoMips;
	DWORD ForksSinceBoot;

	char KernelInstanceName[MAX_PATH];
	int KernelDebug;

	char LinuxFileSystemRoot[MAX_PATH];
	int  LinuxFileSystemRootLen;

	char ProcessStubFilename[MAX_PATH];

	TerminalDeviceDataStruct TerminalDeviceData[MAX_TERMINALS];

	MountPointDataStruct MountPoints[MAX_MOUNTS];
	int NumCurrentMounts;
};

extern KernelSharedDataStruct* pKernelSharedData;
extern HANDLE hKernelLock;


#define SIZE64k 0x10000
#define SIZE4k 0x1000


//c++ helper
#define instanceof(var,type) (dynamic_cast<type*>(var) != 0)


void kernel_init();
void kernel_term();
void ktrace(const char * format, ...);

int Win32ErrToUnixError(DWORD err);
int AllocatePID();
int FindFreeFD();

void MemoryDump(const char *msg, const void * from_addr, DWORD sz);
void GenerateCoreDump();

bool IsThreadSuspended(HANDLE hThread);
bool WaitForThreadToSuspend(HANDLE hThread);

_declspec(dllexport) bool SendSignal(int pid, int sig);


//These ones are 'extern C' to make them easier to load manually in ProcessStub
extern "C" _declspec(dllexport) void Process_Init(const char* keyword, int pid, const char * KernelInstance);
extern "C" _declspec(dllexport) void HandleSysCall( CONTEXT * pCtx );


typedef void (*SYSCALL_HANDLER)(CONTEXT* pCtx);

extern SYSCALL_HANDLER syscall_handlers[NR_syscalls];
extern const char* syscall_names[NR_syscalls];

extern HANDLE WaitTerminatingEvent;


//unix time base
#define FILETIME_TO_TIME_T(t) (unsigned long)( (((ULARGE_INTEGER*)&t)->QuadPart - Time1Jan1970.QuadPart) / 10000000L)
extern ULARGE_INTEGER Time1Jan1970;



//messages for signal thread
#define WM_KERNEL_SIGNAL			WM_USER+0x200		//a unix signal
#define WM_KERNEL_SETFORKCONTEXT	WM_USER+0x201		//used for fork


//syscall prototypes
#define PROTO_SYSCALL(n) void sys_##n(CONTEXT* pCtx)

PROTO_SYSCALL(unhandled);

PROTO_SYSCALL(exit);
PROTO_SYSCALL(fork);
PROTO_SYSCALL(read);
PROTO_SYSCALL(write);
PROTO_SYSCALL(open);
PROTO_SYSCALL(close);
PROTO_SYSCALL(waitpid);
PROTO_SYSCALL(creat);
PROTO_SYSCALL(link);
PROTO_SYSCALL(unlink);
PROTO_SYSCALL(execve);
PROTO_SYSCALL(chdir);
PROTO_SYSCALL(time);
PROTO_SYSCALL(mknod);
PROTO_SYSCALL(chmod);
PROTO_SYSCALL(lchown);
PROTO_SYSCALL(lseek);
PROTO_SYSCALL(getpid);
PROTO_SYSCALL(mount);
PROTO_SYSCALL(umount);
PROTO_SYSCALL(setuid);
PROTO_SYSCALL(getuid);
PROTO_SYSCALL(stime);
PROTO_SYSCALL(ptrace);
PROTO_SYSCALL(alarm);
PROTO_SYSCALL(oldfstat);
PROTO_SYSCALL(pause);
PROTO_SYSCALL(utime);
PROTO_SYSCALL(stty);
PROTO_SYSCALL(gtty);
PROTO_SYSCALL(access);
PROTO_SYSCALL(nice);
PROTO_SYSCALL(ftime);
PROTO_SYSCALL(sync);
PROTO_SYSCALL(kill);
PROTO_SYSCALL(rename);
PROTO_SYSCALL(mkdir);
PROTO_SYSCALL(rmdir);
PROTO_SYSCALL(dup);
PROTO_SYSCALL(pipe);
PROTO_SYSCALL(times);
PROTO_SYSCALL(prof);
PROTO_SYSCALL(brk);
PROTO_SYSCALL(setgid);
PROTO_SYSCALL(getgid);
PROTO_SYSCALL(signal);
PROTO_SYSCALL(geteuid);
PROTO_SYSCALL(getegid);
PROTO_SYSCALL(acct);
PROTO_SYSCALL(umount2);
PROTO_SYSCALL(lock);
PROTO_SYSCALL(ioctl);
PROTO_SYSCALL(fcntl);
PROTO_SYSCALL(mpx);
PROTO_SYSCALL(setpgid);
PROTO_SYSCALL(ulimit);
PROTO_SYSCALL(oldolduname);
PROTO_SYSCALL(umask);
PROTO_SYSCALL(chroot);
PROTO_SYSCALL(ustat);
PROTO_SYSCALL(dup2);
PROTO_SYSCALL(getppid);
PROTO_SYSCALL(getpgrp);
PROTO_SYSCALL(setsid);
PROTO_SYSCALL(sigaction);
PROTO_SYSCALL(sgetmask);
PROTO_SYSCALL(ssetmask);
PROTO_SYSCALL(setreuid);
PROTO_SYSCALL(setregid);
PROTO_SYSCALL(sigsuspend);
PROTO_SYSCALL(sigpending);
PROTO_SYSCALL(sethostname);
PROTO_SYSCALL(setrlimit);
PROTO_SYSCALL(getrlimit);	/* Back compatible);Gig limited rlimit */
PROTO_SYSCALL(getrusage);
PROTO_SYSCALL(gettimeofday);
PROTO_SYSCALL(settimeofday);
PROTO_SYSCALL(getgroups);
PROTO_SYSCALL(setgroups);
PROTO_SYSCALL(select);
PROTO_SYSCALL(symlink);
PROTO_SYSCALL(oldlstat);
PROTO_SYSCALL(readlink);
PROTO_SYSCALL(uselib);
PROTO_SYSCALL(swapon);
PROTO_SYSCALL(reboot);
PROTO_SYSCALL(readdir);
PROTO_SYSCALL(mmap);
PROTO_SYSCALL(munmap);
PROTO_SYSCALL(truncate);
PROTO_SYSCALL(ftruncate);
PROTO_SYSCALL(fchmod);
PROTO_SYSCALL(fchown);
PROTO_SYSCALL(getpriority);
PROTO_SYSCALL(setpriority);
PROTO_SYSCALL(profil);
PROTO_SYSCALL(statfs);
PROTO_SYSCALL(fstatfs);
PROTO_SYSCALL(ioperm);
PROTO_SYSCALL(socketcall);
PROTO_SYSCALL(syslog);
PROTO_SYSCALL(setitimer);
PROTO_SYSCALL(getitimer);
PROTO_SYSCALL(stat);
PROTO_SYSCALL(lstat);
PROTO_SYSCALL(fstat);
PROTO_SYSCALL(olduname);
PROTO_SYSCALL(iopl);
PROTO_SYSCALL(vhangup);
PROTO_SYSCALL(idle);
PROTO_SYSCALL(vm86old);
PROTO_SYSCALL(wait4);
PROTO_SYSCALL(swapoff);
PROTO_SYSCALL(sysinfo);
PROTO_SYSCALL(ipc);
PROTO_SYSCALL(fsync);
PROTO_SYSCALL(sigreturn);
PROTO_SYSCALL(clone);
PROTO_SYSCALL(setdomainname);
PROTO_SYSCALL(uname);
PROTO_SYSCALL(modify_ldt);
PROTO_SYSCALL(adjtimex);
PROTO_SYSCALL(mprotect);
PROTO_SYSCALL(sigprocmask);
PROTO_SYSCALL(create_module);
PROTO_SYSCALL(init_module);
PROTO_SYSCALL(delete_module);
PROTO_SYSCALL(get_kernel_syms);
PROTO_SYSCALL(quotactl);
PROTO_SYSCALL(getpgid);
PROTO_SYSCALL(fchdir);
PROTO_SYSCALL(bdflush);
PROTO_SYSCALL(sysfs);
PROTO_SYSCALL(personality);
PROTO_SYSCALL(afs_syscall); /* Syscall for Andrew File System */
PROTO_SYSCALL(setfsuid);
PROTO_SYSCALL(setfsgid);
PROTO_SYSCALL(_llseek);
PROTO_SYSCALL(getdents);
PROTO_SYSCALL(_newselect);
PROTO_SYSCALL(flock);
PROTO_SYSCALL(msync);
PROTO_SYSCALL(readv);
PROTO_SYSCALL(writev);
PROTO_SYSCALL(getsid);
PROTO_SYSCALL(fdatasync);
PROTO_SYSCALL(_sysctl);
PROTO_SYSCALL(mlock);
PROTO_SYSCALL(munlock);
PROTO_SYSCALL(mlockall);
PROTO_SYSCALL(munlockall);
PROTO_SYSCALL(sched_setparam);
PROTO_SYSCALL(sched_getparam);
PROTO_SYSCALL(sched_setscheduler);
PROTO_SYSCALL(sched_getscheduler);
PROTO_SYSCALL(sched_yield);
PROTO_SYSCALL(sched_get_priority_max);
PROTO_SYSCALL(sched_get_priority_min);
PROTO_SYSCALL(sched_rr_get_interval);
PROTO_SYSCALL(nanosleep);
PROTO_SYSCALL(mremap);
PROTO_SYSCALL(setresuid);
PROTO_SYSCALL(getresuid);
PROTO_SYSCALL(vm86);
PROTO_SYSCALL(query_module);
PROTO_SYSCALL(poll);
PROTO_SYSCALL(nfsservctl);
PROTO_SYSCALL(setresgid);
PROTO_SYSCALL(getresgid);
PROTO_SYSCALL(prctl);
PROTO_SYSCALL(rt_sigreturn);
PROTO_SYSCALL(rt_sigaction);
PROTO_SYSCALL(rt_sigprocmask);
PROTO_SYSCALL(rt_sigpending);
PROTO_SYSCALL(rt_sigtimedwait);
PROTO_SYSCALL(rt_sigqueueinfo);
PROTO_SYSCALL(rt_sigsuspend);
PROTO_SYSCALL(pread);
PROTO_SYSCALL(pwrite);
PROTO_SYSCALL(chown);
PROTO_SYSCALL(getcwd);
PROTO_SYSCALL(capget);
PROTO_SYSCALL(capset);
PROTO_SYSCALL(sigaltstack);
PROTO_SYSCALL(sendfile);
PROTO_SYSCALL(getpmsg);	/* some people actually want streams */
PROTO_SYSCALL(putpmsg);	/* some people actually want streams */
PROTO_SYSCALL(vfork);
PROTO_SYSCALL(ugetrlimit);	/* SuS compliant getrlimit */
PROTO_SYSCALL(mmap2);
PROTO_SYSCALL(truncate64);
PROTO_SYSCALL(ftruncate64);
PROTO_SYSCALL(stat64);
PROTO_SYSCALL(lstat64);
PROTO_SYSCALL(fstat64);
PROTO_SYSCALL(lchown32);
PROTO_SYSCALL(getuid32);
PROTO_SYSCALL(getgid32);
PROTO_SYSCALL(geteuid32);
PROTO_SYSCALL(getegid32);
PROTO_SYSCALL(setreuid32);
PROTO_SYSCALL(setregid32);
PROTO_SYSCALL(getgroups32);
PROTO_SYSCALL(setgroups32);
PROTO_SYSCALL(fchown32);
PROTO_SYSCALL(setresuid32);
PROTO_SYSCALL(getresuid32);
PROTO_SYSCALL(setresgid32);
PROTO_SYSCALL(getresgid32);
PROTO_SYSCALL(chown32);
PROTO_SYSCALL(setuid32);
PROTO_SYSCALL(setgid32);
PROTO_SYSCALL(setfsuid32);
PROTO_SYSCALL(setfsgid32);
PROTO_SYSCALL(pivot_root);
PROTO_SYSCALL(mincore);
PROTO_SYSCALL(madvise);
PROTO_SYSCALL(madvise1);	/* delete when C lib stub is removed */
PROTO_SYSCALL(getdents64);
PROTO_SYSCALL(fcntl64);
PROTO_SYSCALL(security);	/* syscall for security modules */
PROTO_SYSCALL(gettid);
PROTO_SYSCALL(readahead);
PROTO_SYSCALL(setxattr);
PROTO_SYSCALL(lsetxattr);
PROTO_SYSCALL(fsetxattr);
PROTO_SYSCALL(getxattr);
PROTO_SYSCALL(lgetxattr);
PROTO_SYSCALL(fgetxattr);
PROTO_SYSCALL(listxattr);
PROTO_SYSCALL(llistxattr);
PROTO_SYSCALL(flistxattr);
PROTO_SYSCALL(removexattr);
PROTO_SYSCALL(lremovexattr);
PROTO_SYSCALL(fremovexattr);
PROTO_SYSCALL(tkill);
PROTO_SYSCALL(sendfile64);
PROTO_SYSCALL(futex);
PROTO_SYSCALL(sched_setaffinity);
PROTO_SYSCALL(sched_getaffinity);
PROTO_SYSCALL(set_thread_area);
PROTO_SYSCALL(get_thread_area);
PROTO_SYSCALL(io_setup);
PROTO_SYSCALL(io_destroy);
PROTO_SYSCALL(io_getevents);
PROTO_SYSCALL(io_submit);
PROTO_SYSCALL(io_cancel);
PROTO_SYSCALL(alloc_hugepages);
PROTO_SYSCALL(free_hugepages);
PROTO_SYSCALL(exit_group);


#endif //KEOW_KERNEL_H
