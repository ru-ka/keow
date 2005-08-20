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

#undef DEF_SYSCALL
}


void SysCalls::HandleInt80SysCall(Process& P, CONTEXT &ctx)
{
	// eax is the syscall number
	// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
	// any more parameters and the caller just puts a struct pointer in one of these
	// eax is the return value
	SetLastError(0);
	DWORD syscall = ctx.Eax;

	if(syscall < NR_syscalls)
	{
		ktrace("debug: syscall %d [%s] from @ 0x%08lx\n", ctx.Eax, syscall_names[ctx.Eax], ctx.Eip);

		if(P.m_ptrace.OwnerPid
		&& P.m_ptrace.Request == PTRACE_SYSCALL )
		{
			ktrace("stopping for ptrace on syscall entry eax=%d\n", syscall);

			//entry to syscall has eax as -ENOSYS
			//original eax is available as saved value

			P.m_ptrace.Saved_Eax = syscall;
			P.m_ptrace.ctx.Eax = (DWORD)-ENOSYS; //this is what ptrace in the tracer sees as eax

			P.SendSignal(SIGTRAP);
		}


		///
		///
		syscall_handlers[syscall](P, ctx);
		///
		///

		
		if(P.m_ptrace.OwnerPid
		&& P.m_ptrace.Request == PTRACE_SYSCALL )
		{
			ktrace("stopping for ptrace on syscall exit eax=%d, orig=%d\n", ctx.Eax, syscall);

			//return from syscall has eax as return value
			//original eax is available as saved value

			P.m_ptrace.ctx = ctx; //new state
			P.m_ptrace.Saved_Eax = syscall;

			P.SendSignal(SIGTRAP);
		}
	}
	else
	{
		ktrace("debug: bad syscall %d [???] from @ 0x%08lx\n", syscall, ctx.Eip);
		sys_unhandled(P, ctx);
	}

	ktrace("debug: syscall return (Eax=0x%lx,%ld)\n", ctx.Eax, ctx.Eax);
}

//////////////////////////////////////////////////////////////////////////////


void SysCalls::sys_unhandled(Process &P, CONTEXT &ctx)
{
	ktrace("NOT IMPLEMENTED: syscall %d %s\n", ctx.Eax, ((int)ctx.Eax)<NR_syscalls?syscall_names[ctx.Eax]:"?");
	ctx.Eax = -ENOSYS;
}

