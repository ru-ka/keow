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

// IOHandler.h: interface for the IOHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOHANDLER_H__BDA9DB15_5F09_45C4_9607_507D02C38ACD__INCLUDED_)
#define AFX_IOHANDLER_H__BDA9DB15_5F09_45C4_9607_507D02C38ACD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class IOHandler  
{
public:
	IOHandler();
	virtual ~IOHandler();

	static IOHandler* CreateForPath(Path& path);

	virtual bool Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags) = 0;

	virtual HANDLE GetRemoteWriteHandle() = 0;
	virtual HANDLE GetRemoteReadHandle() = 0;

	virtual IOHandler* clone() = 0;
};

#endif // !defined(AFX_IOHANDLER_H__BDA9DB15_5F09_45C4_9607_507D02C38ACD__INCLUDED_)
