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
	ADDR addrActual = AllocateMemAndProtectProcess(GetCurrentProcess(), addr, size, prot);
	if(addrActual != (ADDR)-1)
		g_pKernelThreadLocals->pProcess->m_MemoryAllocations.push_back(new Process::MemoryAlloc(addrActual, size, prot));
	return addrActual;
}

bool MemoryHelper::DeallocateMemory(ADDR addr, DWORD size)
{
	if(!VirtualFree(addr, size, MEM_DECOMMIT))
		return false;

	Process::MemoryAllocationsList::iterator it;
	for(it=g_pKernelThreadLocals->pProcess->m_MemoryAllocations.begin();
	    it!=g_pKernelThreadLocals->pProcess->m_MemoryAllocations.end();
		++it)
	{
		Process::MemoryAlloc * pAlloc = *it;
		if(pAlloc->addr == addr
		&& pAlloc->len == size)
		{
			g_pKernelThreadLocals->pProcess->m_MemoryAllocations.erase(it);
			break;
		}
	}

	return true;
}


/*****************************************************************************/

bool MemoryHelper::WriteMemory(HANDLE hToProcess, ADDR toAddr, int len, ADDR fromAddr)
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

bool MemoryHelper::ReadMemory(ADDR toAddr, HANDLE hFromProcess, ADDR fromAddr, int len)
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

bool MemoryHelper::TransferMemory(HANDLE hFromProcess, ADDR fromAddr, HANDLE hToProcess, ADDR toAddr, int len)
{
	BYTE buf[1024];

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

			//see how big the string is
			//TODO read in blocks for efficiency
			BYTE data;
			DWORD strLen = 0;
			string_addresses.push_back(pStr);
			for(;;)
			{
				ReadMemory(&data, hFromProcess, pStr, sizeof(data));
				if(data==NULL)
					break; //end of string
				strLen++;
				datasize++;
				pStr++;
			}
			datasize++; //include the null
			strLen++; //include the null
			string_sizes.push_back(strLen);
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
		//array entry
		WriteMemory(hToProcess, pToArray, sizeof(ADDR), pToData);
		pToArray += sizeof(ADDR);

		//data
		TransferMemory(hFromProcess, *itAddr, hToProcess, pTo, *itSize);
		pToData += *itSize;

		//next (they are paired)
		++itSize;
		++itAddr;
	}
	//null terminate
	pToArray = 0;
	WriteMemory(hToProcess, pTo, sizeof(ADDR), (ADDR)&pToArray);

	return pTo; //array started here
}
