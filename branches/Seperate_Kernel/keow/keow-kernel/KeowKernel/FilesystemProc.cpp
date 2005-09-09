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

//////////////////////////////////////////////////////////////////////

FilesystemProc::FilesystemProc()
{

}

FilesystemProc::~FilesystemProc()
{

}


IOHandler * FilesystemProc::CreateIOHandler(Path& path)
{
	//everything in this filesystem should be a proc object?
	DebugBreak();
	return NULL; //new IOHFile(path);
}

string FilesystemProc::GetPathSeperator()
{
	return "/";
}

bool FilesystemProc::IsSymbolicLink(string& strPath)
{
	//no sym links is procfs?
	return false;
}

string FilesystemProc::GetLinkDestination(string& strPath)
{
	//no sym links is procfs?
	return strPath;
}

bool FilesystemProc::IsRelativePath(string& strPath)
{
	return strPath[0]!='/';
}

