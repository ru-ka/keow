/*
 * an IO handler for /dev/null
 */
#include "kernel.h"
#include "iohandler.h"


DevNullIOHandler::DevNullIOHandler() 
: IOHandler(sizeof(DevNullIOHandler))
{
}


bool DevNullIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	if(pRead)
		*pRead = 0;
	//never can read from /dev/null, block if we can
	while((m_Flags&O_NONBLOCK)==0)
		Sleep(10000);
	return true;
}


bool DevNullIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	if(pWritten)
		*pWritten = size;
	return true;
}


IOHandler* DevNullIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	//nothing special required
	return new DevNullIOHandler();
}


DWORD DevNullIOHandler::ioctl(DWORD request, DWORD data)
{
	//ignore all
	return 0;
}

bool DevNullIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFCHR);

	return true;
}
