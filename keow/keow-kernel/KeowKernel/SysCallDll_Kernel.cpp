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

DWORD _stdcall SysCallDll::GetLastError()
{
	return P->InjectFunctionCall(P->SysCallAddr.GetLastError, NULL, 0);
}


DWORD _stdcall SysCallDll::CloseHandle(HANDLE h)
{
	DWORD stack[] = {
		(DWORD)h
	};
	return P->InjectFunctionCall(P->SysCallAddr.CloseHandle, &stack, sizeof(stack));
}


DWORD _stdcall SysCallDll::SetFilePointer(HANDLE h, DWORD PosLo, DWORD PosHi, DWORD from)
{
	DWORD stack[] = {
		(DWORD)h,
		(DWORD)PosLo,
		(DWORD)PosHi,
		(DWORD)from,
	};
	return P->InjectFunctionCall(P->SysCallAddr.SetFilePointer, &stack, sizeof(stack));
}

DWORD _stdcall SysCallDll::SetEndOfFile(HANDLE h)
{
	DWORD stack[] = {
		(DWORD)h
	};
	return P->InjectFunctionCall(P->SysCallAddr.SetEndOfFile, &stack, sizeof(stack));
}


DWORD _stdcall SysCallDll::ZeroMem(void *p, DWORD len)
{
	DWORD stack[] = {
		(DWORD)p,
		len
	};
	return P->InjectFunctionCall(P->SysCallAddr.ZeroMem, &stack, sizeof(stack));
}


DWORD _stdcall SysCallDll::CreateFileMapping(HANDLE hFile, DWORD Prot, DWORD sizeHi, DWORD sizeLo)
{
	DWORD stack[] = {
		(DWORD)hFile,
		Prot,
		sizeHi,
		sizeLo
	};
	return P->InjectFunctionCall(P->SysCallAddr.CreateFileMapping, &stack, sizeof(stack));
}

DWORD _stdcall SysCallDll::MapViewOfFileEx(HANDLE hMap, DWORD Prot, DWORD offsetHi, DWORD offsetLo, DWORD len, void* BaseAddr)
{
	DWORD stack[] = {
		(DWORD)hMap,
		Prot,
		offsetHi,
		offsetLo,
		len,
		(DWORD)BaseAddr
	};
	return P->InjectFunctionCall(P->SysCallAddr.MapViewOfFileEx, &stack, sizeof(stack));
}


/*******************************************************************/

DWORD _stdcall SysCallDll::exit(UINT exitcode)
{
	return P->InjectFunctionCall(P->SysCallAddr.exit, &exitcode, sizeof(UINT));
}


DWORD _stdcall SysCallDll::write(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD stack[] = {
		(DWORD)h,
		(DWORD)buf,
		len
	};
	return P->InjectFunctionCall(P->SysCallAddr.write, &stack, sizeof(stack));
}

DWORD _stdcall SysCallDll::writev(HANDLE h, linux::iovec *pVec, int count)
{
	DWORD stack[] = {
		(DWORD)h,
		(DWORD)pVec,
		count
	};
	return P->InjectFunctionCall(P->SysCallAddr.writev, &stack, sizeof(stack));
}

DWORD _stdcall SysCallDll::read(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD stack[] = {
		(DWORD)h,
		(DWORD)buf,
		len
	};
	return P->InjectFunctionCall(P->SysCallAddr.read, &stack, sizeof(stack));
}
