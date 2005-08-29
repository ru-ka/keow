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

// File.cpp: implementation of the File class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "File.h"

//////////////////////////////////////////////////////////////////////

IOHFile::IOHFile(Path &path)
{
	m_RemoteHandle = INVALID_HANDLE_VALUE;
	m_Path = path;

	//don't open yet, use Open()
}

IOHFile::~IOHFile()
{
	SysCallDll::CloseHandle(m_RemoteHandle);
}

HANDLE IOHFile::GetRemoteWriteHandle()
{
	return m_RemoteHandle;
}
HANDLE IOHFile::GetRemoteReadHandle()
{
	return m_RemoteHandle;
}

IOHandler * IOHFile::clone()
{
	IOHFile * pF = new IOHFile(m_Path);
	pF->m_RemoteHandle = m_RemoteHandle;
	return pF;
}

bool IOHFile::Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags)
{
	//Open in the kernel, then move the handle to the user

	HANDLE h = CreateFile(m_Path.GetWin32Path(), win32access, win32share, 0, disposition, flags, 0);
	if(h==INVALID_HANDLE_VALUE)
		return false;

	//move handle to the user
	DuplicateHandle(GetCurrentProcess(), h, P->m_Win32PInfo.hProcess, &m_RemoteHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

	return true;
}

