// Main


#include "includes.h"
#include "KernelStartup.h"


//single copy of the kernel data
KernelTable * g_pKernelTable;

//thread local stuff for current thread state in kernel handler
_declspec(thread) KernelThreadLocals * g_pKernelThreadLocals;


static char * InitialEnv[] = {
	"OLDPWD=/",
	"HOME=/",
	"TERM=ansi80x25", //freebsd console (25-line ansi mode) -- easier to implement than the linux console -- implemented in ioh_console.cpp
	"PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin",
	"PWD=/",
	NULL
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	g_pKernelThreadLocals = new KernelThreadLocals();
	g_pKernelThreadLocals->pProcess = NULL;

	g_pKernelTable= new KernelTable();


	ktrace("Kernel Booting\n");

	KernelStartup::ValidateKernelTraps();

	CoInitialize(NULL); //Ex(NULL, COINIT_MULTITHREADED);

	KernelStartup::ProcessCommandLine(lpCmdLine);
	if(g_pKernelTable->m_FilesystemRoot.length() == 0)
	{
		ktrace("No root= specified\n");
		halt();
	}


	//AutoMount windows drive letters
	KernelStartup::AutoMountDrives();
	
	
	//Load the initial executable
	ktrace("Loading 'init'");
	Path InitPath;
	InitPath.SetUnixPath( KernelStartup::GetInitProgram() );
	Process * proc = Process::StartInit(1, InitPath, InitialEnv);
	if(proc == 0)
	{
		ktrace("Failed to load 'init'\n");
		halt();
	}


	ktrace("'init' started\n");

	//wait for it to exit
	MsgWaitForMultipleObjects(1, &proc->m_Win32PInfo.hProcess, FALSE, INFINITE, NULL);
	ktrace("'init' appears to have terminated, ");

	ktrace("exit code 0x%lx (%ld)\n", proc->m_dwExitCode,proc->m_dwExitCode);


	halt();
	return -1; //never get here
}
