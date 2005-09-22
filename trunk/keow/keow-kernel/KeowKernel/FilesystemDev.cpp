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

// FilesystemDev.cpp: implementation of the FilesystemDev class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "FilesystemDev.h"

#include "IOHNull.h"
#include "IOHNtConsole.h"

//////////////////////////////////////////////////////////////////////

FilesystemDev::FilesystemDev()
{

}

FilesystemDev::~FilesystemDev()
{

}


IOHandler * FilesystemDev::CreateIOHandler(Path& path)
{
	//everything in this filesystem should be a dev object?

	string what = path.GetPathInFilesystem();
	if(what == "/null")
	{
		return new IOHNull();
	}
	else
	if(what == "/tty")
	{
		//for now everyone uses the console
		return new IOHNtConsole(g_pKernelTable->m_pMainConsole);
	}
	else
	{
		ktrace("implement /dev path: %s\n", what.c_str());
		return NULL;
	}
}

string FilesystemDev::GetPathSeperator()
{
	return "/";
}

bool FilesystemDev::IsSymbolicLink(string& strPath)
{
	//no sym links is /dev?
	return false;
}

string FilesystemDev::GetLinkDestination(string& strPath)
{
	//no sym links is /dev?
	return "";
}

bool FilesystemDev::IsRelativePath(string& strPath)
{
	return strPath[0]!='/';
}

