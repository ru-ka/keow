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

IOHFile::IOHFile(Path &path)
{
	m_RemoteHandle = INVALID_HANDLE_VALUE;
	m_Path = path;

	//don't open yet, use Open()
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

bool IOHFile::Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags)
{
	//Open in the kernel, then move the handle to the user

	HANDLE h = CreateFile(m_Path.GetWin32Path(), win32access, win32share, 0, disposition, flags, 0);
	if(h==INVALID_HANDLE_VALUE)
		return false;

	//move handle to the user
	DuplicateHandle(GetCurrentProcess(), h, P->m_Win32PInfo.hProcess, &m_RemoteHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

	return true;
}

bool IOHFile::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;


	WIN32_FILE_ATTRIBUTE_DATA fi;
	ULARGE_INTEGER i;


	IOHandler::BasicStat64(s, 0);


	if(!GetFileAttributesEx(m_Path.GetWin32Path(), GetFileExInfoStandard, &fi))
		return false;


	if(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		s->st_mode = 0555;  // r-xr-xr-x
	else
		s->st_mode = 0755;  // rwxr-xr-x

	if(fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		s->st_mode |= S_IFDIR;
	else
	if(m_Path.IsSymbolicLink())
		s->st_mode |= S_IFLNK;
	else
		s->st_mode |= S_IFREG; //regular file
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


	s->st_nlink = 1;//fi.nNumberOfLinks;
	s->st_uid = 0;
	s->st_gid = 0;

	//use major '3' and minor as drive letter
	string w32 = m_Path.GetWin32Path();
	s->st_dev = s->st_rdev = (3<<8) | (BYTE)(w32[0] - 'A');

	//use hash of filename as inode
	s->st_ino = s->__st_ino = w32.hash();

	i.LowPart = fi.nFileSizeLow;
	i.HighPart = fi.nFileSizeHigh;
	s->st_size = i.QuadPart;

	s->st_blksize = 512; //block size for efficient IO
	
	s->st_blocks = (unsigned long)((s->st_size+511) / 512); //size in 512 byte blocks

	s->st_atime = FILETIME_TO_TIME_T(fi.ftLastAccessTime);
	s->st_mtime = FILETIME_TO_TIME_T(fi.ftLastWriteTime);
	s->st_ctime = FILETIME_TO_TIME_T(fi.ftCreationTime);


	return true;
}

__int64 IOHFile::Length()
{
	linux::stat64 s;
	Stat64(&s);
	return s.st_size;
}

DWORD IOHFile::Seek(__int64 pos, DWORD from)
{
	LARGE_INTEGER li;
	li.QuadPart = pos;
	return SysCallDll::SetFilePointer(m_RemoteHandle, li.LowPart, li.HighPart, from);
}
