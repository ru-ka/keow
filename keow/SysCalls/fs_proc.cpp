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
	return false;
}

int ProcFs::GetUnixFileType(Path& path)
{
	//TODO: proc fs stuff
	return S_IFREG;
}

