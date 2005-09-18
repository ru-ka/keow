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
	DWORD dwRet = 0;
	DWORD dwDone;
	EnterCriticalSection(&m_pConsole->m_csIoctl); //only one ioctl at a time

	//the actual work needs to happen in the console handler process
	//sent request there and read rely if needed

	switch(request)
	{
	case TCGETS: //get stty stuff
		{
			linux::termios rec;
			WriteFile(m_pConsole->m_hIoctlWrite, &request, sizeof(DWORD), &dwDone,0);
			ReadFile(m_pConsole->m_hIoctlRead, &rec, sizeof(rec), &dwDone,0);

			//send to process
			P->WriteMemory((ADDR)data, sizeof(rec), &rec);
		}
		break;

	case TCSETSW:
		//flush output
		//console io never buffered, so not required
		//...fall through...
	case TCSETSF:
		//flush input & output
		//(done in console process)
		//...fall through...
	case TCSETS: //set stty stuff
		{
			linux::termios rec;

			P->ReadMemory(&rec, (ADDR)data, sizeof(rec));

			WriteFile(m_pConsole->m_hIoctlWrite, &request, sizeof(DWORD), &dwDone,0);
			WriteFile(m_pConsole->m_hIoctlWrite, &rec, sizeof(rec), &dwDone,0);
		}
		break;

	case TIOCGPGRP: //get process group
		{
			if(!data)
			{
				dwRet = -EFAULT;
				break;
			}
			P->WriteMemory((ADDR)data, sizeof(linux::pid_t), &m_pConsole->m_ProcessGroup);
		}
		break;
	case TIOCSPGRP: //set process group
		{
			if(!data)
			{
				dwRet = -EFAULT;
				break;
			}
			P->ReadMemory(&m_pConsole->m_ProcessGroup, (ADDR)data, sizeof(linux::pid_t));
		}
		break;

	case TIOCGWINSZ: //get window size
		{
			linux::winsize rec;
			WriteFile(m_pConsole->m_hIoctlWrite, &request, sizeof(DWORD), &dwDone,0);
			ReadFile(m_pConsole->m_hIoctlRead, &rec, sizeof(rec), &dwDone,0);

			//send to process
			P->WriteMemory((ADDR)data, sizeof(rec), &rec);
		}
		break;
	case TIOCSWINSZ: //set window size
		{
			linux::winsize rec;

			P->ReadMemory(&rec, (ADDR)data, sizeof(rec));

			WriteFile(m_pConsole->m_hIoctlWrite, &request, sizeof(DWORD), &dwDone,0);
			WriteFile(m_pConsole->m_hIoctlWrite, &rec, sizeof(rec), &dwDone,0);
		}
		break;

	case TCXONC: //TODO:
		dwRet = 0;
		break;

	default:
		ktrace("IMPLEMENT sys_ioctl 0x%lx for IOHNtConsole\n", request);
		dwRet = -ENOSYS;
		break;
	}

	LeaveCriticalSection(&m_pConsole->m_csIoctl);
	return dwRet;
}
