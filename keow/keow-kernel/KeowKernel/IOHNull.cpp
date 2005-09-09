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

// IOHNull.cpp: implementation of the 
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "IOHNull.h"

//////////////////////////////////////////////////////////////////////

IOHNull::IOHNull()
{
	HANDLE h = CreateFile("NUL:", GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

	DuplicateHandle(GetCurrentProcess(), h,
		P->m_Win32PInfo.hProcess, &m_hRemoteHandle,
		0, FALSE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
}

IOHNull::~IOHNull()
{
	SysCallDll::CloseHandle(m_hRemoteHandle);
}


bool IOHNull::Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags)
{
	return true;
}

HANDLE IOHNull::GetRemoteWriteHandle()
{
	return m_hRemoteHandle;
}

HANDLE IOHNull::GetRemoteReadHandle()
{
	return m_hRemoteHandle;
}

IOHandler* IOHNull::clone()
{
	IOHNull * pC = new IOHNull();
	return pC;
}

bool IOHNull::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFCHR);

	return true;
}


DWORD IOHNull::ioctl(DWORD request, DWORD data)
{
	return 0;
}
