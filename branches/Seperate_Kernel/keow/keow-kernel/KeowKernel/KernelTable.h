// KernelTable.h: interface for the KernelTable class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KERNELTABLE_H__232A87EB_0936_4CC2_8C52_5412CEC21F68__INCLUDED_)
#define AFX_KERNELTABLE_H__232A87EB_0936_4CC2_8C52_5412CEC21F68__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Process.h"
#include "Device.h"
#include "MountPoint.h"
#include "Path.h"


class KernelTable  
{
public:
	KernelTable();
	virtual ~KernelTable();

	Process * FindProcess(DWORD pid);

	SYSTEMTIME m_BootTime;
	DWORD m_BogoMips;

	int m_DebugLevel;

	string m_FilesystemRoot;
	MountPoint * m_pRootMountPoint;

	int m_LastPID;

	typedef list<Process*> ProcessList;
	ProcessList m_Processes;

	typedef list<MountPoint*> MountPointList;
	MountPointList m_MountPoints;

	typedef list<Device*> DeviceList;
	DeviceList m_Devices;

	//HANDLE s_SysCallTable();
};


struct KernelThreadLocals
{
	Process * pProcess;
	char ktrace_buffer[8000];
};



//refs to the global single copies
extern KernelTable * g_pKernelTable;
extern _declspec(thread) KernelThreadLocals * g_pKernelThreadLocals;


#endif // !defined(AFX_KERNELTABLE_H__232A87EB_0936_4CC2_8C52_5412CEC21F68__INCLUDED_)
