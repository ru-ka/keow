/*
 * and IO handler for named and anonymous pipes
 */
#include "kernel.h"
#include "iohandler.h"


PipeIOHandler::PipeIOHandler() 
: IOHandler(sizeof(PipeIOHandler))
{
	m_Handle = INVALID_HANDLE_VALUE;
}

PipeIOHandler::PipeIOHandler(HANDLE h) 
: IOHandler(sizeof(PipeIOHandler))
{
	m_Handle = h;
}


bool PipeIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	if((m_Flags&O_NONBLOCK) == 0)
	{
		if(ReadFile(m_Handle, address, size, pRead, NULL)!=FALSE)
			return true;
		if(GetLastError()==ERROR_BROKEN_PIPE)
		{
			*pRead = 0;
			return true;
		}
		return false;
	}

	DWORD dwBytes;
	if(!PeekNamedPipe(m_Handle, NULL,0, NULL, &dwBytes, NULL))
	{
		if(GetLastError()==ERROR_BROKEN_PIPE)
		{
			*pRead = 0;
			return true;
		}
		return false;
	}

	if(dwBytes > size)
		dwBytes = size;

	return ReadFile(m_Handle, address, dwBytes, pRead, NULL)!=FALSE;
}


bool PipeIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	return WriteFile(m_Handle, address, size, pWritten, NULL)!=FALSE;
}


IOHandler* PipeIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	PipeIOHandler *ionew = new PipeIOHandler();
	if(ionew)
	{
		if(ionew->DupBaseData(*this, hFromProcess, hToProcess))
			return ionew;

		delete ionew;
	}
	return NULL;
}


DWORD PipeIOHandler::ioctl(DWORD request, DWORD data)
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
		ktrace("IMPLEMENT sys_ioctl 0x%lx for PipeIOHandler\n", request);
		return -ENOSYS;
	}
}


bool PipeIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFIFO);

	return true;
}


bool PipeIOHandler::CanRead()
{
	//ok if we are not at eof
	DWORD dwAvail = 0;
	if(!PeekNamedPipe(m_Handle, NULL, 0, NULL, &dwAvail, NULL))
		return false;
	return dwAvail != 0;
}

bool PipeIOHandler::CanWrite()
{
	//TODO: how will we know?
	return true;
}

bool PipeIOHandler::HasException()
{
	//TODO: what could this be?
	return false;
}
