// Stub Main


#include <windows.h>
#include "../KeowKernel/StubResourceInfo.h"

struct StubFunctionsInfo FuncInfo;


void _stdcall StubExitProcess(DWORD param1, DWORD param2, DWORD param3, DWORD param4)
{
	ExitProcess(param1);

	//standard stub function ending (marks end)
	__asm int 3
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//Set the info about the win32 side.
	//The stub provides this info in Eax prior to causing an initial breakpoint 
	//to send it to the kernel
	//(see Process::ConvertProcessToKeow()

	FuncInfo.ExitProcess = StubExitProcess;

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
