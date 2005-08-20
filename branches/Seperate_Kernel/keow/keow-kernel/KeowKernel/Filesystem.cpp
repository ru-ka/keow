// Filesystem.cpp: implementation of the Filesystem class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "Filesystem.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Filesystem::Filesystem()
{
}

Filesystem::~Filesystem()
{
}

void Filesystem::SetAssociatedMount(MountPoint &mp)
{
	m_pAssociatedMountPoint = &mp;
}
