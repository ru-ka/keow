/*
 * Copyright 2005 Paul Walker
 *
 * GNU General Public License
 * 
 * This file is part of: Kernel Emulation on Windows (keow)
 *
 * Keow is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Keow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Keow; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef KEOW_IOHANDLER_H
#define KEOW_IOHANDLER_H

#include "linux_includes.h"

struct TerminalDeviceDataStruct;


/*
 * class to abstract handling of io for various device types
 */
class IOHandler
{
public:
	virtual ~IOHandler();

	virtual bool Read(void* address, DWORD size, DWORD *pRead) = 0;
	virtual bool Write(const void* address, DWORD size, DWORD *pWritten) = 0;
	virtual IOHandler* Duplicate(HANDLE hFromProcess, HANDLE hToProcess) = 0;
	virtual DWORD ioctl(DWORD request, DWORD data) = 0;
	virtual bool Stat64(linux::stat64 *) = 0;

	virtual const char * GetWin32Path();
	virtual const char * GetUnixPath();

	virtual bool Close();

	//for select() to use
	virtual bool CanRead() = 0;
	virtual bool CanWrite() = 0;
	virtual bool HasException() = 0;

	bool Stat(linux::stat *);

	inline HANDLE GetHandle()					{return m_Handle;}
	inline void SetHandle(HANDLE h)				{m_Handle=h;}
	inline bool GetInheritable()				{return m_Inheritable;}
	inline void SetInheritable(bool inherit)	{m_Inheritable=inherit;}
	inline DWORD GetFlags()						{return m_Flags;}
	inline void SetFlags(DWORD flags)			{m_Flags=flags;}
	inline int GetSize()						{return m_ClassSize;}

protected:
	IOHandler(int size);
	bool DupBaseData(IOHandler& from, HANDLE hFromProcess, HANDLE hToProcess);
	void BasicStat64(linux::stat64 * s, int file_type);

	char m_Path[MAX_PATH];
	int m_ClassSize;
	HANDLE m_Handle;
	bool m_Inheritable;
	DWORD m_Flags;
};



class FileIOHandler : public IOHandler
{
public:
	FileIOHandler();

	//FileIOHandler& operator=(FileIOHandler& other);

	bool Open(const char * filename, DWORD access, DWORD ShareMode, DWORD disposition, DWORD flags);
	bool Read(void* address, DWORD size, DWORD *pRead);
	bool Write(const void* address, DWORD size, DWORD *pWritten);
	IOHandler* Duplicate(HANDLE hFromProcess, HANDLE hToProcess);
	DWORD ioctl(DWORD request, DWORD data);
	bool Stat64(linux::stat64 *);

	bool Close();

	bool CanRead();
	bool CanWrite();
	bool HasException();

	int GetDirEnts64(linux::dirent64 *, int maxbytes);
	DWORD Length();

	DWORD Seek(DWORD offset, DWORD method);

protected:
	bool m_IsADirectory;
	HANDLE m_hFindData;
	int m_nFindCount;
};


class PipeIOHandler : public IOHandler
{
public:
	PipeIOHandler();
	PipeIOHandler(HANDLE h);

	//PipeIOHandler& operator=(PipeIOHandler& other);

	bool Read(void* address, DWORD size, DWORD *pRead);
	bool Write(const void* address, DWORD size, DWORD *pWritten);
	IOHandler* Duplicate(HANDLE hFromProcess, HANDLE hToProcess);
	DWORD ioctl(DWORD request, DWORD data);
	bool Stat64(linux::stat64 *);

	bool CanRead();
	bool CanWrite();
	bool HasException();

protected:
};


class ConsoleIOHandler : public IOHandler
{
public:
	ConsoleIOHandler(int nDeviceNum, bool initialise);

	//ConsoleIOHandler& operator=(ConsoleIOHandler& other);

	bool Read(void* address, DWORD size, DWORD *pRead);
	bool Write(const void* address, DWORD size, DWORD *pWritten);
	IOHandler* Duplicate(HANDLE hFromProcess, HANDLE hToProcess);
	DWORD ioctl(DWORD request, DWORD data);
	bool Stat64(linux::stat64 *);

	bool CanRead();
	bool CanWrite();
	bool HasException();

protected:
	bool ReadChar(bool canblock, char &c);
	bool WriteChar(char c);
	bool PushForNextRead(const char * pString);

	HANDLE m_HandleOut;		//extra handle because CONIN$ and CONOUT$ are seperate but a handler may need to do both
	int m_nTerminalNumber;	//index into kernel terminal device data struct
	TerminalDeviceDataStruct &m_DeviceData; //reference to corresponding device data
};


class DevZeroIOHandler : public IOHandler
{
public:
	DevZeroIOHandler();

	bool Read(void* address, DWORD size, DWORD *pRead);
	bool Write(const void* address, DWORD size, DWORD *pWritten);
	IOHandler* Duplicate(HANDLE hFromProcess, HANDLE hToProcess);
	DWORD ioctl(DWORD request, DWORD data);
	bool Stat64(linux::stat64 *);

	bool CanRead();
	bool CanWrite();
	bool HasException();

protected:
};


class DevNullIOHandler : public IOHandler
{
public:
	DevNullIOHandler();

	bool Read(void* address, DWORD size, DWORD *pRead);
	bool Write(const void* address, DWORD size, DWORD *pWritten);
	IOHandler* Duplicate(HANDLE hFromProcess, HANDLE hToProcess);
	DWORD ioctl(DWORD request, DWORD data);
	bool Stat64(linux::stat64 *);

	bool CanRead();
	bool CanWrite();
	bool HasException();

protected:
};


#endif //KEOW_IOHANDLER_H
