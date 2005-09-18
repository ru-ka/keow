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
	ktrace("mapview.. %d %d:%d, %d, %p\n", hMap, offsetHi, offsetLo, len, BaseAddr);
	RET( (DWORD)::MapViewOfFileEx(hMap, Prot, offsetHi, offsetLo, len, BaseAddr) );
}

DWORD _stdcall SysCallDll::UnmapViewOfFile(void* BaseAddr)
{
	ktrace("unmapview.. %p\n", BaseAddr);
	RET( (DWORD)::UnmapViewOfFile(BaseAddr) );
}


DWORD _stdcall SysCallDll::exit(UINT exitcode)
{
	::ExitProcess(exitcode);

	//never get here
	RET(-1);
}


DWORD _stdcall SysCallDll::write(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD dw;
	BOOL ok = ::WriteFile(h, buf, len, &dw, NULL);
	RET(dw);
}

DWORD _stdcall SysCallDll::writev(HANDLE h, linux::iovec *pVec, int count)
{
	int total=0;
	while(count--)
	{
		DWORD dw=0;
		if(!::WriteFile(h, pVec->iov_base, pVec->iov_len, &dw, NULL))
			RET(GetLastError());
		total += dw;
		++pVec;
	}
	RET(total);
}

DWORD _stdcall SysCallDll::read(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD dw;
	BOOL ok = ::ReadFile(h, buf, len, &dw, NULL);
	RET(dw);
}
