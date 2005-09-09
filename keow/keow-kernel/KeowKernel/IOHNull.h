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
	virtual DWORD ioctl(DWORD request, DWORD data);

	virtual HANDLE GetRemoteWriteHandle();
	virtual HANDLE GetRemoteReadHandle();

	virtual IOHandler* clone();

	virtual bool Stat64(linux::stat64 * s);

private:
	HANDLE m_hRemoteHandle;
};

#endif // !defined(AFX_IOHNULL_H__A82590CD_8CD7_4759_B40F_0563D597E0C6__INCLUDED_)
