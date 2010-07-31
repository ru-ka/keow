// IOHPipe.cpp: implementation of the IOHPipe class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "IOHPipe.h"


//////////////////////////////////////////////////////////////////////

IOHPipe::IOHPipe(HANDLE hPipe)
{
	//make it inheritable
	DuplicateHandle(GetCurrentProcess(), hPipe, P->m_Win32PInfo.hProcess, &m_hRemotePipe, 0, TRUE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
}

IOHPipe::~IOHPipe()
{
	Close();
}


HANDLE IOHPipe::GetRemoteWriteHandle()
{
	return m_hRemotePipe;
}
HANDLE IOHPipe::GetRemoteReadHandle()
{
	return m_hRemotePipe;
}

IOHandler * IOHPipe::Duplicate()
{
	IOHPipe * p2 = new IOHPipe(NULL); //dup in constructor will fail, but we deal with it next

	DuplicateHandle(m_pInProcess->m_Win32PInfo.hProcess, m_hRemotePipe,
		P->m_Win32PInfo.hProcess, &p2->m_hRemotePipe,
		0, TRUE, DUPLICATE_SAME_ACCESS);

	return p2;
}

bool IOHPipe::Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags)
{
	return true;
}

bool IOHPipe::Close()
{
	if(m_hRemotePipe != INVALID_HANDLE_VALUE)
		SysCallDll::CloseHandle(m_hRemotePipe);
	m_hRemotePipe = INVALID_HANDLE_VALUE;

	return true;
}


bool IOHPipe::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFIFO); //fifo - aha pipe

	return true;
}


DWORD IOHPipe::ioctl(DWORD request, DWORD data)
{
	DWORD dwRet;

	switch(request)
	{
	case 0:
	default:
		ktrace("IMPLEMENT sys_ioctl 0x%lx for IOHPipe\n", request);
		dwRet = -ENOSYS;
		break;
	}

	return dwRet;
}

bool IOHPipe::Read(void* address, DWORD size, DWORD *pRead)
{
	if(m_Flags&O_NONBLOCK)
	{
		//non-blocking - ensure we can do the read before doing it
		DWORD dwBytes;
		dwBytes = SysCallDll::PeekAvailablePipe(m_hRemotePipe);
		if(dwBytes < size)
		{
			if(dwBytes==0)
			{
				if(SysCallDll::GetLastError()==ERROR_BROKEN_PIPE)
				{
					//EOF
					*pRead = 0;
					return true;
				}
			}
			//can't read without blocking
			return false;
		}
	}

	//read
	*pRead = SysCallDll::ReadFile(m_hRemotePipe, address, size);
	if(*pRead==0)
	{
		if(SysCallDll::GetLastError()==ERROR_BROKEN_PIPE)
		{
			//EOF
			return true;
		}

		//failed read
		return false;
	}
	return true;
}


bool IOHPipe::Write(void* address, DWORD size, DWORD *pWritten)
{
	*pWritten = SysCallDll::WriteFile(m_hRemotePipe, address, size);
	return *pWritten!=0;
}


bool IOHPipe::CanRead()
{
	//ok if we are not at eof
	DWORD dwAvail = SysCallDll::PeekAvailablePipe(m_hRemotePipe);
	return dwAvail!=0;
}

bool IOHPipe::CanWrite()
{
	//TODO: how will we know?
	return true;
}

bool IOHPipe::HasException()
{
	//TODO: what could this be?
	return false;
}

__int64 IOHPipe::Length()
{
	return 0;
}

__int64 IOHPipe::Seek(__int64 offset, DWORD method)
{
	return -1;
}