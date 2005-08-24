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

/*
 * This include file defines the functions that are supplied in the
 * keow user syscall dll.
 * They allow some system calls to be run from the user/stub process directly.
 *
 * The kernel implementes these functions as calls to InvokeStubSysCall
 * and the user/stub implements them as the actual calls
 */

#if !defined(AFX_SYSCALLDLL_H__065A3BC3_71C3_4302_8E39_297E193A46AF__INCLUDED_)
#define AFX_SYSCALLDLL_H__065A3BC3_71C3_4302_8E39_297E193A46AF__INCLUDED_

//This file needs to not reference the other kernel structures
//because it is included in the user process
class Process; //exception to the rule, just to allow a pointer to the associated process on the kernel implementation

//We need the _stdcall convention to allow all functions to clean up the stack
//and not rely on the caller to do it.
//This means the kernel can inject a call (&stack params) into the user process
//and not need to inject any stack cleanup code.

//WE DO expect that EAX will contain any return code if needed

class SysCallDll
{
public: //functionality provided
	void _stdcall ExitProcess(UINT exitcode);

	DWORD _stdcall Write(HANDLE h, LPVOID buf, DWORD len, DWORD *pdwWritten);
	DWORD _stdcall WriteV(HANDLE h, linux::iovec *pVec, int count, DWORD *pdwWritten);

	DWORD _stdcall Read(HANDLE h, LPVOID buf, DWORD len, DWORD *pdwRead);

public: //for passing info to the kernel
	struct RemoteAddrInfo {
		LPVOID ExitProcess;
		LPVOID Write;
		LPVOID WriteV;
		LPVOID Read;
	};

public: //support stuff
	SysCallDll(Process * proc = NULL)
		: P(proc)
	{
	}

	RemoteAddrInfo m_RemoteAddresses;
	Process * P;
};

#endif
