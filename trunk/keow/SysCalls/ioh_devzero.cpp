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
 * an IO handler for /dev/zero
 */
#include "kernel.h"
#include "iohandler.h"


DevZeroIOHandler::DevZeroIOHandler() 
: IOHandler(sizeof(DevZeroIOHandler))
{
}


bool DevZeroIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	ZeroMemory(address, size);
	if(pRead)
		*pRead = size;
	return true;
}


bool DevZeroIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	if(pWritten)
		*pWritten = size;
	return true;
}


IOHandler* DevZeroIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	//nothing special required
	return new DevZeroIOHandler();
}


DWORD DevZeroIOHandler::ioctl(DWORD request, DWORD data)
{
	//ignore all
	return 0;
}

bool DevZeroIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFCHR);

	return true;
}

bool DevZeroIOHandler::CanRead()
{
	//always
	return true;
}

bool DevZeroIOHandler::CanWrite()
{
	//never
	return false;
}

bool DevZeroIOHandler::HasException()
{
	//always ok?
	return false;
}
