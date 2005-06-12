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
#include "filesys.h"


/* Table of supported filesystem handlers. Accessed by index because
 * we cant re-use pointers across processes.
 * MountPointDataStruct.m_nFsHandler contains an index into this.
 */
/*static*/ FileSystemHandler* FileSystemHandler::FileSystemTable[] = {
	//items MUST match the #defines in filesys.h
	new KeowFs(),	//[KEOW_FS_INDEX]
	new ProcFs(),	//[PROC_FS_INDEX]
	//new DevFs(),	//[DEV_FS_INDEX]

	NULL //eof indicator
};

/*
 * root fs is this
 */
/*static*/ FileSystemHandler* FileSystemHandler::RootFileSystemHandler = FileSystemTable[0];



/*static*/ FileSystemHandler * FileSystemHandler::Get(int index)
{
	return FileSystemTable[index];
}

/*static*/ FileSystemHandler * FileSystemHandler::Get(const char * name)
{
	int i = GetIndex(name);
	if(i==-1)
		return NULL;
	return FileSystemTable[i];
}

/*static*/ int FileSystemHandler::GetIndex(const char * name)
{
	int i;
	for(i=0; FileSystemTable[i]!=NULL; ++i)
	{
		if(strcmp(FileSystemTable[i]->Name(), name) == 0)
			return i;
	}
	return -1; //not found
}



FileSystemHandler::FileSystemHandler()
{
}

FileSystemHandler::~FileSystemHandler()
{
}
