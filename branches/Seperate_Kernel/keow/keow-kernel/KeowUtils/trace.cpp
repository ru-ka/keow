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
#include "utils.h"


/* debug helper */
void MemoryDump(const char * msg, const void * from_addr, DWORD sz)
{
	char *buf;
	char *end;
	char *line, *chars;
	const unsigned char * a = (const unsigned char*)from_addr;
	size_t remaining;
	int col;
	DWORD idx;
	const char hex[] = "0123456789abcdef";
	int bufsize;

	bufsize = 200 + strlen(msg) + (sz * 5) + (sz/8 * 20);
	buf = new char [ bufsize ];
	StringCbPrintfEx(buf, bufsize, &end, &remaining, 0, "Memory dump [%s] @ 0x%08lx, size %ld\n", msg, a, sz);

	//dump in 8 byte blocks like this:
	//  0x1234abcd  00 01 02 03  f5 f2 f8 00   .... ....
	for(idx=0; idx<sz; )
	{
		//prepare a new row
		StringCbPrintfEx(end,remaining, &end, &remaining, 0, "   0x%08lx  ", a);
		line = end;
		StringCbCopyEx(end, remaining, "00 00 00 00  00 00 00 00  ", &end, &remaining, 0);
		chars = end;
		StringCbCopyEx(end, remaining, ".... ....\n", &end, &remaining, 0);

		//add data as available
		for(col=0; col<8; idx++,col++,a++)
		{
			if(idx<sz)
			{
				int c = *a;
				*line = hex[c>>4];  line++;
				*line = hex[c&0xF]; line++;
				if(ispunct(c) || isalnum(c))
					*chars = c;
				else
					*chars = '.';
				chars++;
			}
			else
			{
				*line  = ' ';
				*line  = ' ';
				*chars = ' ';
				line+=2;
				chars++;
			}

			line += (col==3 ? 2 : 1);
			chars += (col==3 ? 1 : 0);
		}
	}

	ktrace(buf);
	delete buf;
}



/*
 * kernel debug tracing
 */
void _cdecl ktrace(const char * format, ...)
{
	//if(pKernelSharedData && pKernelSharedData->KernelDebug==0)
	//	return;

	int bufsize = strlen(format) + 1000;
	char * buf = new char [bufsize];

	SYSTEMTIME st;
	GetSystemTime(&st);

	va_list va;
	va_start(va, format);
	StringCbPrintf(buf, bufsize, "[%d : %ld] %d:%02d:%02d.%04d ",
					g_PID, GetCurrentProcessId(),
					st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
	StringCbVPrintf(&buf[strlen(buf)], bufsize-strlen(buf), format, va);
	va_end(va);

	/*
	if(pKernelSharedData && pProcessData)
	{
		if(pProcessData->hKernelDebugFile==INVALID_HANDLE_VALUE)
		{
			char n[MAX_PATH];
			StringCbPrintf(n,sizeof(n),"%s\\..\\pid%05d.trace", pKernelSharedData->LinuxFileSystemRoot, pProcessData->PID);
			pProcessData->hKernelDebugFile = CreateFile(n, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, ktrace_should_append?OPEN_ALWAYS:CREATE_ALWAYS, 0, 0);
		}
		DWORD dw;
		SetFilePointer(pProcessData->hKernelDebugFile, 0,0, FILE_END);
		WriteFile(pProcessData->hKernelDebugFile, buf, strlen(buf), &dw ,0);
		//FlushFileBuffers(pProcessData->hKernelDebugFile);
	}
	*/

	OutputDebugString(buf);
	delete buf;
}

