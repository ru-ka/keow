#ifndef LKFW_MEM_H
#define LKFW_MEM_H


void RecordMemoryAllocation(ADDR addr, DWORD len, DWORD prot);
void RecordMemoryFree(ADDR addr, DWORD len);
void RecordMMapAllocation(HANDLE hFile, HANDLE hMap, DWORD offset, ADDR addr, DWORD len, DWORD prot);
void RecordMMapFree(ADDR addr, DWORD len);

bool ReadMemory(ADDR toAddr, HANDLE hFromProcess, ADDR fromAddr, int len);
bool WriteMemory(HANDLE hToProcess, ADDR toAddr, int len, ADDR fromAddr);

ADDR AllocateMemAndProtectProcess(HANDLE hProcess, ADDR addr, DWORD size, DWORD prot);
ADDR AllocateMemAndProtect(ADDR addr, DWORD size, DWORD prot);
bool DeallocateMemory(ADDR addr, DWORD size);

_declspec(dllexport) char** CopyStringListToProcess(HANDLE hProcess, char * list[]);


struct MemBlock
{
	ADDR addr;
	DWORD len;
};

#endif //LKFW_MEM_H
