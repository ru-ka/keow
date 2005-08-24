// IOHNtConsole.cpp: implementation of the IOHNtConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "IOHNtConsole.h"


const char * IOHNtConsole::ms_ConsoleProcessExe = "KeowConsole.exe";

//////////////////////////////////////////////////////////////////////

IOHNtConsole::IOHNtConsole()
{
}

IOHNtConsole::IOHNtConsole(string Title)
{
	m_Title = Title;
	m_hConsoleRead = m_hConsoleWrite = 0;

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
	
	string cmdline = ms_ConsoleProcessExe;
	cmdline += " Keow Console";

	CreateProcess(NULL, (char*)cmdline.c_str(), 0, 0, TRUE, 0, 0, 0, &si, &pi);
}

IOHNtConsole::~IOHNtConsole()
{
	CloseHandle(m_hConsoleRead);
	CloseHandle(m_hConsoleWrite);
}

bool IOHNtConsole::Write(LPVOID buffer, DWORD count, DWORD& written)
{
	DWORD ret = WriteFile(m_hConsoleWrite, buffer, count, &written, NULL);
	FlushFileBuffers(m_hConsoleWrite);
	return ret != 0;
}

bool IOHNtConsole::Read(LPVOID buffer, DWORD count, DWORD& read)
{
	return ReadFile(m_hConsoleRead, buffer, count, &read, NULL) != 0;
}


HANDLE IOHNtConsole::GetWriteHandle()
{
	return m_hConsoleWrite;
}
HANDLE IOHNtConsole::GetReadHandle()
{
	return m_hConsoleRead;
}


IOHandler * IOHNtConsole::clone()
{
	IOHNtConsole * pC = new IOHNtConsole();
	pC->m_Title = m_Title;
	pC->m_hConsoleRead = m_hConsoleRead;
	pC->m_hConsoleWrite = m_hConsoleWrite;
	return pC;
}
