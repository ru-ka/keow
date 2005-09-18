// DevConsole.cpp: implementation of the DevConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "DevConsole.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DevConsole::DevConsole(int tty)
: Device("tty", 0, 0)
{
	m_tty = tty;
	m_ProcessGroup = 0;
	m_hConsoleRead = m_hConsoleWrite = NULL;
	InitializeCriticalSection(&m_csIoctl);

	//need a console process to do the actual terminal stuff

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	//make stdin, stdout talk with us
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	memset(&si,0,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	CreatePipe( &si.hStdInput, &m_hConsoleWrite, &sa, 0);
	CreatePipe( &m_hConsoleRead, &si.hStdOutput, &sa, 0);

	HANDLE hIoctlIn, hIoctlOut;
	CreatePipe( &hIoctlOut, &m_hIoctlWrite, &sa, 0);
	CreatePipe( &m_hIoctlRead, &hIoctlIn, &sa, 0);

	string cmdline = "KeowConsole.exe ";
	cmdline += string::format("%ld ", GetCurrentProcessId());
	cmdline += string::format("%ld ", si.hStdInput);
	cmdline += string::format("%ld ", si.hStdOutput);
	cmdline += string::format("%ld ", hIoctlOut);
	cmdline += string::format("%ld ", hIoctlIn);
	cmdline += " \"Keow Console\"";

	CreateProcess(NULL, cmdline.GetBuffer(cmdline.length()), 0, 0, TRUE, 0, 0, 0, &si, &pi);
	cmdline.ReleaseBuffer();
}

DevConsole::~DevConsole()
{
	CloseHandle(m_hConsoleRead);
	CloseHandle(m_hConsoleWrite);
	CloseHandle(m_hIoctlRead);
	CloseHandle(m_hIoctlWrite);
	DeleteCriticalSection(&m_csIoctl);
}
