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

protected:
	HANDLE m_hRemotePipe;
};

#endif // !defined(AFX_IOHPIPE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_)