void SysCalls::sys_exit(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fork(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_read(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

/*
 * write bytes to a handle
 * sys_write(handle,text,len)
 */
void SysCalls::sys_write(Process &P, CONTEXT &ctx)
{
	DWORD dwWritten;
	int fd;
	File * f;
	
	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	f = P.m_OpenFiles[fd];
	if(f == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}


	//Do the write in the user process
	P.InvokeStubFunction(P.m_StubFunctionsInfo.Write, ctx.Ecx, ctx.Edx, dwWritten);

	if(!f->Write((void*)ctx.Ecx, ctx.Edx, &dwWritten))
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	ctx.Eax = dwWritten;
}

void SysCalls::sys_open(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_close(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_waitpid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_creat(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_link(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_unlink(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_execve(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_chdir(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_time(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mknod(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_chmod(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lchown(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lseek(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getpid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mount(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_umount(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setuid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getuid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_stime(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ptrace(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_alarm(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_oldfstat(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_pause(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_utime(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_stty(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_gtty(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_access(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_nice(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ftime(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sync(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_kill(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rename(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mkdir(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rmdir(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_dup(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_pipe(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_times(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_prof(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

/*
 * unsigned long brk(unsigned long end_data_segment)
 *
 * In Linux the brk (program break) is the top of the data segment of the process
 * It starts at process creation time as the end of the bss segment,
 * and continues to grow (and shrink) from there.
 */
void SysCalls::sys_brk(Process &P, CONTEXT &ctx)
{
	ADDR p;
	ADDR old_brk = P.m_ElfLoadData.brk;
	ADDR new_brk = (ADDR)ctx.Ebx;

	if(new_brk == 0)
	{
		//return current location
		ctx.Eax = (DWORD)P.m_ElfLoadData.brk;
		ktrace("brk(0) = 0x%08lx\n", ctx.Eax);
		return;
	}


	//only allow growing for now
	//allocate all memory between old brk and new brk
	if(new_brk < old_brk)
	{
		ctx.Eax = -1; //failed
		return;
	}
	if(new_brk > old_brk)
	{
		ktrace("brk requested break @ 0x%08lx\n", new_brk);
		p = MemoryHelper::AllocateMemAndProtect(old_brk, new_brk-old_brk, PAGE_EXECUTE_READWRITE);//PAGE_READWRITE);
		if(p==(ADDR)-1)
		{
			ctx.Eax = -1;
			ktrace("out of memory in sys_brk?\n");
			return;
		}
		//dont do this or else things fail!!
		//ZeroMemory(old_brk, new_brk-old_brk);
	}

	P.m_ElfLoadData.brk = new_brk;
	ktrace("brk(x) = 0x%08lx\n", new_brk);
	ctx.Eax = (DWORD)new_brk;
	return;
}

void SysCalls::sys_setgid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getgid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_signal(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_geteuid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getegid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_acct(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_umount2(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lock(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ioctl(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fcntl(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mpx(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setpgid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ulimit(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_oldolduname(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_umask(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_chroot(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ustat(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_dup2(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getppid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getpgrp(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setsid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sigaction(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sgetmask(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ssetmask(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setreuid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setregid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sigsuspend(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sigpending(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sethostname(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setrlimit(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getrlimit(Process &P, CONTEXT &ctx)	/* Back compatible(Process &P, CONTEXT &ctx)Gig limited rlimit */
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getrusage(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_gettimeofday(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_settimeofday(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getgroups(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setgroups(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_select(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_symlink(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_oldlstat(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_readlink(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_uselib(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_swapon(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_reboot(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_readdir(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mmap(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_munmap(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_truncate(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ftruncate(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fchmod(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fchown(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getpriority(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setpriority(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_profil(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_statfs(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fstatfs(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ioperm(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_socketcall(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_syslog(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setitimer(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getitimer(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_stat(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lstat(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fstat(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_olduname(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_iopl(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_vhangup(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_idle(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_vm86old(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_wait4(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_swapoff(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sysinfo(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ipc(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fsync(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sigreturn(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_clone(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setdomainname(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_uname(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_modify_ldt(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_adjtimex(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mprotect(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sigprocmask(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_create_module(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_init_module(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_delete_module(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_get_kernel_syms(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_quotactl(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getpgid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fchdir(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_bdflush(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sysfs(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_personality(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_afs_syscall(Process &P, CONTEXT &ctx) /* Syscall for Andrew File System */
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setfsuid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setfsgid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys__llseek(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getdents(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys__newselect(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_flock(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_msync(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_readv(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_writev(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getsid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fdatasync(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys__sysctl(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mlock(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_munlock(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mlockall(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_munlockall(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_setparam(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_getparam(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_setscheduler(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_getscheduler(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_yield(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_get_priority_max(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_get_priority_min(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_rr_get_interval(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_nanosleep(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mremap(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setresuid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getresuid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_vm86(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_query_module(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_poll(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_nfsservctl(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setresgid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getresgid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_prctl(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rt_sigreturn(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rt_sigaction(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rt_sigprocmask(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rt_sigpending(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rt_sigtimedwait(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rt_sigqueueinfo(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_rt_sigsuspend(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_pread(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_pwrite(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_chown(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getcwd(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_capget(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_capset(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sigaltstack(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sendfile(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getpmsg(Process &P, CONTEXT &ctx)	/* some people actually want streams */
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_putpmsg(Process &P, CONTEXT &ctx)	/* some people actually want streams */
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_vfork(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ugetrlimit(Process &P, CONTEXT &ctx)	/* SuS compliant getrlimit */
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mmap2(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_truncate64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_ftruncate64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_stat64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lstat64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fstat64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lchown32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getuid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getgid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_geteuid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getegid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setreuid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setregid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getgroups32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setgroups32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fchown32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setresuid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getresuid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setresgid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getresgid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_chown32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setuid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setgid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setfsuid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setfsgid32(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_pivot_root(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_mincore(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_madvise(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_madvise1(Process &P, CONTEXT &ctx)	/* delete when C lib stub is removed */
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getdents64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fcntl64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_security(Process &P, CONTEXT &ctx)	/* syscall for security modules */
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_gettid(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_readahead(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_setxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lsetxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fsetxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_getxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lgetxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fgetxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_listxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_llistxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_flistxattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_removexattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_lremovexattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_fremovexattr(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_tkill(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sendfile64(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_futex(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_setaffinity(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_sched_getaffinity(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_set_thread_area(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_get_thread_area(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_io_setup(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_io_destroy(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_io_getevents(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_io_submit(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_io_cancel(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_alloc_hugepages(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_free_hugepages(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}

void SysCalls::sys_exit_group(Process &P, CONTEXT &ctx)
{
	sys_unhandled(P, ctx);
}


