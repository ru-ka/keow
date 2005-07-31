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

#include "kernel.h"
#include "path.h"
#include "filesys.h"



Path::Path(bool FollowSymLinks)
{
	m_UnixPath[0] = 0;
	m_Win32Path[0] = 0;
	m_Win32Calculated = false;
	m_FollowSymLinks = FollowSymLinks;
	m_nMountPoint = -1; //none, root
	m_nUnixPathMountPath = 0;
}
Path::~Path()
{
	//never free m_FsHandler - we don't own it
}


const char * Path::Win32Path()
{
	CalculateWin32Path();
	return m_Win32Path;
}
const char * Path::UnixPath()
{
	return m_UnixPath;
}
const char * Path::CurrentFileSystemUnixPath()
{
	return &m_UnixPath[m_nUnixPathMountPath];
}


void Path::SetUnixPath(const char * unixp)
{
	if(unixp[0] != '/')
		strncpy(m_UnixPath, KeowProcess()->unix_pwd, MAX_PATH-1);
	else
		m_UnixPath[0] = 0;

	AddUnixPath(unixp);
}

void Path::AddUnixPath(const char * unixp)
{
	if(unixp[0] == '/')
		m_UnixPath[0] = 0;

	int len = strlen(m_UnixPath);
	if(len>0 && m_UnixPath[len-1]!='/') {
		m_UnixPath[len]='/';
		m_UnixPath[len+1]=0;
	}

	strncat(m_UnixPath, unixp, MAX_PATH-strlen(m_UnixPath));

	CollapseUnixPath();
}


void Path::FollowSymLinks(bool follow)
{
	m_FollowSymLinks = follow;
	m_Win32Calculated = false;
}


//resolve /.. and /. references
void Path::CollapseUnixPath()
{
	ktrace("collapse %s\n", m_UnixPath);

	char * p1 = m_UnixPath;
	char * p2 = p1;

	for(;;)
	{
		*p1 = *p2;
		if(*p1 == 0)
			break;

		while(strncmp("//", p2, 2)==0) {
			//skip redundant slashes
			p2++;
		}

		if(strcmp("/.", p2)==0
		|| strncmp("/./", p2, 3)==0) {
			//skip redundant reference to current directory
			p2+=2;
			continue;
		}

		if(strcmp("/..", p2)==0
		|| strncmp("/../", p2, 4)==0) {
			//back-up one directory. cannot go past root
			p2+=3;
			while(p1>m_UnixPath) {
				*p1=0;
				--p1;
				if(*p1=='/')
					break;
			}
			continue;
		}

		++p1;
		++p2;
	}

	ktrace(" to %s\n", m_UnixPath);
}


/*
 * Using LinuxFileSystem root kernel parameter to build correct win32 path
 * Also take care of kernel "mount points".
 */
void Path::CalculateWin32Path()
{
	if(m_Win32Calculated)
		return;

	//always start here
	StringCbCopy(m_Win32Path, MAX_PATH, g_KernelData.LinuxFileSystemRoot);

	m_nMountPoint = -1; //none, root
	FileSystemHandler* h = FileSystemHandler::RootFileSystemHandler;

	//apply requested path
	char *pStart = m_UnixPath;
	char *pEnd;
	int nOldMount = m_nMountPoint;
	do {
		while(*pStart == '/')
			pStart++; //don't want the slash

		pEnd = strchr(pStart, '/');
		if(pEnd==0)
			pEnd = pStart + strlen(pStart);

		//null terminate so we can use pStart as a string
		char save = *pEnd;
		*pEnd = 0;

		h->ApplyPathElement(*this, pStart);

		//get correct handler
		if(m_nMountPoint == -1)
			h = FileSystemHandler::RootFileSystemHandler;
		else
			h = FileSystemHandler::Get( g_KernelData.MountPoints[m_nMountPoint].nFsHandler );

		ktrace("fs:%s, ApplyPathElement:%s => %s\n", h->Name(), pStart, m_Win32Path);

		//restore
		*pEnd = save;

		//new mount?
		if(m_nMountPoint != nOldMount)
		{
			//first directory in the new mount
			m_nUnixPathMountPath = pEnd - m_UnixPath;

			nOldMount = m_nMountPoint;
		}

		pStart = pEnd; //next char to process
	} while(*pStart != 0); //not null terminator

	ktrace("%s -> [%s] %s\n", m_UnixPath, h->Name(), m_Win32Path);
	m_Win32Calculated = true;
}


FileSystemHandler * Path::GetFileSystemHandler()
{
	CalculateWin32Path();

	FileSystemHandler* h = FileSystemHandler::RootFileSystemHandler;
	if(m_nMountPoint != -1)
		h = FileSystemHandler::Get( g_KernelData.MountPoints[m_nMountPoint].nFsHandler );

	return h;
}


IOHandler* Path::CreateIOHandler()
{
	CalculateWin32Path();
	return GetFileSystemHandler()->CreateIOHandler(*this);
}

bool Path::IsSymbolicLink()
{
	CalculateWin32Path();
	return GetFileSystemHandler()->IsSymbolicLink(*this);
}

int Path::GetUnixFileType()
{
	CalculateWin32Path();
	return GetFileSystemHandler()->GetUnixFileType(*this);
}

DWORD Path::GetFileAttributes()
{
	CalculateWin32Path();
	return GetFileSystemHandler()->GetFileAttributes(*this);
}
