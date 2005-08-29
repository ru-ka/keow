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

// MemoryHelper.cpp: implementation of the MemoryHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "MemoryHelper.h"

//////////////////////////////////////////////////////////////////////

MemoryHelper::MemoryHelper()
{
}

MemoryHelper::~MemoryHelper()
{
}


ADDR MemoryHelper::AllocateMemAndProtectProcess(HANDLE hProcess, ADDR addr, DWORD size, DWORD prot)
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


ADDR MemoryHelper::AllocateMemAndProtect(ADDR addr, DWORD size, DWORD prot)
{
	ADDR addrActual = AllocateMemAndProtectProcess(P->m_Win32PInfo.hProcess, addr, size, prot);
	if(addrActual != (ADDR)-1)
		P->m_MemoryAllocations.push_back(new Process::MemoryAlloc(addrActual, size, prot));
	return addrActual;
}

bool MemoryHelper::DeallocateMemory(ADDR addr, DWORD size)
{
	if(!VirtualFree(addr, size, MEM_DECOMMIT))
		return false;

	Process::MemoryAllocationsList::iterator it;
	for(it=P->m_MemoryAllocations.begin();
	    it!=P->m_MemoryAllocations.end();
		++it)
	{
		Process::MemoryAlloc * pAlloc = *it;
		if(pAlloc->addr == addr
		&& pAlloc->len == size)
		{
			P->m_MemoryAllocations.erase(it);
			break;
		}
	}

	return true;
}


/*****************************************************************************/

bool MemoryHelper::ProcessReadWriteMemory(HANDLE hProcess, LPVOID pRemoteAddr, LPVOID pLocalAddr, DWORD len, DWORD * pDone, ReadWrite rw )
{
	/* It's not documented (anywhere I could find) but Read/Write ProcessMemory() calls
	   appear to fail if either te local or remote addresses cross a 4k boundary.
	   Therefore this routine has to transfer memory in chunks to ensure
	   that neither end does.

	   Example: | is a 4k boundary, . is not accessed, * is copied

		local buffer   | . . . * | * * * * | * * * * | * * * . |
                            ___|   ----- |
                           /    ____/   /
                          /    /    ___/
	                     - -----   /
		remote buffer  | * * * * | * * * * | * * * * | . . . . |

		Basically, divide the local buffer into 4k chunks and then transfer those
		chunks (possibly in two goes) with the remote process.
	 */

	DWORD pLocal = (DWORD)pLocalAddr;
	DWORD pRemote = (DWORD)pRemoteAddr;

	DWORD dwCopied = 0;
	DWORD dwAmountLeft = len;
	while(dwAmountLeft != 0)
	{
		DWORD dwNext4k;
		DWORD dwXfer;
		BOOL bRet;

		//determine which peice of the local buffer to copy
		dwNext4k = (pLocal & 0xFFFFF000) + SIZE4k;
		DWORD dwLocalToCopy = dwNext4k - pLocal;

		if(dwLocalToCopy > dwAmountLeft)
			dwLocalToCopy = dwAmountLeft;


		//determine first block of the remote copy

		dwNext4k = (pRemote & 0xFFFFF000) + SIZE4k;
		DWORD dwRemoteToCopy = dwNext4k - pRemote;

		if(dwRemoteToCopy > dwLocalToCopy)
			dwRemoteToCopy = dwLocalToCopy;

		if(rw == Read)
			bRet = ReadProcessMemory(hProcess, (LPVOID)pRemote, (LPVOID)pLocal, dwRemoteToCopy, &dwXfer);
		else
			bRet = WriteProcessMemory(hProcess, (LPVOID)pRemote, (LPVOID)pLocal, dwRemoteToCopy, &dwXfer);
		if(bRet==FALSE)
		{
			DWORD err = GetLastError();
			ktrace("error 0x%lx in xfer program segment\n", err);
			//keep trying //return false;
		}
		pLocal += dwRemoteToCopy;
		pRemote += dwRemoteToCopy;
		dwCopied += dwXfer;

		//determine second block of the remote copy
		//we already did part of a 4k block, so the rest will be less than 4k and
		//easily fit now.

		dwRemoteToCopy = dwLocalToCopy - dwRemoteToCopy; //all the rest

		if(dwRemoteToCopy!=0)
		{
			if(rw == Read)
				bRet = ReadProcessMemory(hProcess, (LPVOID)pRemote, (LPVOID)pLocal, dwRemoteToCopy, &dwXfer);
			else
				bRet = WriteProcessMemory(hProcess, (LPVOID)pRemote, (LPVOID)pLocal, dwRemoteToCopy, &dwXfer);
			if(bRet==FALSE)
			{
				DWORD err = GetLastError();
				ktrace("error 0x%lx in xfer program segment\n", err);
				//keep trying //return false;
			}
			pLocal += dwRemoteToCopy;
			pRemote += dwRemoteToCopy;
			dwCopied += dwXfer;
		}

		dwAmountLeft -= dwLocalToCopy;
	}

	if(pDone)
		*pDone = dwCopied;

	return true;
}

