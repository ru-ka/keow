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

#ifndef KEOW_FILESYS_H
#define KEOW_FILESYS_H


class Path;
class IOHandler;

//indexes into FileSystemTable
#define KEOW_FS_INDEX  0
#define PROC_FS_INDEX  1
#define DEV_FS_INDEX   2


/*
 * This class works along side Path and mounting to resolve paths 
 * to the required win32 path.
 * Pure virtual class. Implemented for each filesystem type we support.
 * Is a friend class of Path so that it can help in resolving the path
 */
class FileSystemHandler
{
public:
	FileSystemHandler();
	virtual ~FileSystemHandler();

	virtual const char * Name() = 0;

	virtual IOHandler* CreateIOHandler(Path& path) = 0;
	virtual bool IsSymbolicLink(Path& path) = 0;
	virtual int GetUnixFileType(Path& path) = 0;

	static FileSystemHandler * RootFileSystemHandler;
	static FileSystemHandler * FileSystemTable[];

	static FileSystemHandler * Get(int index);
	static FileSystemHandler * Get(const char * name);
	static int GetIndex(const char * name);

protected:
	friend Path;
	virtual void ApplyPathElement(Path& path, const char *pStr) = 0;

};


/***************************************************************************/

class KeowFs : public FileSystemHandler
{
public:
	const char * Name() {
		return "keow";
	}
protected:
	virtual void ApplyPathElement(Path& path, const char *pStr);
	virtual IOHandler* CreateIOHandler(Path& path);
	virtual bool IsSymbolicLink(Path& path);
	virtual int GetUnixFileType(Path& path);
};

class ProcFs : public FileSystemHandler
{
public:
	const char * Name() {
		return "proc";
	}
protected:
	virtual void ApplyPathElement(Path& path, const char *pStr);
	virtual IOHandler* CreateIOHandler(Path& path);
	virtual bool IsSymbolicLink(Path& path);
	virtual int GetUnixFileType(Path& path);
};

class DevFs : public FileSystemHandler
{
public:
	const char * Name() {
		return "dev";
	}
protected:
	virtual void ApplyPathElement(Path& path, const char *pStr);
	virtual IOHandler* CreateIOHandler(Path& path);
	virtual bool IsSymbolicLink(Path& path);
	virtual int GetUnixFileType(Path& path);
};



#endif // KEOW_FILESYS_H
