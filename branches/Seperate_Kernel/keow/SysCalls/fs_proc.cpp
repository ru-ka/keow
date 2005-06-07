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



/* append a single file or directory name to the path
 * take care when traversing mount points and symlinks
 */
void ProcFs::ApplyPathElement(Path& path, const char *pStr)
{
	//we don't use Win32Path, just Unix Path
	//so not updating required.
	//but we do it to make debug messages readable (they use win32path)
	StringCbCat(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), "/");
	StringCbCat(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), pStr);

	//some things in proc are symlinks to real directories
	ProcIOHandler ioh(path);
	if(ioh.Type() == ProcIOHandler::TypeSymLink)
	{
		Path newP;
		newP = path;
		DWORD dwRead = 0;
		ioh.Read(newP.m_UnixPath, sizeof(newP.m_UnixPath), &dwRead);
		newP.m_UnixPath[dwRead] = 0;
		newP.CalculateWin32Path();
		
		memcpy(path.m_Win32Path, newP.m_Win32Path, sizeof(path.m_Win32Path));
		path.m_nMountPoint = newP.m_nMountPoint;
	}


	//check that the name we currently have is not a mount point we need to resolve
	//!!! rely on pStr being a null terminated substring of m_UnixPath so if we examine
	//!!!  m_UnixPath right now it will be null terminated right at the point we are
	//!!!  currently considering
	for(int m=0; m<pKernelSharedData->NumCurrentMounts; ++m)
	{
		MountPointDataStruct &mnt = pKernelSharedData->MountPoints[m];

		if(strcmp(mnt.Destination, path.m_UnixPath) == 0)
		{
			ktrace("mnt%d %s == %s\n", m, mnt.Destination, path.m_UnixPath);

			//follow this mount by replacing with a new path
			StringCbCopy(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), mnt.Source);

			path.m_nMountPoint = m;
		}
	}

}



/*
 * construct an iohandler suitable for manipulating a path
 * the iohandler is minimally initialized on return (may not have done Open for example)
 */
IOHandler* ProcFs::CreateIOHandler(Path& path)
{
	return new ProcIOHandler(path);
}


bool ProcFs::IsSymbolicLink(Path& path)
{
	ProcIOHandler p(path);
	return p.Type() == ProcIOHandler::TypeSymLink;
}

int ProcFs::GetUnixFileType(Path& path)
{
	ProcIOHandler p(path);
	switch(p.Type())
	{
	case ProcIOHandler::TypeSymLink:
		return S_IFLNK;

	case ProcIOHandler::TypeDir:
		return S_IFDIR;

	case ProcIOHandler::TypeData:
	default:
		return S_IFREG;
	}
}

DWORD ProcFs::GetFileAttributes(Path& path)
{
	ProcIOHandler p(path);
	DWORD attr = 0;
	switch(p.Type())
	{
	case ProcIOHandler::TypeSymLink:
		//attr |= ?;
		break;

	case ProcIOHandler::TypeDir:
		attr |= FILE_ATTRIBUTE_DIRECTORY;
		break;

	case ProcIOHandler::TypeData:
	default:
		break;
	}
		
	attr |= FILE_ATTRIBUTE_READONLY;
	return attr;
}
