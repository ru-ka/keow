// MountPoint.h: interface for the MountPoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MOUNTPOINT_H__FA34E576_A49E_41D5_9108_47937854CEDD__INCLUDED_)
#define AFX_MOUNTPOINT_H__FA34E576_A49E_41D5_9108_47937854CEDD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Filesystem;

class MountPoint  
{
public:
	bool Unmount();

	static MountPoint* Mount( Path& UnixMountPoint, string sDestination, Filesystem* pFS, BYTE * pData, int nDataLen);

	const string& GetDestination()
	{
		return m_sDestination;
	}
	const Path& GetUnixMountPoint()
	{
		return m_UnixMountPoint;
	}

protected:
	friend Filesystem;

	MountPoint();
	virtual ~MountPoint();

	Path	m_UnixMountPoint;
	string	m_sDestination;
	Filesystem * m_pFileSystem;
	void * m_pData;
	int m_nDataLength;
};

#endif // !defined(AFX_MOUNTPOINT_H__FA34E576_A49E_41D5_9108_47937854CEDD__INCLUDED_)
