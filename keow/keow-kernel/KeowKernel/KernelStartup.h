// KernelStartup.h: interface for the KernelStartup class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KERNELSTARTUP_H__B2AA20E4_FD73_4EA4_8C87_E3F083105A7D__INCLUDED_)
#define AFX_KERNELSTARTUP_H__B2AA20E4_FD73_4EA4_8C87_E3F083105A7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class KernelStartup  
{
public:
	static void ProcessCommandLine(LPSTR lpCmdLine);
	static void ValidateKernelTraps();
	static void AutoMountDrives();

	static string GetInitProgram()  {
		return s_InitProgram;
	}

protected:
	static string s_InitProgram;
	static const char * ms_pszAutoMount;

	typedef void (*ARG_HANDLER)(const char *);
	struct HandlerRec {
		const char * arg_name;
		ARG_HANDLER handler;
	};
	static HandlerRec s_ArgumentHandlers[];

	static void arg_root(const char *);
	static void arg_init(const char *);
	static void arg_debug(const char *);
	static void arg_automount(const char *);
};

#endif // !defined(AFX_KERNELSTARTUP_H__B2AA20E4_FD73_4EA4_8C87_E3F083105A7D__INCLUDED_)
