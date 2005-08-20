// MountPoint.cpp: implementation of the MountPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "MountPoint.h"

//////////////////////////////////////////////////////////////////////

MountPoint::MountPoint()
{

}

MountPoint::~MountPoint()
{
	//no longer need the handler
	delete m_pFileSystem;

	delete m_pData;
}


MountPoint* MountPoint::Mount( Path& UnixMountPoint, string sDestination, Filesystem* pFS, BYTE * pData, int nDataLen)
{
	MountPoint * pMP = new MountPoint();

	pMP->m_UnixMountPoint = UnixMountPoint;
	pMP->m_sDestination = sDestination;

	pMP->m_pFileSystem = pFS;
	pMP->m_pFileSystem->SetAssociatedMount(*pMP);

	pMP->m_pData = new BYTE[nDataLen];
	pMP->m_nDataLength = nDataLen;
	memcpy(pMP->m_pData, pData, nDataLen);

	//add to the kernel
	g_pKernelTable->m_MountPoints.push_back(pMP);

	return pMP;
}
