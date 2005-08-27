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

// Process.h: interface for the Process class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESS_H__065A3BC3_71C3_4302_8E39_297E193A46AF__INCLUDED_)
#define AFX_PROCESS_H__065A3BC3_71C3_4302_8E39_297E193A46AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//type of a Keow/Unix Process ID (needs to be signed)
typedef long PID;

//pointer
typedef BYTE* ADDR;


//some process limits
#define MAX_PENDING_SIGNALS  128
#define MAX_OPEN_FILES 1024


class Process  
{
public:
	void GenerateCoreDump();
	void SendSignal(int sig);
	void HandleSignal(int sig);

	bool WriteMemory(ADDR addr, DWORD len, LPVOID pBuf);
	bool ReadMemory(LPVOID pBuf, ADDR addr, DWORD len);

	DWORD StartNewImageRunning();
	static Process* StartInit(PID pid, Path& path, char ** InitialEnvironment);

	DWORD InjectFunctionCall(void *func, void *pStackData, int nStackDataSize);

	virtual ~Process();

	//process handler thread
	static DWORD WINAPI KernelProcessHandlerMain(LPVOID param);
	static const int KERNEL_PROCESS_THREAD_STACK_SIZE;
	static const char * KEOW_PROCESS_STUB;

	PID m_Pid;
	PID m_ParentPid;
	PID m_ProcessGroupPID;

	int m_gid, m_uid;
	int m_egid, m_euid;
	int m_saved_uid, m_saved_gid;
	int m_umask;

	PROCESS_INFORMATION m_Win32PInfo;
	DWORD m_dwExitCode;
	FILETIME m_StartedTime;
	HANDLE m_hWaitTerminatingEvent;

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
		bool ctx_valid;
		DWORD Saved_Eax;
		int new_signal;
	};
	PtraceData m_ptrace;

	//info about what resources the stub can provide
	SysCallDll::RemoteAddrInfo SysCallAddr;

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

	//open files
	IOHandler * m_OpenFiles[MAX_OPEN_FILES];

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
