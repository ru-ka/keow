// Process.h: interface for the Process class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESS_H__065A3BC3_71C3_4302_8E39_297E193A46AF__INCLUDED_)
#define AFX_PROCESS_H__065A3BC3_71C3_4302_8E39_297E193A46AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//type of a Keow/Unix Process ID
typedef unsigned long PID;

//pointer
typedef BYTE* ADDR;

#define MAX_PENDING_SIGNALS 128

class Process  
{
public:
	void GenerateCoreDump();
	void InvokeStubFunction(StubFunc func, DWORD param1=0, DWORD param2=0, DWORD param3=0, DWORD param4=0);
	void SendSignal(int sig);
	void HandleSignal(int sig);

	bool WriteMemory(ADDR addr, DWORD len, LPVOID pBuf);
	bool ReadMemory(LPVOID pBuf, ADDR addr, DWORD len);

	DWORD StartNewImageRunning();
	static Process* StartInit(PID pid, Path& path, char ** InitialEnvironment);

	virtual ~Process();

	//process handler thread
	static DWORD WINAPI KernelProcessHandlerMain(LPVOID param);
	static const int KERNEL_PROCESS_THREAD_STACK_SIZE;
	static const char * KEOW_PROCESS_STUB;

	PID m_Pid;
	PID m_ParentPid;

	int m_gid, m_uid;

	PROCESS_INFORMATION m_Win32PInfo;
	DWORD m_dwExitCode;
	FILETIME m_StartedTime;

	DWORD m_KernelThreadId;

	Path m_ProcessFileImage;
	Path m_UnixPwd;

	string m_CommandLine;

	struct MemoryAlloc
	{
		ADDR addr;
		DWORD len;
		DWORD protection;

		MemoryAlloc(ADDR addrActual, DWORD size, DWORD prot)
			: addr(addrActual), len(size), protection(prot)
		{
		}
	};
	typedef list<MemoryAlloc*> MemoryAllocationsList;
	MemoryAllocationsList m_MemoryAllocations;

	struct ElfLoadData {
		ADDR phdr_addr;
		DWORD phdr_phnum;
		DWORD phdr_phent;

		ADDR start_addr;
		ADDR program_base, program_max;
		ADDR interpreter_base;
		ADDR interpreter_start;
		ADDR bss_start, brk;
		ADDR last_lib_addr;
	};
	ElfLoadData	m_ElfLoadData;

	struct PtraceData {
		PID OwnerPid;
		DWORD Request;
		CONTEXT ctx;
		DWORD Saved_Eax;
		int new_signal;
	};
	PtraceData m_ptrace;

	//info about what resources the stub can provide
	StubFunctionsInfo m_StubFunctionsInfo;

	ADDR m_Environment;
	ADDR m_Arguments;

	//Data that the debugger thread uses to initialise this process
	bool m_bDoExec, m_bDoFork;
	HANDLE m_hProcessStartEvent;
	bool m_bInWin32Setup;

	//signal handling
	bool m_PendingSignals[_NSIG];
	linux::sigset_t m_SignalMask[MAX_PENDING_SIGNALS];
	linux::sigaction m_SignalAction[_NSIG];
	int m_SignalDepth;

private:
	Process(); //private - use methods to create
protected:
	void SetSingleStep(bool set);
	void ForkCopyOtherProcess(Process * pOther);
	void CopyProcessHandles(Process* pParent);
	void HandleException(DEBUG_EVENT &evt);
	void ConvertProcessToKeow();
	void DebuggerLoop();

	DWORD LoadElfImage(HANDLE hImg, struct linux::elf32_hdr * pElf, ElfLoadData * pElfLoadData, bool LoadAsLibrary);
	DWORD LoadImage(Path img, bool LoadAsLibrary);
};

#endif // !defined(AFX_PROCESS_H__065A3BC3_71C3_4302_8E39_297E193A46AF__INCLUDED_)
