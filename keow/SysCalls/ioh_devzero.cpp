/*
 * an IO handler for /dev/zero
 */
#include "kernel.h"
#include "iohandler.h"


DevZeroIOHandler::DevZeroIOHandler() 
: IOHandler(sizeof(DevZeroIOHandler))
{
}


bool DevZeroIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	ZeroMemory(address, size);
	if(pRead)
		*pRead = size;
	return true;
}


bool DevZeroIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	if(pWritten)
		*pWritten = size;
	return true;
}


IOHandler* DevZeroIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	//nothing special required
	return new DevZeroIOHandler();
}


DWORD DevZeroIOHandler::ioctl(DWORD request, DWORD data)
{
	//ignore all
	return 0;
}

bool DevZeroIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFCHR);

	return true;
}
