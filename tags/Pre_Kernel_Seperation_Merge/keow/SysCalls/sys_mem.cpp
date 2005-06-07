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
#include "memory.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value



/*
 * unsigned long brk(unsigned long end_data_segment)
 *
 * In Linux the brk (program break) is the top of the data segment of the process
 * It starts at process creation time as the end of the bss segment,
 * and continues to grow (and shrink) from there.
 */
void  sys_brk(CONTEXT* pCtx)
{
	ADDR p;
	ADDR old_brk = pProcessData->brk;
	ADDR new_brk = (ADDR)pCtx->Ebx;

	if(new_brk == 0)
	{
		//return current location
		ktrace("brk(0) = 0x%08lx\n", pProcessData->brk);
		pCtx->Eax = (DWORD)pProcessData->brk;
		return;
	}


	//only allow growing for now
	//allocate all memory between old brk and new brk
	if(new_brk < old_brk)
	{
		pCtx->Eax = -1; //failed
		return;
	}
	if(new_brk > old_brk)
	{
		ktrace("brk requested break @ 0x%08lx\n", new_brk);
		p = AllocateMemAndProtect(old_brk, new_brk-old_brk, PAGE_EXECUTE_READWRITE);//PAGE_READWRITE);
		if(p==(ADDR)-1)
		{
			pCtx->Eax = -1;
			ktrace("out of memory in sys_brk?\n");
			return;
		}
		//dont do this or else things fail
		//ZeroMemory(old_brk, new_brk-old_brk);
	}

	pProcessData->brk = new_brk;
	ktrace("brk(x) = 0x%08lx\n", pProcessData->brk);
	pCtx->Eax = (DWORD)new_brk;
	return;
}


/*****************************************************************************/


namespace linux {
	struct mmap_arg_struct {
		unsigned long addr;
		unsigned long len;
		unsigned long prot;
		unsigned long flags;
		unsigned long fd;
		unsigned long offset;
	};
};

/*
 * unsigned long sys_mmap(mmap_arg_struct* args)
 *
 * Map a section of a file into memory, or alternatively a peice of swap file (no fd). 
 */
