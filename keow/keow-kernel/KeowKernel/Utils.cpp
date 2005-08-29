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


ULARGE_INTEGER Time1Jan1970;

//////////////////////////////////////////////////////////////////////


void ktrace(const char *format, ...)
{
	va_list va;
	va_start(va, format);

	char * pNext = g_pTraceBuffer;
	size_t size = KTRACE_BUFFER_SIZE;

	if(P==NULL || IsBadReadPtr(P, sizeof(Process)))
		StringCbPrintfEx(g_pTraceBuffer, size, &pNext, &size, 0, "kernel:");
	else
		StringCbPrintfEx(g_pTraceBuffer, size, &pNext, &size, 0, "pid %d:", P->m_Pid);

	StringCbVPrintfEx(pNext, size, &pNext, &size, 0, format, va);

	OutputDebugString(g_pTraceBuffer);

}

void halt()
{
	MSG msg;

	ktrace("Kernel halted.");

	ExitProcess(0);
	while(GetMessage(&msg,0,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
