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


#include <windows.h>
#include "../../keow-kernel/KeowKernel/StubResourceInfo.h"

//global
struct StubFunctionsInfo FuncInfo;

#define STUB_EXIT __asm int 3

//////////////////////////////////////////////////////////////////////////////////////


void _stdcall Stub_ExitProcess(DWORD param1, DWORD param2, DWORD param3, DWORD param4)
{
	ExitProcess(param1);

	STUB_EXIT
}


//////////////////////////////////////////////////////////////////////////////////////


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//Set the info about the win32 side.
	//The stub provides this info in Eax prior to causing an initial breakpoint 
	//to send it to the kernel
	//(see Process::ConvertProcessToKeow()

	FuncInfo.ExitProcess = Stub_ExitProcess;


	//pass control to the kernel to re-develop use into a keow process
	__asm {
		lea eax, FuncInfo

		//set single-step mode to tell kernel we are ready
		pushfd
		pop ebx
		or ebx, 0x100   //trap bit
		push ebx
		popfd
	}


	//should never get here
	ExitProcess(-1);
	return -1;
}
