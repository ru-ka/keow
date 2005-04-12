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

/*
 * an IO handler for /dev/null
 */
#include "kernel.h"
#include "iohandler.h"


DevNullIOHandler::DevNullIOHandler() 
: IOHandler(sizeof(DevNullIOHandler))
{
}


bool DevNullIOHandler::Open(Path& filepath, DWORD access, DWORD ShareMode, DWORD disposition, DWORD flags)
{
	//was always open
	return true;
}

bool DevNullIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	if(pRead)
		*pRead = 0;
	//never can read from /dev/null, block if we can
	//TODO: wake on signals
	while((m_Flags&O_NONBLOCK)==0)
		Sleep(10000);
	return true;
}


bool DevNullIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	if(pWritten)
		*pWritten = size;
	return true;
}


IOHandler* DevNullIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	//nothing special required
	return new DevNullIOHandler();
}


DWORD DevNullIOHandler::ioctl(DWORD request, DWORD data)
{
	//ignore all
	return 0;
}

bool DevNullIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFCHR);

	return true;
}

bool DevNullIOHandler::CanRead()
{
	//never
	return false;
}

bool DevNullIOHandler::CanWrite()
{
	//always
	return true;
}

bool DevNullIOHandler::HasException()
{
	//always ok?
	return false;
}



int DevNullIOHandler::GetDirEnts64(linux::dirent64 *, int maxbytes)
{
	return 0;
}

DWORD DevNullIOHandler::Length()
{
	return 0;
}

DWORD DevNullIOHandler::Seek(DWORD offset, DWORD method)
{
	return -1;
}
