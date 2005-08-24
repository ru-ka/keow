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

// KernelTable.cpp: implementation of the KernelTable class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "KernelTable.h"
#include "IOHNtConsole.h"
#include "SysCalls.h"

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

	//main console
	m_pMainConsole = new IOHNtConsole("Main Console");

}

KernelTable::~KernelTable()
{

}



//Search for and return the process for a pid
//
Process * KernelTable::FindProcess(PID pid)
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
