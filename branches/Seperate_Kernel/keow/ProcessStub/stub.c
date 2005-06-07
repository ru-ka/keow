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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>


char msg[100] = "";
HMODULE hLibSyscalls = 0;

typedef void (*PHandleSysCall)(CONTEXT *pCtx);
PHandleSysCall HandleSysCall = 0;

typedef void (*PProcess_Init)(const char* keyword, int pid, const char * KernelInstance);
PProcess_Init Process_Init = 0;


int main(int argc, char ** argv)
{
	OutputDebugString("ProcessStub: starting\n");

	// Once started we have out own memory (small) and the ELF code already loaded
	// into the process. (because kernel started us suspended and then allocated mem).
	// Now load the (largish) syscall code. It can fit into the memory
	// not yet allocated and we don't have to worry about its largish 
	// size overwriting the addresses that the ELF code needed :-)

	OutputDebugString("ProcessStub: loading SysCalls.dll\n");
	hLibSyscalls = LoadLibrary("SysCalls.dll");
	if(hLibSyscalls==NULL)
	{
		OutputDebugString("ProcessStub: loadlib failed\n");
		ExitProcess((UINT)-11);
	}
	//get functions we need
	HandleSysCall = (PHandleSysCall)GetProcAddress(hLibSyscalls, "HandleSysCall");
	Process_Init = (PProcess_Init)GetProcAddress(hLibSyscalls, "Process_Init");

	//Start this process
	Process_Init(argv[0], atoi(argv[1]), argv[2]);

	//we shouldn't get here, but....
	OutputDebugString("ProcessStub: ELF code exited (not using exit(n))\n");
	return -11; //SIGSEGV for want of a better return code in this instance
}
