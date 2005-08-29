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

// KeowConsole.cpp : Defines the entry point for the console application.
//

#include "includes.h"

#include "cons25.h"


HANDLE g_hKernelTextOutput;
HANDLE g_hKernelTextInput;
HANDLE g_hConsoleOutput;
HANDLE g_hConsoleInput;

cons25 g_Terminal;


static char TraceBuffer[1000];


char KernelTextBuffer[1000];
DWORD KernelTextRead;


BOOL WINAPI CtrlEventHandler(DWORD dwCtrlType)
{
	return TRUE; //always say we handle it
}


void ktrace(const char *format, ...)
{
	va_list va;
	va_start(va, format);

	char * pNext = TraceBuffer;
	size_t size = sizeof(TraceBuffer);

	StringCbPrintfEx(TraceBuffer, size, &pNext, &size, 0, "console:");

	StringCbVPrintf(pNext, size, format, va);

	OutputDebugString(TraceBuffer);
}



int main(int argc, char* argv[])
{
	//The console handler process is started by the kernel
	//It is a console mode app (so it has a console or can use an existing one)

	//It communicates with the kernel by anonymous pipes, whose 
	//handle values are passed in at startup


	//Set up the handles we want
	g_hKernelTextOutput = GetStdHandle(STD_INPUT_HANDLE);
	g_hKernelTextInput  = GetStdHandle(STD_OUTPUT_HANDLE);

	//now use the console window for io
	g_hConsoleInput  = CreateFile("CONIN$",  GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	g_hConsoleOutput = CreateFile("CONOUT$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);


	//set title as given
	SetConsoleTitle(GetCommandLine());


	//80 columns, lots of history
	COORD c;
	c.X = 80;
	c.Y = 1000;
	SetConsoleScreenBufferSize(g_hConsoleOutput, c);
	//start output at the bottom of the buffer
	c.X = 0;
	c.Y--;
	SetConsoleCursorPosition(g_hConsoleOutput, c);

	SMALL_RECT sr;
	sr.Left = 0;
	sr.Top = 0;
	sr.Right = 80-1;
	sr.Bottom = 25-1;
	SetConsoleWindowInfo(g_hConsoleOutput, true, &sr);

	//try for this more unix-like font (normal NT ones are too bold)
	//TODO: use Lucidia Console 12

	//SetConsoleCtrlHandler(CtrlEventHandler, TRUE);
	//SetConsoleCtrlHandler(NULL, TRUE);
	SetConsoleCtrlHandler(NULL, TRUE);

	SetConsoleMode(g_hConsoleInput, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT);
	SetConsoleMode(g_hConsoleOutput, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT);



	for(;;) {
		DWORD what = MsgWaitForMultipleObjects(1, &g_hConsoleInput, FALSE, 100, QS_INPUT);
		switch(what) {
		case WAIT_OBJECT_0:
			g_Terminal.InputChar();
			break;
		}

		//see if the kernel has sent any data
		DWORD dwAvail;
		if(PeekNamedPipe(g_hKernelTextOutput, NULL, 0, NULL, &dwAvail, NULL) == 0)
		{
			//kernel disappeared and left us behind?
			//if(GetLastError()==ERROR_BROKEN_PIPE)
				ExitProcess(0);
		}
		else
		if(dwAvail!=0)
		{
			ReadFile(g_hKernelTextOutput, KernelTextBuffer, dwAvail, &dwAvail, NULL);
			for(DWORD i=0; i<dwAvail; ++i)
				g_Terminal.OutputChar(KernelTextBuffer[i]);
		}
	}

	return 0;
}
