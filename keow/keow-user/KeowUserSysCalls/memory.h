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

#ifndef KEOW_MEM_H
#define KEOW_MEM_H


void RecordMemoryAllocation(ADDR addr, DWORD len, DWORD prot);
void RecordMemoryFree(ADDR addr, DWORD len);
void RecordMMapAllocation(HANDLE hFile, HANDLE hMap, DWORD offset, ADDR addr, DWORD len, DWORD prot);
void RecordMMapFree(ADDR addr, DWORD len);

bool ReadMemory(ADDR toAddr, HANDLE hFromProcess, ADDR fromAddr, int len);
bool WriteMemory(HANDLE hToProcess, ADDR toAddr, int len, ADDR fromAddr);

ADDR AllocateMemAndProtectProcess(HANDLE hProcess, ADDR addr, DWORD size, DWORD prot);
ADDR AllocateMemAndProtect(ADDR addr, DWORD size, DWORD prot);
bool DeallocateMemory(ADDR addr, DWORD size);

_declspec(dllexport) char** CopyStringListToProcess(HANDLE hProcess, char * list[]);


struct MemBlock
{
	ADDR addr;
	DWORD len;
};

#endif //KEOW_MEM_H
