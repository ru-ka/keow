/*
 * generate core dumps
 */
#include "kernel.h"
#include "loadelf.h"


void GenerateCoreDump()
{
	pProcessData->core_dumped = true;

	HANDLE hCore = CreateFile("core",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
	if(hCore==INVALID_HANDLE_VALUE)
	{
		ktrace("FAIL writing core file: err %lx\n", GetLastError());
		return;
	}

	//TODO implement a real core dump
	DWORD written;
	WriteFile(hCore, "Core dump of ", 13, &written, 0);
	WriteFile(hCore, (char*)pProcessData->ProgramPath, strlen((char*)pProcessData->ProgramPath), &written, 0);


	CloseHandle(hCore);
}
