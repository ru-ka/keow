// Stub Main


#include <windows.h>
#include "../../keow-kernel/KeowKernel/StubResourceInfo.h"

//global
struct StubFunctionsInfo FuncInfo;

#define STUB_EXIT __asm int 3

//////////////////////////////////////////////////////////////////////////////////////


void _stdcall Stub_ExitProcess(DWORD param1, DWORD param2, DWORD param3, DWORD param4)
{
	ExitProcess(param1);

	STUB_EXIT
}

void _stdcall Stub_Write(DWORD param1, DWORD param2, DWORD param3, DWORD param4)
{
	WriteFile((HANDLE)param1, (LPVOID)param2, param3, &param4, NULL);

	STUB_EXIT
}


//////////////////////////////////////////////////////////////////////////////////////


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//Set the info about the win32 side.
	//The stub provides this info in Eax prior to causing an initial breakpoint 
	//to send it to the kernel
	//(see Process::ConvertProcessToKeow()

	FuncInfo.ExitProcess = Stub_ExitProcess;
	FuncInfo.Write = Stub_Write;

	//pass control to the kernel to re-develop use into a keow process
	__asm {
		lea eax, FuncInfo

		//set single-step mode to tell kernel we are ready
		pushfd
		pop ebx
		or ebx, 0x100   //trap bit
		push ebx
		popfd
	}


	//should never get here
	ExitProcess(-1);
	return -1;
}