void sys_mmap(CONTEXT* pCtx)
{
	linux::mmap_arg_struct * args = (linux::mmap_arg_struct *)pCtx->Ebx;
	DWORD err = -EINVAL;
	HANDLE hMap, hFile;
	DWORD ProtOpen, ProtMap;
	void *p;
	DWORD dwFileSize;
	FileIOHandler * ioh = NULL;

	if(args->fd != -1)
	{
		if(args->fd<0 || args->fd>MAX_OPEN_FILES)
		{
			pCtx->Eax = -EBADF;
			return;
		}
		ioh = dynamic_cast<FileIOHandler*>(pProcessData->FileHandlers[args->fd]);
		if(ioh==NULL)
		{
			pCtx->Eax = -EACCES; //not a simple file - can't handle that
			return;
		}

		ULONGLONG len = ioh->Length();
		if(len>LONG_MAX)
			dwFileSize = LONG_MAX;
		else
			dwFileSize = (DWORD)len;
	}


	//If an address or offset is supplied, then it is probably 
	// 4k aligned (linux x86 page size). However win32 needs uses
	// a 64k boundary for file mappings. If they don't match then
	// MapViewOfFile cannot work.
	// However.. often mmap is a shortcut to loading an executable
	// into memory (readonly at a fixed address, so just read the 
	// file and ignore that we won't get any future updates.
	// If copy-on-write was requested then this means we get our own 
	// copy of the data anyhow so just read it and take the extra
	// swap space hit and IO hit.
	// Also linux allows(?) mmap larger than the underlying file
	// size to get extra memory. Win32 needs to grow the file to do
	// this and we don't want that, so read the file then too.
	// And! linux seems to alloc mmap over the top of an old map
	// area, which can't happen in win32. Therefore unless we
	// need to write (not copy-on-write) to the map, then always
	// allocate memory and read to get around all these problems.
	if((args->addr & 0xFFFF00000) != args->addr	    /*64k test*/
	|| (args->offset & 0xFFFF0000) != args->offset  /*64k test*/
	|| ((args->fd != -1) && (args->offset+args->len) > dwFileSize)  /*mmap larger than file*/
	|| ((args->prot & PROT_WRITE)==0 || (args->flags & MAP_SHARED)==0)  /* ReadOnly or private map (not write shared) */
	) {

		//can't handle shared write access in these scenarios
		if((args->prot & PROT_WRITE) && (args->flags & MAP_SHARED))
		{
			pCtx->Eax = -EINVAL; //invalid arguments
			return;
		}

		//allocate the requested mem size
		if(args->prot & PROT_WRITE)
			ProtMap = PAGE_READWRITE;
		else
			ProtMap = PAGE_READONLY;
		p = AllocateMemAndProtect((ADDR)args->addr, args->len, PAGE_READWRITE); //correct prot after we write mem
		if(p==(void*)-1)
		{
			pCtx->Eax = -ENOMEM;
			return;
		}

		int len4k = (args->len+0xFFF) & ~0xFFF;
		ZeroMemory(p, len4k);

		//read file into the memory (unless using swap)
		if(ioh != NULL)
		{
			DWORD dwRead;
			DWORD dwReadLen;

			dwReadLen = args->len;
			if(args->offset+args->len > dwFileSize)
				dwReadLen = dwFileSize - args->offset;

			if(ioh->Seek(args->offset, FILE_BEGIN)==INVALID_SET_FILE_POINTER)
			{
				pCtx->Eax = -Win32ErrToUnixError(GetLastError());
				return;
			}
			if(!ioh->Read(p, dwReadLen, &dwRead) )
			{
				pCtx->Eax = -Win32ErrToUnixError(GetLastError());
				return;
			}
		}

//can't change protection within the 64k once set?
//		VirtualProtect(p, args->len, ProtMap, &ProtOpen);
		pCtx->Eax = (DWORD)p;
		return;
	}



	//the following is standard win32 file mappings
	

	// memory so we can just read it ourselves.
	// we don't get any future updates 
	// mapping 
	//protection needed
	if(args->prot & PROT_WRITE)
	{
		if(args->flags & MAP_PRIVATE)
		{
			ProtOpen = PAGE_WRITECOPY;
			ProtMap = FILE_MAP_COPY;
		}
		else
		{
			ProtOpen = PAGE_READWRITE;
			ProtMap = FILE_MAP_WRITE;
		}
	}
	else
	{
		ProtOpen = PAGE_READONLY;
		ProtMap = FILE_MAP_READ;
	}


	//if fd is invalid, use swapfile
	if(ioh==NULL)
		hFile=INVALID_HANDLE_VALUE;
	else
		hFile=ioh->GetHandle();

	//map the file
	hMap = CreateFileMapping(hFile, NULL, ProtOpen, 0, args->offset+args->len, NULL);
	if(hMap==NULL)
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}
	p = MapViewOfFileEx(hMap, ProtMap, 0,args->offset, args->len, (void*)args->addr);
	if(p==0)
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	RecordMMapAllocation(hFile, hMap, args->offset, (ADDR)p, args->len, ProtMap);
	//leave open - CloseHandle(hMap);

	//opened - return actual addr
	pCtx->Eax = (DWORD)p;
	return;
}

/*****************************************************************************/

/*
 * void munmap(void* start, unsigned long len)
 */
void  sys_munmap(CONTEXT* pCtx)
{
	ADDR addr = (ADDR)pCtx->Ebx;
	DWORD len = pCtx->Ecx;

	//try Unmap first (may not actually be a map - see sys_old_mmap)
	if(UnmapViewOfFile(addr))
	{
		RecordMMapFree(addr,len);
	}
	else
	{
		if(!DeallocateMemory(addr, len))
		{
			pCtx->Eax = -Win32ErrToUnixError(GetLastError());
			return;
		}
	}
	pCtx->Eax = 0;
}


/*****************************************************************************/

/*
 * int mprotect(const void *addr, size_t len, int prot)
 */
void  sys_mprotect(CONTEXT* pCtx)
{
	void* addr = (void*)pCtx->Ebx;
	DWORD len = pCtx->Ecx;
	DWORD prot = pCtx->Edx;

	//for now we are not doing protection
	pCtx->Eax = 0;
}
