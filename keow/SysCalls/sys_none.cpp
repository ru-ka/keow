#include "kernel.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 * unhandled syscall
 */

void sys_unhandled(CONTEXT* pCtx)
{
	ktrace("NOT IMPLEMENTED: syscall %d %s\n", pCtx->Eax, ((int)pCtx->Eax)<NR_syscalls?syscall_names[pCtx->Eax]:"?");
	pCtx->Eax = -ENOSYS;
}


/*
 * all syscalls yet to be implemented are listed here
 * move them to their own files as coded
 */

void sys_waitpid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_creat(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mknod(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_chmod(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_lchown(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_lseek(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mount(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_umount(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setuid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_stime(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_alarm(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_oldfstat(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_pause(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_utime(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_stty(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_gtty(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_nice(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ftime(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sync(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_rename(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mkdir(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_rmdir(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_times(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_prof(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setgid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_signal(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_acct(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_umount2(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_lock(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mpx(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ulimit(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_oldolduname(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_chroot(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ustat(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setsid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sgetmask(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ssetmask(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sigsuspend(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sigpending(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sethostname(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setrlimit(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getrusage(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_settimeofday(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getgroups(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setgroups(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_symlink(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_oldlstat(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_uselib(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_swapon(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_readdir(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_truncate(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ftruncate(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fchmod(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fchown(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getpriority(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setpriority(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_profil(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_statfs(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fstatfs(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ioperm(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_syslog(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setitimer(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getitimer(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_olduname(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_iopl(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_vhangup(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_idle(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_vm86old(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_swapoff(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sysinfo(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ipc(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fsync(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sigreturn(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_clone(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setdomainname(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_modify_ldt(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_adjtimex(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_create_module(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_init_module(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_delete_module(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_get_kernel_syms(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_quotactl(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_bdflush(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sysfs(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_personality(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_afs_syscall(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setfsuid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setfsgid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getdents(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_flock(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_msync(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_readv(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getsid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fdatasync(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys__sysctl(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mlock(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_munlock(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mlockall(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_munlockall(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_setparam(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_getparam(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_setscheduler(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_getscheduler(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_yield(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_get_priority_max(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_get_priority_min(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_rr_get_interval(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mremap(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setresuid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getresuid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_vm86(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_query_module(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_poll(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_nfsservctl(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setresgid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getresgid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_prctl(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_pread(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_pwrite(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_chown(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_capget(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_capset(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sigaltstack(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sendfile(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getpmsg(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_putpmsg(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_vfork(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mmap2(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_truncate64(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_ftruncate64(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_lchown32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getgroups32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setgroups32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fchown32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setresuid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getresuid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setresgid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getresgid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_chown32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setuid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setgid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setfsuid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setfsgid32(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_pivot_root(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_mincore(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_madvise(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_madvise1(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fcntl64(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_security(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_gettid(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_readahead(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_setxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_lsetxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fsetxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_getxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_lgetxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fgetxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_listxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_llistxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_flistxattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_removexattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_lremovexattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_fremovexattr(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_tkill(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sendfile64(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_futex(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_setaffinity(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_sched_getaffinity(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_set_thread_area(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_get_thread_area(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_io_setup(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_io_destroy(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_io_getevents(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_io_submit(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_io_cancel(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_alloc_hugepages(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_free_hugepages(CONTEXT* pCtx)		{sys_unhandled(pCtx);}
void sys_exit_group(CONTEXT* pCtx)		{sys_unhandled(pCtx);}

