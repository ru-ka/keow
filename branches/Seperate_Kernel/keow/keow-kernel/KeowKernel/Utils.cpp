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

// Utils.cpp: implementation of the Utils class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "Utils.h"

//////////////////////////////////////////////////////////////////////

void ktrace(const char *format, ...)
{
	va_list va;
	va_start(va, format);

	char * pNext = g_pKernelThreadLocals->ktrace_buffer;
	size_t size = sizeof(g_pKernelThreadLocals->ktrace_buffer);

	if(g_pKernelThreadLocals->pProcess==NULL || IsBadReadPtr(g_pKernelThreadLocals->pProcess, sizeof(Process)))
		StringCbPrintfEx(g_pKernelThreadLocals->ktrace_buffer, size, &pNext, &size, 0, "kernel:");
	else
		StringCbPrintfEx(g_pKernelThreadLocals->ktrace_buffer, size, &pNext, &size, 0, "pid %d:", g_pKernelThreadLocals->pProcess->m_Pid);

	StringCbVPrintf(pNext, size, format, va);

	OutputDebugString(g_pKernelThreadLocals->ktrace_buffer);
}

void halt()
{
	MSG msg;

	ktrace("Kernel halted.");

	while(GetMessage(&msg,0,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
