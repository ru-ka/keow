// IOHNull.h: interface for the IOHNull class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOHNULL_H__A82590CD_8CD7_4759_B40F_0563D597E0C6__INCLUDED_)
#define AFX_IOHNULL_H__A82590CD_8CD7_4759_B40F_0563D597E0C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IOHandler.h"

class IOHNull : public IOHandler  
{
public:
	IOHNull();
	virtual ~IOHNull();

	virtual bool Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags);
	virtual bool Close();
	virtual DWORD ioctl(DWORD request, DWORD data);

	virtual bool Read(void* address, DWORD size, DWORD *pRead);
	virtual bool Write(void* address, DWORD size, DWORD *pWritten);

	virtual __int64 Length();
	virtual __int64 Seek(__int64 offset, DWORD method);

	virtual HANDLE GetRemoteWriteHandle();
	virtual HANDLE GetRemoteReadHandle();

	virtual IOHandler* Duplicate();

	virtual bool Stat64(linux::stat64 * s);

	virtual bool CanRead();
	virtual bool CanWrite();
	virtual bool HasException();

private:
	HANDLE m_hRemoteHandle;
};

#endif // !defined(AFX_IOHNULL_H__A82590CD_8CD7_4759_B40F_0563D597E0C6__INCLUDED_)
