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

	IOHandler::BasicStat64(s, S_IFCHR); //console is a character device

	return true;
}

DWORD IOHNtConsole::ioctl(DWORD request, DWORD data)
{
	switch(request)
	{
		/*
	case TCGETS:
		{
			linux::termios * arg = (linux::termios*)pCtx->Edx;
			arg->c_iflag = 0;		/* input mode flags *
			arg->c_oflag = 0;		/* output mode flags *
			arg->c_cflag = 0;		/* control mode flags *
			arg->c_lflag = 0;		/* local mode flags *
			arg->c_line = 0;			/* line discipline *
			//arg->c_cc[NCCS];		/* control characters *
			return 0;
		}
		break;
		*/
	case 0:
	default:
		ktrace("IMPLEMENT sys_ioctl 0x%lx for IOHNtConsole\n", request);
		return -ENOSYS;
	}
}
