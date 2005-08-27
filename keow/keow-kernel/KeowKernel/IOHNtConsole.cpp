// IOHNtConsole.cpp: implementation of the IOHNtConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "IOHNtConsole.h"


//////////////////////////////////////////////////////////////////////

IOHNtConsole::IOHNtConsole(DevConsole* pConsole)
{
	m_pConsole = pConsole;

	DuplicateHandle(GetCurrentProcess(), pConsole->m_hConsoleRead,
		P->m_Win32PInfo.hProcess, &m_hRemoteConsoleRead,
		0, FALSE, DUPLICATE_SAME_ACCESS);

	DuplicateHandle(GetCurrentProcess(), pConsole->m_hConsoleWrite,
		P->m_Win32PInfo.hProcess, &m_hRemoteConsoleWrite,
		0, FALSE, DUPLICATE_SAME_ACCESS);
}

IOHNtConsole::~IOHNtConsole()
{
	SysCallDll::CloseHandle(m_hRemoteConsoleRead);
	SysCallDll::CloseHandle(m_hRemoteConsoleWrite);
}


HANDLE IOHNtConsole::GetRemoteWriteHandle()
{
	return m_hRemoteConsoleWrite;
}
HANDLE IOHNtConsole::GetRemoteReadHandle()
{
	return m_hRemoteConsoleRead;
}


IOHandler * IOHNtConsole::clone()
{
	IOHNtConsole * pC = new IOHNtConsole(m_pConsole);
	pC->m_hRemoteConsoleRead = m_hRemoteConsoleRead;
	pC->m_hRemoteConsoleWrite = m_hRemoteConsoleWrite;
	return pC;
}
