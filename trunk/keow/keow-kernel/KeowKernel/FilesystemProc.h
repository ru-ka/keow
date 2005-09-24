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

// FilesystemProc.h: interface for the FilesystemProc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILESYSTEMPROC_H__04DF9EB5_9771_4497_B1FF_89F772093E50__INCLUDED_)
#define AFX_FILESYSTEMPROC_H__04DF9EB5_9771_4497_B1FF_89F772093E50__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Filesystem.h"
#include "FilesystemGenericStatic.h"

class FilesystemProc : public FilesystemGenericStatic
{
public:
	FilesystemProc();
	virtual ~FilesystemProc();

	static Process* PidStrToProcess(const char * pid);

	static const int Jiffies;
	static DWORD ToJiffies(FILETIME * ftBase, FILETIME * ftNow);
	static void CalcCpuTimingsInJiffies(DWORD *uptime, DWORD *user, DWORD *system, DWORD *nice, DWORD *idle);

protected: //handlers
	static void GetPids(DirEnt64List& lst);
	static void GetOpenFiles(DirEnt64List& lst, const char * pid);

	static IOHandler* Get_MemInfo();
	static IOHandler* Get_CpuInfo();
	static IOHandler* Get_Uptime();
	static IOHandler* Get_Stat();
	static IOHandler* Get_Mounts();
	static IOHandler* Get_LoadAvg();

	static IOHandler* Get_Pid_Cmdline(const char * pid);
	static IOHandler* Get_Pid_Cwd(const char * pid);
	static IOHandler* Get_Pid_Environ(const char * pid);
	static IOHandler* Get_Pid_Exe(const char * pid);
	static IOHandler* Get_Pid_Maps(const char * pid);
	static IOHandler* Get_Pid_Mem(const char * pid);
	static IOHandler* Get_Pid_Root(const char * pid);
	static IOHandler* Get_Pid_Stat(const char * pid);
	static IOHandler* Get_Pid_Statm(const char * pid);
	static IOHandler* Get_Pid_Status(const char * pid);

	static IOHandler* Get_Pid_Fd(const char * pid, const char * fd);
};

#endif // !defined(AFX_FILESYSTEMPROC_H__04DF9EB5_9771_4497_B1FF_89F772093E50__INCLUDED_)
