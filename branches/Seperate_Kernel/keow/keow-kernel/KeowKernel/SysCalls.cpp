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

// SysCalls.cpp: implementation of the SysCalls class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "SysCalls.h"

SysCalls::SYSCALL_HANDLER SysCalls::syscall_handlers[NR_syscalls];
const char* SysCalls::syscall_names[NR_syscalls];

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


//////////////////////////////////////////////////////////////////////

void SysCalls::InitSysCallTable()
{
	//default all syscalls to unhandled
	for(int i=0; i<NR_syscalls; ++i)
		syscall_handlers[i] = Unhandled;

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

#undef DEF_SYSCALL
}


void SysCalls::HandleInt80SysCall(CONTEXT &ctx)
{
	// eax is the syscall number
	// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
	// any more parameters and the caller just puts a struct pointer in one of these
	// eax is the return value
	SetLastError(0);
	DWORD syscall = ctx.Eax;

	ktrace("\n"); //just get some blank lines to break up output between syscalls

	if(syscall < NR_syscalls)
	{
		ktrace("debug: syscall %d [%s] from @ 0x%08lx\n", ctx.Eax, syscall_names[ctx.Eax], ctx.Eip);

		if(P->m_ptrace.OwnerPid
		&& P->m_ptrace.Request == PTRACE_SYSCALL )
		{
			ktrace("stopping for ptrace on syscall entry eax=%d\n", syscall);

			//entry to syscall has eax as -ENOSYS
			//original eax is available as saved value

			P->m_ptrace.Saved_Eax = syscall;
			P->m_ptrace.ctx.Eax = (DWORD)-ENOSYS; //this is what ptrace in the tracer sees as eax

			P->SendSignal(SIGTRAP);
		}


		///
		///
		syscall_handlers[syscall](ctx);
		///
		///

		
		if(P->m_ptrace.OwnerPid
		&& P->m_ptrace.Request == PTRACE_SYSCALL )
		{
			ktrace("stopping for ptrace on syscall exit eax=%d, orig=%d\n", ctx.Eax, syscall);

			//return from syscall has eax as return value
			//original eax is available as saved value

			P->m_ptrace.ctx = ctx; //new state
			P->m_ptrace.Saved_Eax = syscall;

			P->SendSignal(SIGTRAP);
		}
	}
	else
	{
		ktrace("debug: bad syscall %d [???] from @ 0x%08lx\n", syscall, ctx.Eip);
		Unhandled(ctx);
	}

	ktrace("debug: syscall return (Eax=0x%lx,%ld)\n", ctx.Eax, ctx.Eax);
}

//////////////////////////////////////////////////////////////////////////////


void SysCalls::Unhandled(CONTEXT &ctx)
{
	ktrace("NOT IMPLEMENTED: syscall %d %s\n", ctx.Eax, ((int)ctx.Eax)<NR_syscalls?syscall_names[ctx.Eax]:"?");
	//DebugBreak();
	ctx.Eax = -ENOSYS;
}




/*
 * all syscalls yet to be implemented are listed here
 * move them to their own files as coded
 */

void SysCalls::sys_waitpid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_creat(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_mknod(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_chmod(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_lchown(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setuid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_stime(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_alarm(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_oldfstat(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_pause(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_utime(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_stty(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_gtty(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_nice(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ftime(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sync(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_times(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_prof(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setgid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_signal(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_acct(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_umount2(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_lock(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_mpx(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ulimit(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_oldolduname(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_chroot(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ustat(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setsid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sgetmask(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ssetmask(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sigsuspend(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sigpending(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sethostname(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setrlimit(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getrusage(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_settimeofday(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getgroups(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setgroups(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_symlink(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_oldlstat(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_uselib(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_swapon(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_readdir(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_truncate(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ftruncate(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fchmod(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fchown(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getpriority(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setpriority(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_profil(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_statfs(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fstatfs(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ioperm(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_syslog(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setitimer(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getitimer(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_olduname(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_iopl(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_vhangup(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_idle(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_vm86old(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_swapoff(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sysinfo(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ipc(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fsync(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sigreturn(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_clone(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setdomainname(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_modify_ldt(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_adjtimex(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_create_module(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_init_module(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_delete_module(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_get_kernel_syms(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_quotactl(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_bdflush(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sysfs(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_personality(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_afs_syscall(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setfsuid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setfsgid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getdents(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_flock(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_msync(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_readv(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getsid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fdatasync(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys__sysctl(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_mlock(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_munlock(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_mlockall(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_munlockall(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_setparam(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_getparam(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_setscheduler(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_getscheduler(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_yield(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_get_priority_max(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_get_priority_min(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_rr_get_interval(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_mremap(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setresuid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getresuid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_vm86(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_query_module(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_poll(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_nfsservctl(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setresgid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getresgid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_prctl(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_pread(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_pwrite(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_chown(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_capget(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_capset(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sigaltstack(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sendfile(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getpmsg(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_putpmsg(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_vfork(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_mmap2(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_truncate64(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_ftruncate64(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_lchown32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getgroups32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setgroups32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fchown32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setresuid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getresuid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setresgid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getresgid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_chown32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setuid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setgid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setfsuid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setfsgid32(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_pivot_root(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_mincore(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_madvise(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_madvise1(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fcntl64(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_security(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_gettid(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_readahead(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_setxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_lsetxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fsetxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_getxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_lgetxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fgetxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_listxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_llistxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_flistxattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_removexattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_lremovexattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_fremovexattr(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_tkill(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sendfile64(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_futex(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_setaffinity(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_sched_getaffinity(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_set_thread_area(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_get_thread_area(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_io_setup(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_io_destroy(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_io_getevents(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_io_submit(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_io_cancel(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_alloc_hugepages(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_free_hugepages(CONTEXT &ctx)		{Unhandled(ctx);}
void SysCalls::sys_exit_group(CONTEXT &ctx)		{Unhandled(ctx);}

