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

// Kernel side implementation of the SysCallDll class.
//
////////////////////////////////////////////////////////////////////////

#include "includes.h"


SysCallDll::RemoteAddrInfo AddrInfo;
DWORD g_LastError = 0;


//How to return from these functions so that the kernel/debugger see's it
//It needs a breakpoint (int 3) and return value in Eax
//We save last error in case we are asked for it too

#define RET(val)  { DWORD __x=val; g_LastError=::GetLastError(); {__asm mov eax, __x  __asm int 3}; return __x; }

////////////////////////////////////////////////////////////////////////


DWORD _stdcall SysCallDll::VirtualAlloc(LPVOID lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect)
{
	ktrace("VirtualAlloc(0x%08lx, %d, %d, %d)\n", lpAddress, dwSize, flAllocationType, flProtect);
	RET( (DWORD)::VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect) );
}

DWORD _stdcall SysCallDll::VirtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType)
{
	ktrace("VirtualFree(0x%08lx, %d, %d)\n", lpAddress, dwSize, dwFreeType);
	RET( ::VirtualFree(lpAddress, dwSize, dwFreeType) );
}

DWORD _stdcall SysCallDll::VirtualQuery(LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength)
{
	ktrace("virtualquery(0x%08lx,0x%08lx,%d)\n", lpAddress, lpBuffer, dwLength);
	RET( ::VirtualQuery(lpAddress, lpBuffer, dwLength) );
}


DWORD _stdcall SysCallDll::GetLastError()
{
	RET( g_LastError );
}


DWORD _stdcall SysCallDll::CloseHandle(HANDLE h)
{
	RET( ::CloseHandle(h) );
}

DWORD _stdcall SysCallDll::SetFilePointer(HANDLE h, DWORD PosLo, DWORD PosHi, DWORD from)
{
	SetLastError(0); //because we need to check it always and the call will not update it on success
	RET( ::SetFilePointer(h, PosLo, (LONG*)&PosHi, from) );
}

DWORD _stdcall SysCallDll::GetFilePointer(HANDLE h)
{
	SetLastError(0); //because we need to check it always and the call will not update it on success
	//seek "zero from current" returns current position
	RET( ::SetFilePointer(h, 0, NULL, FILE_CURRENT) );
}

DWORD _stdcall SysCallDll::SetEndOfFile(HANDLE h)
{
	RET( ::SetEndOfFile(h) );
}

DWORD _stdcall SysCallDll::ZeroMem(void *p, DWORD len)
{
	RET( (DWORD)::ZeroMemory(p, len) );
}

DWORD _stdcall SysCallDll::CreateFileMapping(HANDLE hFile, DWORD Prot, DWORD sizeHi, DWORD sizeLo)
{
	ktrace("Cratefilemapping %d %d:%d\n", hFile, sizeHi,sizeLo);
	RET( (DWORD)::CreateFileMapping(hFile, NULL, Prot, sizeHi, sizeLo, NULL) );
}

DWORD _stdcall SysCallDll::MapViewOfFileEx(HANDLE hMap, DWORD Prot, DWORD offsetHi, DWORD offsetLo, DWORD len, void* BaseAddr)
{
	ktrace("mapview.. %d %d:%d, %d, 0x%08lx\n", hMap, offsetHi, offsetLo, len, BaseAddr);
	RET( (DWORD)::MapViewOfFileEx(hMap, Prot, offsetHi, offsetLo, len, BaseAddr) );
}

DWORD _stdcall SysCallDll::UnmapViewOfFile(void* BaseAddr)
{
	ktrace("unmapview.. 0x%08lx\n", BaseAddr);
	RET( (DWORD)::UnmapViewOfFile(BaseAddr) );
}


DWORD _stdcall SysCallDll::exit(UINT exitcode)
{
	::ExitProcess(exitcode);

	//never get here
	RET(-1);
}


DWORD _stdcall SysCallDll::WriteFile(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD dw;
	SetLastError(0); //because we need to check it always and the call will not update it on success
	BOOL ok = ::WriteFile(h, buf, len, &dw, NULL);
	RET(dw);
}

DWORD _stdcall SysCallDll::ReadFile(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD dw;
	SetLastError(0); //because we need to check it always and the call will not update it on success
	BOOL ok = ::ReadFile(h, buf, len, &dw, NULL);
	RET(dw);
}

DWORD _stdcall SysCallDll::PeekAvailablePipe(HANDLE h)
{
	BYTE buf;
	DWORD dwRead, dwMsgLeft;
	DWORD dwAvail = 0;

	if(!::PeekNamedPipe(h, &buf, 0, &dwRead, &dwAvail, &dwMsgLeft))
		dwAvail = 0;
	RET(dwAvail);
}
