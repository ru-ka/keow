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


//How to return from these functions so that the kernel/debugger see's it

#define RET(val)  { DWORD __x=val; {__asm mov eax, __x  __asm int 3}; return val; }

////////////////////////////////////////////////////////////////////////

DWORD _stdcall SysCallDll::CloseHandle(HANDLE h)
{
	RET( ::CloseHandle(h) );
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
	DWORD n = WriteFile(h, buf, len, &dw, NULL);
	RET(n);
}

DWORD _stdcall SysCallDll::writev(HANDLE h, linux::iovec *pVec, int count)
{
	int total=0;
	while(count--)
	{
		DWORD dw=0;
		if(!WriteFile(h, pVec->iov_base, pVec->iov_len, &dw, NULL))
			RET(GetLastError());
		total += dw;
	}
	RET(total);
}

DWORD _stdcall SysCallDll::read(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD dw;
	DWORD n = ReadFile(h, buf, len, &dw, NULL);
	RET(n);
}
