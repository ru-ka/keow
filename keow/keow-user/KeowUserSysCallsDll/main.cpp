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
struct SysCallDll::RemoteAddrInfo AddrInfo;

static void LoadAddressInfo()
{
	memset(&AddrInfo, 0, sizeof(AddrInfo));

#define SET_ADDR(func) AddrInfo.#func = func

//	SET_ADDR(Write);

#undef SET_ADDR
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

		LoadAddressInfo();

		//pass control to the kernel to re-develop us into a keow process
		__asm {
			lea eax, AddrInfo

			//set single-step mode to tell kernel we are ready
			pushfd
			pop ebx
			or ebx, 0x100   //trap bit
			push ebx
			popfd

			nop
		}

		//should never get here - it's bad
		ExitProcess(-11);
		break;

	} //switch

	return TRUE;
}
