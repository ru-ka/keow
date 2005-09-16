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

// Stub Main

#include "includes.h"


//global
extern SysCallDll::RemoteAddrInfo AddrInfo;

static void LoadAddressInfo()
{
	memset(&AddrInfo, 0, sizeof(AddrInfo));

#define SET_ADDR(func) AddrInfo.##func = (LPVOID)(SysCallDll::##func )

	SET_ADDR(GetLastError);
	SET_ADDR(CloseHandle);
	SET_ADDR(SetFilePointer);
	SET_ADDR(SetEndOfFile);
	SET_ADDR(ZeroMem);
	SET_ADDR(CreateFileMapping);
	SET_ADDR(MapViewOfFileEx);

	SET_ADDR(write);
	SET_ADDR(writev);
	SET_ADDR(read);
	SET_ADDR(exit);

#undef SET_ADDR
}


static char g_TraceBuffer[1024];

void ktrace(const char * format, ...)
{
	va_list va;
	va_start(va, format);

	wvsprintf(g_TraceBuffer, format, va);

	OutputDebugString(g_TraceBuffer);
}

void hexdump(BYTE *addr, DWORD len)
{
	char buf[5 * 16 + 100];
	int x;

	x=0;
	for(DWORD i=0; i<len; ++i)
	{
		wsprintf(&buf[x*5], "0x%02x", *addr);
		buf[x*5+4]=' ';
		buf[x*5+5]=0;

		if(x<15)
			++x;
		else
		{
			ktrace("dump %p,%d: %s\n", addr,len, buf);
			x=0;
		}

		++addr;
	}
	if(x>0)
		ktrace("dump %p,%d: %s\n", addr,len, buf);
}


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID p)
{
	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:
		//Set the info about the win32 side.
		//The stub provides this info in Eax prior to causing an initial breakpoint 
		//to send it to the kernel
		//(see Process::ConvertProcessToKeow()

		OutputDebugString("SysCallDll loading\n");
		LoadAddressInfo();

		//pass control to the kernel to re-develop us into a keow process
		OutputDebugString("SysCallDll transfering to kernel\n");
		__asm {
			lea eax, AddrInfo

			//set single-step mode to tell kernel we are ready
			pushfd
			pop ebx
			or ebx, 0x100   //trap bit
			push ebx
			popfd
			//now we are single-stepping

			nop
		}

		//should never get here - it's bad
		ExitProcess(-11);
		break;

	} //switch

	return TRUE;
}