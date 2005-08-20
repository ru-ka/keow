// MemoryHelper.h: interface for the MemoryHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MEMORYHELPER_H__1A80F3C0_1A72_426A_B6E5_6D4BD43E2EC6__INCLUDED_)
#define AFX_MEMORYHELPER_H__1A80F3C0_1A72_426A_B6E5_6D4BD43E2EC6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class MemoryHelper  
{
public:
	static ADDR AllocateMemAndProtectProcess(HANDLE hProcess, ADDR addr, DWORD size, DWORD prot);
	static ADDR AllocateMemAndProtect(ADDR addr, DWORD size, DWORD prot);
	static bool DeallocateMemory(ADDR addr, DWORD size);
	static bool WriteMemory(HANDLE hToProcess, ADDR toAddr, int len, ADDR fromAddr);
	static bool ReadMemory(ADDR toAddr, HANDLE hFromProcess, ADDR fromAddr, int len);
	static bool TransferMemory(HANDLE hFromProcess, ADDR fromAddr, HANDLE hToProcess, ADDR toAddr, int len);
	static ADDR CopyStringListBetweenProcesses(HANDLE hFromProcess, ADDR pFromList, HANDLE hToProcess, DWORD * pdwCount);

private: 
	MemoryHelper();
	virtual ~MemoryHelper();
};

#endif // !defined(AFX_MEMORYHELPER_H__1A80F3C0_1A72_426A_B6E5_6D4BD43E2EC6__INCLUDED_)
