#include "kernel.h"


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID p )
{
	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:
		CoInitialize(0);
		kernel_init();
		break;

	case DLL_PROCESS_DETACH:
		kernel_term();
		CoUninitialize();
		break;
	}
	return TRUE;
}


