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
void KeowFs::ApplyPathElement(Path& path, const char *pStr)
{
	//build up the path
	StringCbCat(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), "/"); //"\\");
	char * pWin32OldEnd = &path.m_Win32Path[strlen(path.m_Win32Path)];

	StringCbCat(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), pStr);

	//now see if we arrived at a special path....


	//check that the name we currently have is not a link we need to follow
	char * pWin32End = &path.m_Win32Path[strlen(path.m_Win32Path)];
	StringCbCat(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), ".lnk");
	if(::GetFileAttributes(path.m_Win32Path)==INVALID_FILE_ATTRIBUTES)
		*pWin32End = 0; //remove the .lnk
	else
	{
		if(path.m_FollowSymLinks)
		{
			char szTarget[MAX_PATH];
			//try to open the file as a windows shortcut
			HRESULT hr = GetShortCutTarget(path.m_Win32Path, szTarget);
			if(SUCCEEDED(hr))
			{
				//remove link name and replace with what it refers to
				*pWin32OldEnd = 0;
				if(PathIsRelative(szTarget))
					StringCbCat(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), szTarget);
				else
					StringCbCopy(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), szTarget);
			}
		}
	}

	
	//check that the name we currently have is not a mount point we need to resolve
	//!!! rely on pStr being a null terminated substring of m_UnixPath so if we examine
	//!!!  m_UnixPath right now it will be null terminated right at the point we are
	//!!!  currently considering
	for(int m=0; m<g_KernelData.NumCurrentMounts; ++m)
	{
		MountPointDataStruct &mnt = g_KernelData.MountPoints[m];

		if(strcmp(mnt.Destination, path.m_UnixPath) == 0)
		{
			ktrace("mnt%d %s == %s\n", m, mnt.Destination, path.m_UnixPath);

			//follow this mount by replacing with a new path
			StringCbCopy(path.m_Win32Path, MAX_PATH-strlen(path.m_Win32Path), mnt.Source);

			//new mount point
			path.m_nMountPoint = m;
		}
	}

}

/*
 * construct an iohandler suitable for manipulating a path
 * the iohandler is minimally initialized on return (may not have done Open for example)
 */
IOHandler* KeowFs::CreateIOHandler(Path& path)
{
	static const char tty_prefix[] = "/dev/tty";
	static const char dev_prefix[] = "/dev/";

	const char * UnixPath = path.UnixPath();

	if(strcmp(UnixPath, "/dev/tty") == 0)
	{
		//TODO:
		/*
		//find terminal we are attached to
		for(int i=0; i<MAX_TERMINALS; ++i) 
		{
		}
		return NULL; //no controlling tty?
		*/
		return new ConsoleIOHandler(0, false); //existing console window
	}
	else
	if(strncmp(UnixPath, tty_prefix, sizeof(tty_prefix)-1) == 0)
	{
		// it is /dev/tty*
		int num = atoi(&UnixPath[sizeof(tty_prefix)-1]);
		if(num >= NUM_CONSOLE_TERMINALS)
			return NULL; //no device
		//allocate a console if one is not around
		//return a ConsoleIOHandler for it
		//return new ConsoleIOHandler()
		return NULL; //TODO
	}
	else
	if(strcmp(UnixPath, "/dev/console") == 0)
	{
		return new ConsoleIOHandler(0, false); //existing console window
	}
	else
	if(strcmp(UnixPath, "/dev/null") == 0)
	{
		return new DevNullIOHandler();
	}
	else
	if(strcmp(UnixPath, "/dev/zero") == 0)
	{
		return new DevZeroIOHandler();
	}
	else
	if(strncmp(UnixPath, dev_prefix, sizeof(dev_prefix)-1) == 0)
	{
		ktrace("unhandled device requested: %s\n", UnixPath);
		return NULL;
	}
	else
	{
		return new FileIOHandler();
	}
}


//Helper 
//if fullpath is a link make linkpath the relative locate it refers to
//otherwise linkpath is blank
static void GetLinkRelativePath(const char *fullpath, char* linkpath, int maxsize)
{
	HRESULT hr;
	char szTarget[MAX_PATH]; 

	*linkpath = 0;

	//try to open the file as a windows shortcut
	hr = GetShortCutTarget(fullpath, szTarget);
	if(SUCCEEDED(hr))
	{
		if(PathIsRelative(szTarget))
			StringCbCopy(linkpath, maxsize, szTarget);
		else
		{
			//make linkpath a relative path from link to target
			PathRelativePathTo(linkpath, fullpath, FILE_ATTRIBUTE_NORMAL, szTarget, FILE_ATTRIBUTE_NORMAL);
		}
	}
}



bool KeowFs::IsSymbolicLink(Path& path)
{
	char sourcepath[MAX_PATH];
	char destpath[MAX_PATH];

	for(int i=0; i<2; ++i)
	{
		destpath[0] = 0;
		StringCbCopy(sourcepath, sizeof(sourcepath)-5, path.Win32Path());
		if(i>0)
			StringCbCat(sourcepath, sizeof(sourcepath), ".lnk");

		if(::GetFileAttributes(sourcepath)!=INVALID_FILE_ATTRIBUTES)
		{
			GetLinkRelativePath(sourcepath, destpath, sizeof(destpath));
			if(destpath[0]!=0)
				return true;
		}
	}

	return false;
}

int KeowFs::GetUnixFileType(Path& path)
{
	DWORD attr = GetFileAttributes(path);
	if(attr == INVALID_FILE_ATTRIBUTES)
	{
		//ktrace("GetFileAddributes err=0x%lx on %s\n", GetLastError(), path.Win32Path());
		return 0; //nothing
	}

	if(attr & FILE_ATTRIBUTE_DIRECTORY)
		return S_IFDIR;

	if(IsSymbolicLink(path))
		return S_IFLNK;

	return S_IFREG; //regular file

	/*
#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000
	*/
}

DWORD KeowFs::GetFileAttributes(Path& path)
{
	return ::GetFileAttributes(path.Win32Path());
}

