#include "kernel.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 * we don't handle realtime signals
 * stub out here to remove IMPLEMENT... messages from ktrace
 */

void  sys_rt_sigreturn(CONTEXT* pCtx)		{pCtx->Eax = -ENOSYS;}
void  sys_rt_sigaction(CONTEXT* pCtx)		{pCtx->Eax = -ENOSYS;}
void  sys_rt_sigprocmask(CONTEXT* pCtx)		{pCtx->Eax = -ENOSYS;}
void  sys_rt_sigpending(CONTEXT* pCtx)		{pCtx->Eax = -ENOSYS;}
void  sys_rt_sigtimedwait(CONTEXT* pCtx)	{pCtx->Eax = -ENOSYS;}
void  sys_rt_sigqueueinfo(CONTEXT* pCtx)	{pCtx->Eax = -ENOSYS;}
void  sys_rt_sigsuspend(CONTEXT* pCtx)		{pCtx->Eax = -ENOSYS;}
