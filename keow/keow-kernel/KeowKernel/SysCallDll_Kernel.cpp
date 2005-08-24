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
#include "SysCallDll.h"


////////////////////////////////////////////////////////////////////////

void _stdcall SysCallDll::ExitProcess(UINT exitcode)
{
	P->InjectFunctionCall(m_RemoteAddresses.ExitProcess, &exitcode, sizeof(UINT), false);
}


DWORD _stdcall SysCallDll::Write(HANDLE h, LPVOID buf, DWORD len, DWORD *pdwWritten)
{
	DWORD stack[] = {
		(DWORD)h,
		(DWORD)buf,
		len,
		0
	};
	DWORD dwRet = P->InjectFunctionCall(m_RemoteAddresses.Write, &stack, sizeof(stack), true);
	if(pdwWritten)
		*pdwWritten = stack[3];
	return 0;
}

DWORD _stdcall SysCallDll::WriteV(HANDLE h, linux::iovec *pVec, int count, DWORD *pdwWritten)
{
	return ERROR_1;
}

DWORD _stdcall SysCallDll::Read(HANDLE h, LPVOID buf, DWORD len, DWORD *pdwRead)
{
	DWORD stack[] = {
		(DWORD)h,
		(DWORD)buf,
		len,
		0
	};
	DWORD dwRet = P->InjectFunctionCall(m_RemoteAddresses.Read, &stack, sizeof(stack), true);
	if(pdwRead)
		*pdwRead = stack[3];
	return 0;
}
