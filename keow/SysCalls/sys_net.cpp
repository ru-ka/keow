#include "kernel.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 * int socketcall(int call, unsigned long *args);
 */
void  sys_socketcall(CONTEXT* pCtx)
{
	ktrace("IMPLEMENT sys_socketcall (unix domain socket?, not inet?)\n");
	pCtx->Eax = -ENOSYS;
}


/*****************************************************************************/


