// Filesystem.h: interface for the Filesystem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILESYSTEM_H__4BFEC0CD_0A6A_42B5_9BB8_445350427B3E__INCLUDED_)
#define AFX_FILESYSTEM_H__4BFEC0CD_0A6A_42B5_9BB8_445350427B3E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Filesystem  
{
public:
	virtual void SetAssociatedMount(MountPoint& mp);

	Filesystem();
	virtual ~Filesystem();

protected:
	MountPoint * m_pAssociatedMountPoint;
};

#endif // !defined(AFX_FILESYSTEM_H__4BFEC0CD_0A6A_42B5_9BB8_445350427B3E__INCLUDED_)
