// KernelTable.cpp: implementation of the KernelTable class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "KernelTable.h"

#include "SysCalls.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

KernelTable::KernelTable()
{
	m_DebugLevel = 0;
	m_pRootMountPoint = 0;
	m_LastPID = 0;

	GetSystemTime(&m_BootTime);

	//simple - probably doesn't match the kernel
	m_BogoMips = 0;
	const DWORD msSample = 1000;
	DWORD dwEnd = GetTickCount() + msSample;
	while(GetTickCount() < dwEnd) {
		Sleep(0);
		m_BogoMips++;
	}
	m_BogoMips /= msSample;
	ktrace("keow bogomips: %ld\n", m_BogoMips);

	SysCalls::InitSysCallTable();
}

KernelTable::~KernelTable()
{

}



//Search for and return the process for a pid
//
Process * KernelTable::FindProcess(DWORD pid)
{
	//TODO: more efficient search

	KernelTable::ProcessList::iterator it;
	for(it=g_pKernelTable->m_Processes.begin(); it!=g_pKernelTable->m_Processes.end(); ++it)
	{
		if((*it)->m_Pid == pid)
			return *it;
	}

	return NULL;
}
