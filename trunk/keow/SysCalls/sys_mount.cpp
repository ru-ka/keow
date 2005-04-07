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
 * int  mount(const char *source, const char *target, const char *filesystemtype,
 *				unsigned long mountflags, const void *data);
 */
void sys_mount(CONTEXT* pCtx)
{
	const char *source = (const char*)pCtx->Ebx;
	const char *target = (const char*)pCtx->Ecx;
	const char *filesystemtype = (const char*)pCtx->Edx;
	unsigned long mountflags = pCtx->Esi;
	const void *data = (const void*)pCtx->Edi;

	if(source==0
	|| target==0
	|| filesystemtype==0
	|| source[0]==0
	|| target[0]==0)
	{
		pCtx->Eax = EINVAL;
		return;
	}
	ktrace("mount request: '%s' on '%s' - type %s\n", source,target,filesystemtype);

	//the target must always be a directory
	Path p;
	p.SetUnixPath(target);

	DWORD attr = GetFileAttributes(p.Win32Path());
	if(attr==INVALID_FILE_ATTRIBUTES)
	{
		pCtx->Eax = ENOTDIR;
		return;
	}
	if((attr&FILE_ATTRIBUTE_DIRECTORY)==0)
	{
		pCtx->Eax = ENOTDIR;
		return;
	}


	//TODO: make these if's a lookup for plugins etc
	if(strcmp("keow", filesystemtype)==0)
	{
		//expect the source to be a valid win32 directory path
		//and we also need it to end in a backslash if it is just a drive letter
		if(source[1]==':' && source[2]==0) {
			//just drive letter, need path:  eg. C: bad,  C:\ is ok
			pCtx->Eax = ENOTDIR;
			return;
		}
		attr = GetFileAttributes(source);
		if(attr==INVALID_FILE_ATTRIBUTES)
		{
			pCtx->Eax = ENOTDIR;
			return;
		}
		if((attr&FILE_ATTRIBUTE_DIRECTORY)==0)
		{
			pCtx->Eax = ENOTDIR;
			return;
		}

		//for now we just keep a table of mounts - a fixed number are available
		if(pKernelSharedData->NumCurrentMounts >= MAX_MOUNTS)
		{
			pCtx->Eax = ENOMEM;
			return;
		}

		//try to perform the mount
		//no work to do for keow, just remember the mount point

		//record the mount
		MountPointDataStruct& mnt = pKernelSharedData->MountPoints[pKernelSharedData->NumCurrentMounts];
		strncpy(mnt.Destination, target, MAX_PATH);
		mnt.DestinatinLen = strlen(mnt.Destination); 
		if(mnt.Destination[mnt.DestinatinLen-1]=='/')
			mnt.Destination[mnt.DestinatinLen-1]=0; //dont' want to record the trailing slash - MakeWin32Path is easier that way
		strncpy(mnt.Source, source, MAX_PATH);
		mnt.SourceLen = strlen(mnt.Source); 
		mnt.Flags = mountflags;
		mnt.pType = "keow"; //const
		//data not required for this fs type
		pKernelSharedData->NumCurrentMounts++;

		pCtx->Eax = 0;
	}
	else
	{
		pCtx->Eax = ENODEV; //filesystem type not supported
		return;
	}
}

/*****************************************************************************/

/*
 * int umount(const char *target)
 */
void sys_umount(CONTEXT* pCtx)
{
	const char *target = (const char*)pCtx->Ebx;
	if(target==0)
	{
		pCtx->Eax = -EINVAL;
		return;
	}

	int len = strlen(target);
	if(target[len-1]=='/')
		len--; //don't want to match the slash

	int m;
	for(m=0; m<pKernelSharedData->NumCurrentMounts; ++m)
	{
		if(strcmp(target, pKernelSharedData->MountPoints[m].Destination)==0)
		{
			//remove this mount from the table,
			//move others to fill the space

			while(m<pKernelSharedData->NumCurrentMounts-1)
				pKernelSharedData->MountPoints[m] = pKernelSharedData->MountPoints[m+1];
			--pKernelSharedData->NumCurrentMounts;
			pCtx->Eax = 0;
			return;
		}
	}

	//nothing found
	pCtx->Eax = -EINVAL;
}