bool MemoryHelper::WriteMemory(HANDLE hToProcess, ADDR toAddr, int len, ADDR fromAddr)
{
	DWORD dwWritten;
	if(!ProcessReadWriteMemory(hToProcess, toAddr, fromAddr, len, &dwWritten, Write))
	{
		DWORD err = GetLastError();
		ktrace("error 0x%lx in write program segment\n", err);
		return false;
	}
	return true;
}

bool MemoryHelper::ReadMemory(ADDR toAddr, HANDLE hFromProcess, ADDR fromAddr, int len)
{
	DWORD dwRead;
	if(!ProcessReadWriteMemory(hFromProcess, fromAddr, toAddr, len, &dwRead, Read))
	{
		DWORD err = GetLastError();
		ktrace("error 0x%lx in read program segment\n", err);
		return false;
	}
	return true;
}

/*
 * Copy memory between two managed processes
 */
bool MemoryHelper::TransferMemory(HANDLE hFromProcess, ADDR fromAddr, HANDLE hToProcess, ADDR toAddr, int len)
{
	BYTE buf[SIZE4k];

	for(int i=0; i<len; i+=sizeof(buf))
	{
		int copy = sizeof(buf);
		if(i+copy > len)
			copy = len - i;

		ReadMemory(buf, hFromProcess, fromAddr+i, copy);
		WriteMemory(hToProcess, toAddr+i, copy, buf);
	}
	return true;
}

/*****************************************************************************/

/*
 * Copy a string list (eg argv[]) between two processes
 * Return addr to use in the 'to' process (eg as argv)
 */
ADDR MemoryHelper::CopyStringListBetweenProcesses(HANDLE hFromProcess, ADDR pFromList, HANDLE hToProcess, DWORD * pdwCount)
{
	//calculate how large the data is
	DWORD count, datasize;

	ADDR pFrom;
	list<DWORD> string_sizes;
	list<ADDR> string_addresses;

	count=datasize=0;

	pFrom=pFromList;
	if(pFrom)
	{
		for(;;)
		{
			ADDR pStr;
			ReadMemory((ADDR)&pStr, hFromProcess, pFrom, sizeof(ADDR));
			if(pStr==NULL)
				break; //end of list

			string_addresses.push_back(pStr);

			//see how big the string is
			string s;
			s = ReadString(hFromProcess, pStr);
			DWORD strLen = s.length()+1; //include the null
			string_sizes.push_back(strLen);

			datasize += strLen;
			count++;

			pFrom += sizeof(ADDR);
		}
	}

	//how many strings?
	if(pdwCount)
		*pdwCount = count;

	//allocate space for the strings AND the list of pointers
	DWORD total = datasize + sizeof(ADDR)*(count+1); //+1 for the null array terminater
	ADDR pTo;

	pTo = AllocateMemAndProtectProcess(hToProcess, 0, total, PAGE_EXECUTE_READWRITE);

	//copy the data, keeping track of pointers used
	ADDR pToArray = pTo;
	ADDR pToData = pTo + sizeof(ADDR)*(count+1); //write data here;
	list<DWORD>::iterator itSize;
	list<ADDR>::iterator itAddr;
	itSize=string_sizes.begin();
	itAddr=string_addresses.begin();
	while(itSize!=string_sizes.end())
	{
		//array entry (copy over value of pToData)
		WriteMemory(hToProcess, pToArray, sizeof(ADDR), (ADDR)&pToData);
		pToArray += sizeof(ADDR);

		//data
		TransferMemory(hFromProcess, *itAddr, hToProcess, pToData, *itSize);
		pToData += *itSize;

		//next (they are paired)
		++itSize;
		++itAddr;
	}
	//null terminate
	pToData = 0; //just a zero dword to write
	WriteMemory(hToProcess, pToArray, sizeof(ADDR), (ADDR)&pToData);

	return pTo; //array started here
}


string MemoryHelper::ReadString(HANDLE hFromProcess, ADDR fromAddr)
{
	string str;

	//read in 4k allocation blocks (to be faster than reading bytes)
	DWORD next4k;
	char buf[SIZE4k];
	DWORD len;
	DWORD addr;

	addr = (DWORD)fromAddr;
	for(;;)
	{
		next4k = (addr & 0xFFFFF000) + SIZE4k;
		len = next4k - addr;

		ReadMemory((ADDR)buf, hFromProcess, (ADDR)addr, len);

		for(DWORD i=0; i<len; ++i)
		{
			if(buf[i]==0)
				return str;

			str += buf[i];
		}

		//next block
		addr += len;
	}
}
