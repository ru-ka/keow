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


extern HRESULT GetShortCutTarget(LPCSTR lpszLinkFile, LPSTR lpszPath);


Path::Path(bool FollowSymLinks)
{
	m_UnixPath[0] = 0;
	m_Win32Path[0] = 0;
	m_Win32Calculated = false;
	m_FollowSymLinks = FollowSymLinks;
}
Path::~Path()
{
}


const char * Path::Win32Path()
{
	if(!m_Win32Calculated)
		CalculateWin32Path();
	return m_Win32Path;
}
const char * Path::UnixPath()
{
	return m_UnixPath;
}


void Path::SetUnixPath(const char * unixp)
{
	if(unixp[0] != '/')
		strncpy(m_UnixPath, pProcessData->unix_pwd, MAX_PATH-1);
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
	//always start here
	StringCbCopy(m_Win32Path, MAX_PATH, pKernelSharedData->LinuxFileSystemRoot);

	//apply requested path
	char *pStart = m_UnixPath;
	char *pEnd;
	do {
		while(*pStart == '/')
			pStart++; //don't want the slash

		pEnd = strchr(pStart, '/');
		if(pEnd==0)
			pEnd = pStart + strlen(pStart);

		//null terminate so we can use pStart as a string
		char save = *pEnd;
		*pEnd = 0;

		ApplyPathElement(pStart);

		//restore
		*pEnd = save;

		pStart = pEnd; //next char to process
	} while(*pStart != 0); //not null terminator

	ktrace("%s -> %s\n", m_UnixPath, m_Win32Path);
	m_Win32Calculated = true;
}


/* append a single file or directory name to the path
 * take care when traversing mount points and symlinks
 */
void Path::ApplyPathElement(const char *pStr)
{
	StringCbCat(m_Win32Path, MAX_PATH-strlen(m_Win32Path), "/"); //"\\");
	char * pWin32OldEnd = &m_Win32Path[strlen(m_Win32Path)];
	StringCbCat(m_Win32Path, MAX_PATH-strlen(m_Win32Path), pStr);

	//check that the name we currently have is not a mount point we need to resolve
	//!!! rely on pStr being a null terminated substring of m_UnixPath so if we examine
	//!!!  m_UnixPath right now it will be null terminated right at the point we are
	//!!!  currently considering
	for(int m=0; m<pKernelSharedData->NumCurrentMounts; ++m)
	{
		MountPointDataStruct &mnt = pKernelSharedData->MountPoints[m];

		if(strcmp(mnt.Destination, m_UnixPath) == 0)
		{
			//TODO: use a pointer to a resolver routine instead of fs type tests here
			if(strcmp("keow", mnt.pType) == 0)
			{
				//follow this mount by replacing with a new path
				StringCbCopy(m_Win32Path, MAX_PATH-strlen(m_Win32Path), mnt.Source);
			}
			else
			{
				ktrace("Panic: unhandled filesystem type: %s\n", mnt.pType);
				ExitProcess(-11);
			}
		}
	}

	//check that the name we currently have is not a link we need to follow
	char * pWin32End = &m_Win32Path[strlen(m_Win32Path)];
	StringCbCat(m_Win32Path, MAX_PATH-strlen(m_Win32Path), ".lnk");
	if(GetFileAttributes(m_Win32Path)==INVALID_FILE_ATTRIBUTES)
		*pWin32End = 0; //remove the .lnk
	else
	{
		if(m_FollowSymLinks)
		{
			char szTarget[MAX_PATH];
			//try to open the file as a windows shortcut
			HRESULT hr = GetShortCutTarget(m_Win32Path, szTarget);
			if(SUCCEEDED(hr))
			{
				//remove link name and replace with what it refers to
				*pWin32OldEnd = 0;
				if(PathIsRelative(szTarget))
					StringCbCat(m_Win32Path, MAX_PATH-strlen(m_Win32Path), szTarget);
				else
					StringCbCopy(m_Win32Path, MAX_PATH-strlen(m_Win32Path), szTarget);
			}
		}
	}
}
