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
 * generate core dumps
 */
#include "kernel.h"
#include "loadelf.h"


void GenerateCoreDump()
{
	pProcessData->core_dumped = true;

	HANDLE hCore = CreateFile("core",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
	if(hCore==INVALID_HANDLE_VALUE)
	{
		ktrace("FAIL writing core file: err %lx\n", GetLastError());
		return;
	}

	//TODO implement a real core dump
	DWORD written;
	WriteFile(hCore, "Core dump of ", 13, &written, 0);
	WriteFile(hCore, (char*)pProcessData->ProgramPath, strlen((char*)pProcessData->ProgramPath), &written, 0);
	WriteFile(hCore, "\x0a", 1, &written, 0);


	CloseHandle(hCore);
}
