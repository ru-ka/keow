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

HANDLE IOHNtConsole::GetKernelWriteHandle()
{
	return m_pConsole->m_hConsoleWrite;
}
HANDLE IOHNtConsole::GetKernelReadHandle()
{
	return m_pConsole->m_hConsoleRead;
}


IOHandler * IOHNtConsole::clone()
{
	IOHNtConsole * pC = new IOHNtConsole(m_pConsole);
	pC->m_hRemoteConsoleRead = m_hRemoteConsoleRead;
	pC->m_hRemoteConsoleWrite = m_hRemoteConsoleWrite;
	return pC;
}

bool IOHNtConsole::Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags)
{
	//always open anyway
	return true;
}

bool IOHNtConsole::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFCHR);

	s->st_mode |= S_IFCHR; //console is a character device

	/*
	#define S_IFMT  00170000
	#define S_IFSOCK 0140000
	#define S_IFLNK	 0120000
	#define S_IFREG  0100000
	#define S_IFBLK  0060000
	#define S_IFDIR  0040000
	#define S_IFCHR  0020000
	#define S_IFIFO  0010000
	#define S_ISUID  0004000
	#define S_ISGID  0002000
	#define S_ISVTX  0001000
	*/

	return true;
}
