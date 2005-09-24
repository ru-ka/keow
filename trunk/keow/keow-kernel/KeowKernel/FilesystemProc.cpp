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

// FilesystemProc.cpp: implementation of the FilesystemProc class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "FilesystemProc.h"
#include "IOHStaticData.h"

//////////////////////////////////////////////////////////////////////

FilesystemProc::FilesystemProc()
: FilesystemGenericStatic("proc")
{
	AddLister("/", GetPids);

	AddFile("/meminfo", Get_MemInfo);
	AddFile("/*/exe", Get_Pid_Exe);
}

FilesystemProc::~FilesystemProc()
{
}


void FilesystemProc::GetPids(DirEnt64List& lst)
{
	linux::dirent64 de;

	KernelTable::ProcessList::iterator it;
	for(it=g_pKernelTable->m_Processes.begin();
	    it!=g_pKernelTable->m_Processes.end();
		++it)
	{
		de.d_ino = 0; //dummy value
		de.d_type = 0; //not provided on linux x86 32bit?  (GetUnixFileType(p);

		StringCbPrintf(de.d_name, sizeof(de.d_name), "%d", (*it)->m_Pid);
	
		lst.push_back(de);
	}
}

IOHandler* FilesystemProc::Get_MemInfo()
{
	IOHStaticData * ioh = new IOHStaticData(true);

	ioh->AddData("hello");

	return ioh;
}

IOHandler* FilesystemProc::Get_Pid_Exe(const char * pid)
{
	int n = atoi(pid);
	Process * pp = g_pKernelTable->FindProcess(n);

	IOHStaticData * ioh = new IOHStaticData(true);

	ioh->AddData(pp->m_ProcessFileImage.GetUnixPath().c_str());

	return ioh;
}
