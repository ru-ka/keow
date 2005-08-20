// KeowConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "termdata.h"

HANDLE g_hKernelTextOutput;
HANDLE g_hKernelTextInput;
HANDLE g_hConsoleOutput, g_hConsoleInput;

DWORD			g_InputState; //for shared escape code handling
DWORD			g_OutputState; //for shared escape code handling
BYTE			g_InputStateData[32];
BYTE			g_OutputStateData[32];



BOOL WINAPI CtrlEventHandler(DWORD dwCtrlType)
{
	return TRUE; //always say we handled it
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


	//do io forever (at least until kernel terminates us)
	HANDLE waits[2];
	waits[0] = g_hKernelTextOutput;
	waits[1] = g_hConsoleInput;

	for(;;) {
		DWORD what = MsgWaitForMultipleObjects(NUM_HANDLES, waits, FALSE, INFINITE, 0);
		switch(what) {
		case WAIT_OBJECT_0 + 0:
			HandleKernelTextOut();
			break;
		case WAIT_OBJECT_0 + 1:
			HandleConsoleInput();
			break;
		}
	}

	return 0;
}
