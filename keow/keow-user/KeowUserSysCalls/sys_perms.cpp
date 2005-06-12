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

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value

void sys_getuid(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->uid;
}

void sys_geteuid(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->euid;
}

void  sys_getgid(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->gid;
}

void  sys_getegid(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->egid;
}


void sys_getuid32(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->uid;
}

void sys_geteuid32(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->euid;
}

void  sys_getgid32(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->gid;
}

void  sys_getegid32(CONTEXT* pCtx)
{
	pCtx->Eax = pProcessData->egid;
}

/*****************************************************************************/

/*
 * int setreuid(int ruid, int euid)
 */
void sys_setreuid(CONTEXT* pCtx)
{
	int ruid = pCtx->Ebx;
	int euid = pCtx->Ecx;

	pCtx->Eax = -EPERM;

	if(ruid != -1)
	{
		//only root can set real uid
		if(pProcessData->uid == 0
		|| pProcessData->euid == 0)
		{
			pProcessData->uid = ruid;
			pCtx->Eax = 0;
		}
	}

	if(euid != -1)
	{
		//root can set to anything
		//users can set to any existing id
		if(pProcessData->uid == 0
		|| pProcessData->euid == 0
		|| euid == pProcessData->uid
		|| euid == pProcessData->euid
		|| euid == pProcessData->sav_uid )
		{
			pProcessData->euid = euid;
			pCtx->Eax = 0;
		}
	}

}

/*
 * int setregid(int rgid, int egid)
 */
void sys_setregid(CONTEXT* pCtx)
{
	int rgid = pCtx->Ebx;
	int egid = pCtx->Ecx;

	pCtx->Eax = -EPERM;

	if(rgid != -1)
	{
		//only root can set real gid
		if(pProcessData->uid == 0
		|| pProcessData->euid == 0)
		{
			pProcessData->gid = rgid;
			pCtx->Eax = 0;
		}
	}

	if(egid != -1)
	{
		//root can set to anything
		//users can set to any existing id
		if(pProcessData->uid == 0
		|| pProcessData->euid == 0
		|| egid == pProcessData->gid
		|| egid == pProcessData->egid
		|| egid == pProcessData->sav_gid )
		{
			pProcessData->egid = egid;
			pCtx->Eax = 0;
		}
	}

}

void sys_setreuid32(CONTEXT* pCtx)
{
	sys_setreuid(pCtx);
}

void sys_setregid32(CONTEXT* pCtx)
{
	sys_setregid(pCtx);
}
