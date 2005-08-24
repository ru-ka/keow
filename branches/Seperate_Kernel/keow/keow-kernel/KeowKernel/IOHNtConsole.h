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
	IOHNtConsole(string Title);
	virtual ~IOHNtConsole();

	virtual bool Write(void* buffer, DWORD count, DWORD& written);
	virtual bool Read(void* buffer, DWORD count, DWORD& read);

	virtual HANDLE GetWriteHandle();
	virtual HANDLE GetReadHandle();

	virtual IOHandler* clone();

protected:
	IOHNtConsole();
	static const char * ms_ConsoleProcessExe;

	string m_Title;
	HANDLE m_hConsoleRead, m_hConsoleWrite;
};

#endif // !defined(AFX_IOHNTCONSOLE_H__0DCCBAED_CE76_4433_A2D4_0FD17F465626__INCLUDED_)
