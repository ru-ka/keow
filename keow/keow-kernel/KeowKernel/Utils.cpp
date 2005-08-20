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
