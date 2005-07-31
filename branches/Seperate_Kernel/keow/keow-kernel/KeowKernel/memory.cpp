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
 * allocate and deallocation of memory, preserving details for fork() etc.
 */
#include "kernel.h"
#include "memory.h"


ADDR AllocateMemAndProtectProcess(HANDLE hProcess, ADDR addr, DWORD size, DWORD prot)
{
	ADDR p;
	ADDR tmp64k ,tmp4k;
	MEMORY_BASIC_INFORMATION mbi;
	ADDR end_addr;
//	DWORD oldprot;

	if(size<0)
	{
		return (ADDR)-1;
	}

	//if no addr, then we can assign one
	if(addr==0)
	{
		addr = (ADDR)VirtualAllocEx(hProcess, NULL, size, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	}
	end_addr = addr + size;

	//ensure reserved first
	//reservation is in 64k pages
	//may already be allocated
	tmp64k = (ADDR)((DWORD)addr & 0xFFFF0000);
	while(tmp64k < end_addr)
	{
		VirtualQueryEx(hProcess, tmp64k, &mbi, sizeof(mbi));
		if(mbi.State == MEM_FREE)
		{
			//allocate with full perms, individual pages can lock it down as required
			p = (ADDR)VirtualAllocEx(hProcess, tmp64k, SIZE64k, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if(p!=tmp64k)
			{
				return (ADDR)-1;
			}
		}
		tmp64k += SIZE64k;
	}

	//commit pages from reserved area
	//must allocate on a page boundary (4k for Win32)
	tmp4k = (ADDR)((DWORD)addr & 0xFFFFF000);
	while(tmp4k < end_addr)
	{
		VirtualQueryEx(hProcess, (void*)tmp4k, &mbi, sizeof(mbi));
		if(mbi.State == MEM_RESERVE)
		{
//seems this cannot be changed within the 64k successfully?
//			p = VirtualAlloc((void*)tmp4k, SIZE4k, MEM_COMMIT, prot);
			p = (ADDR)VirtualAllocEx(hProcess, (void*)tmp4k, SIZE4k, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if(p!=tmp4k)
			{
				return (ADDR)-1;
			}
		}
//		VirtualProtect((void*)tmp64k, SIZE4k, prot, &oldprot); //in case re-using old area
		tmp4k += SIZE4k;
	}

	//ktrace("AllocateMemAndProtect [h%lX] 0x%08lx -> 0x%08lx\n", hProcess, addr, end_addr);

	return addr;
}


ADDR AllocateMemAndProtect(ADDR addr, DWORD size, DWORD prot)
{
	ADDR addrActual = AllocateMemAndProtectProcess(GetCurrentProcess(), addr, size, prot);
	if(addrActual != (ADDR)-1)
		RecordMemoryAllocation(addrActual, size, prot);
	return addrActual;
}

bool DeallocateMemory(ADDR addr, DWORD size)
{
	if(!VirtualFree(addr, size, MEM_DECOMMIT))
		return false;
	RecordMemoryFree(addr,size);
	return true;
}

/*****************************************************************************/

static void AddMemoryAllocRecord(MemoryAllocRecord::RecType type, ADDR addr, DWORD len, DWORD prot, HANDLE hFile, HANDLE hMap, DWORD offset)
{
	//find where in the list to add
	//keep the list in order of memory addresses
	//join consecutive entries as one entry
	MemoryAllocRecord *pPrev, *pCurr, *pNew;

	pPrev = &KeowProcess()->m_MemoryAllocationsHeader;
	pCurr = (MemoryAllocRecord *)KeowProcess()->m_MemoryAllocationsHeader.next;
	while(pCurr)
	{
		if(pCurr->addr >= addr)
			break;	//add before this one

		pPrev = pCurr;
		pCurr = (MemoryAllocRecord *)pCurr->next;
	}

	//adding after pPrev (before pCurr)
	pNew = new MemoryAllocRecord;

	pNew->addr = addr;
	pNew->len = len;
	pNew->protection = prot;

	pNew->type = type;

	pNew->hOriginalFile = hFile;
	pNew->hFileMapping = hMap;

	pNew->offset = offset;

	pNew->AddAfter(pPrev);

	ktrace("recorded memory allocate type %c @ 0x%08lx len x%lx\n", type, addr, len);

	//prev just changed
	pPrev = pNew;
	pCurr = (MemoryAllocRecord *)pNew->next;
	if(pCurr==NULL)
		return; //last block won't span others

	//new block may span others?
	while(pCurr->addr <= (pPrev->addr + pPrev->len)
	&&    pCurr->type == pPrev->type)
	{
		//pPrev (new block) spans pCurr
		ADDR CurrEnd = pCurr->addr + pCurr->len;
		pPrev->len = CurrEnd - pCurr->addr;
		pCurr->Remove();

		ktrace("coalesed to allocate type %c @ 0x%08lx len x%lx\n", pPrev->type, pPrev->addr, pPrev->len);

		pCurr = (MemoryAllocRecord *)pPrev->next;
		if(pCurr==NULL)
			break;
	}
}

void FreeMemoryAllocRecord(int type, ADDR addr, DWORD len)
{
	//find allocation block that covers this area
	MemoryAllocRecord * pMem = &KeowProcess()->m_MemoryAllocationsHeader;
	while(pMem->next)
	{
		pMem = (MemoryAllocRecord*)pMem->next;
		if(pMem->type != type)
			continue;

		if(addr == pMem->addr
		&& len == pMem->len)
		{
			//free entire block
			pMem->Remove();
			return;
		}
		else
		if(addr == pMem->addr
		&& len < pMem->len)
		{
			//free first part of block
			pMem->len -= len;
			pMem->addr += len;
			return;
		}
		else
		if(addr > pMem->addr
		&& addr+len == pMem->addr+pMem->len)
		{
			//free last part of block
			pMem->len -= len;
			return;
		}
		else
		if(addr > pMem->addr
		&& addr+len < pMem->addr+pMem->len)
		{
			//free middle of block

			//new block so we can split this one
			MemoryAllocRecord * pMemNew = new MemoryAllocRecord;

			//new block keeps end of old block
			pMemNew->addr = pMem->addr+pMem->len;
			pMemNew->len = (pMem->addr + pMem->len) - (addr + len);
			pMemNew->protection = pMem->protection;
			pMemNew->type = pMem->type;
			pMemNew->hFileMapping = pMem->hFileMapping;
			pMemNew->hOriginalFile = pMem->hOriginalFile;
			pMemNew->offset = pMem->offset;

			//orignal block keeps beginning
			pMem->len = addr - pMem->addr;

			//link in new block
			pMemNew->AddAfter(pMem);
			return;
		}
	}

	ktrace("FAIL: unable to record deallocation if memory\n");
}

void RecordMMapAllocation(HANDLE hFile, HANDLE hMap, DWORD offset, ADDR addr, DWORD len, DWORD prot)
{
	AddMemoryAllocRecord(MemoryAllocRecord::RecType::MMap, addr, len, prot, hFile, hMap, offset);
}
void RecordMMapFree(ADDR addr, DWORD len)
{
	FreeMemoryAllocRecord(MemoryAllocRecord::RecType::MMap,addr,len);
}

void RecordMemoryAllocation(ADDR addr, DWORD len, DWORD prot)
{
	AddMemoryAllocRecord(MemoryAllocRecord::RecType::Memory,addr,len,prot,0,0,0);
}
void RecordMemoryFree(ADDR addr, DWORD len)
{
	FreeMemoryAllocRecord(MemoryAllocRecord::RecType::Memory,addr,len);
}


/*****************************************************************************/

bool WriteMemory(HANDLE hToProcess, ADDR toAddr, int len, ADDR fromAddr)
{
	int offset, size, write;
	DWORD dwWritten;

	for(offset=0,size=len; size>0; offset+=write,size-=write)
	{
		write = size>SIZE4k ? SIZE4k : size;
		if( (((DWORD)toAddr+offset) & 0x00000FFF) != 0)
		{
			//write needs to not cross a 4k boundary?
			int bytes_left = 0x1000 - (((DWORD)toAddr+offset) & 0x00000FFF);
			if(bytes_left < write)
				write = bytes_left;
		}
		if(!WriteProcessMemory(hToProcess, toAddr+offset, fromAddr+offset, write, &dwWritten))
		{
			DWORD err = GetLastError();
			ktrace("error 0x%lx in write program segment\n", err);
			return false;
		}
	}
	return true;
}

bool ReadMemory(ADDR toAddr, HANDLE hFromProcess, ADDR fromAddr, int len)
{
	int offset, size, read;
	DWORD dwRead;

	for(offset=0,size=len; size>0; offset+=read,size-=read)
	{
		read = size>SIZE4k ? SIZE4k : size;
		if( (((DWORD)fromAddr+offset) & 0x00000FFF) != 0)
		{
			//write needs to not cross a 4k boundary?
			int bytes_left = 0x1000 - (((DWORD)fromAddr+offset) & 0x00000FFF);
			if(bytes_left < read)
				read = bytes_left;
		}
		if(!ReadProcessMemory(hFromProcess, fromAddr+offset, toAddr+offset, read, &dwRead))
		{
			DWORD err = GetLastError();
			ktrace("error 0x%lx in read program segment\n", err);
			return false;
		}
	}
	return true;
}

/*****************************************************************************/

/*
 * copy a string list (eg argv[]) to process
 * return addr that process can use (eg as argv)
 * the 2 dwords immediatly prior to the returned address
 * are a struct MemBlock.
 */
char ** CopyStringListToProcess(HANDLE hProcess, char * list[] )
{
	int i, arraysize, items;
	DWORD total;
	ADDR p;

	if(list==NULL)
		return NULL;

	//need to know how big the list is
	total=0;
	for(items=0; list[items]!=0; ++items)
		total += 1 + strlen(list[items]);
	items++; //include the last null item
	arraysize = sizeof(char*) * items;
	total += arraysize + sizeof(struct MemBlock);

	//allocate memory in the dest process
	ADDR addr = AllocateMemAndProtectProcess(hProcess, 0, total, PAGE_EXECUTE_READWRITE);
	if(addr==(ADDR)-1)
		return (char**)addr;

	//local copy of pointers - copy last
	char ** newlist = new char* [items];

	//copy strings
	p = addr;
	for(i=0; i<items; ++i)
	{
		if(list[i]==0)
		{
			newlist[i] = 0;
		}
		else
		{
			int len = 1 + strlen(list[i]);
			if(!WriteMemory(hProcess, p, len+1, (ADDR)list[i]))
			{
				addr = (ADDR)-1;
				break;
			}
			newlist[i] = (char*)p;
			p += len;
		}
	}
	//copy memblock info
	MemBlock mb;
	mb.addr = addr;
	mb.len = total;
	if(!WriteMemory(hProcess, p, sizeof(mb), (ADDR)&mb))
		p = (ADDR)-1;
	else
	{
		p+=sizeof(mb); //becomes the pointer we actually return
		//copy array of pointers to data we just copied
		if(!WriteMemory(hProcess, p, arraysize, (ADDR)newlist))
			p = (ADDR)-1;
	}

	delete newlist;
	return (char**)p;
}
