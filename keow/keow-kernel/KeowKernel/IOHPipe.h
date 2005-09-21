// IOHPipe.h: interface for the IOHPipe class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOHPIPE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_)
#define AFX_IOHPIPE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class IOHPipe : public IOHandler
{
public:
	IOHPipe(HANDLE hPipe);
	virtual ~IOHPipe();

	virtual bool Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags);
	virtual bool Close();
	virtual DWORD ioctl(DWORD request, DWORD data);

	virtual HANDLE GetRemoteWriteHandle();
	virtual HANDLE GetRemoteReadHandle();

	virtual IOHandler* Duplicate();

	virtual bool Stat64(linux::stat64 * s);

protected:
	HANDLE m_hRemotePipe;
};

#endif // !defined(AFX_IOHPIPE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_)
