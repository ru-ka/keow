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

class FilesystemProc : public Filesystem  
{
public:
	FilesystemProc();
	virtual ~FilesystemProc();

	virtual IOHandler * CreateIOHandler(Path& path);
	virtual string GetPathSeperator();
	virtual bool IsSymbolicLink(string& strPath);
	virtual string GetLinkDestination(string& strPath);
	virtual bool IsRelativePath(string& strPath);
};

#endif // !defined(AFX_FILESYSTEMPROC_H__04DF9EB5_9771_4497_B1FF_89F772093E50__INCLUDED_)