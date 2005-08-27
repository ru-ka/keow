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

IOHFile::IOHFile(Path path)
{
	m_RemoteHandle = INVALID_HANDLE_VALUE;
	m_Path = path;

	DebugBreak();
//	CreateFile(path.GetWin32Path(), access, share, 0, OPEN_EXISTING, flags, 0);
//	DuplicateHandle(GetCurrentProcess(), h, P->g_pkSysCallDll::CreateFile(path.GetWin32Path().c_str);
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
