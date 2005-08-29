// IOHNtConsole.h: interface for the IOHNtConsole class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOHNTCONSOLE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_)
#define AFX_IOHNTCONSOLE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class IOHNtConsole : public IOHandler
{
public:
	IOHNtConsole(DevConsole * pConsole);
	virtual ~IOHNtConsole();

	virtual bool Open(DWORD win32access, DWORD win32share, DWORD disposition, DWORD flags);

	virtual HANDLE GetRemoteWriteHandle();
	virtual HANDLE GetRemoteReadHandle();
	virtual HANDLE GetKernelWriteHandle();
	virtual HANDLE GetKernelReadHandle();

	virtual IOHandler* clone();

protected:
	HANDLE m_hRemoteConsoleRead, m_hRemoteConsoleWrite;
	DevConsole * m_pConsole;
};

#endif // !defined(AFX_IOHNTCONSOLE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_)
