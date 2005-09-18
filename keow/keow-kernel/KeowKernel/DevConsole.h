// DevConsole.h: interface for the DevConsole class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVCONSOLE_H__8E978E10_AA78_4039_9F10_847E0D757C27__INCLUDED_)
#define AFX_DEVCONSOLE_H__8E978E10_AA78_4039_9F10_847E0D757C27__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class DevConsole : public Device
{
public:
	friend class IOHNtConsole;

	DevConsole(int tty);
	virtual ~DevConsole();

private:
	int m_tty;
	HANDLE m_hConsoleWrite, m_hConsoleRead;
	HANDLE m_hIoctlWrite, m_hIoctlRead;
	CRITICAL_SECTION m_csIoctl;
	linux::pid_t m_ProcessGroup;
};

#endif // !defined(AFX_DEVCONSOLE_H__8E978E10_AA78_4039_9F10_847E0D757C27__INCLUDED_)
