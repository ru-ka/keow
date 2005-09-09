/*
 * Copyright 2005 Paul Walker
 *
 * GNU General Public License
 * 
 * This file is part of: Kernel Emulation on Windows (keow)
 *
 * Keow is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Keow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Keow; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// Path.cpp: implementation of the Path class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "Path.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Path::Path(bool FollowSymLinks)
{
	m_FollowSymLinks = FollowSymLinks;
}
Path::Path(string UnixPath, bool FollowSymLinks)
{
	m_FollowSymLinks = FollowSymLinks;
	SetUnixPath(UnixPath);
}
Path::Path(LPCSTR UnixPath, bool FollowSymLinks)
{
	m_FollowSymLinks = FollowSymLinks;
	SetUnixPath(UnixPath);
}

Path::~Path()
{
}


Path& Path::operator=(const Path& other)
{
	m_FollowSymLinks = other.m_FollowSymLinks;
	m_PathStack = other.m_PathStack;
	return *this;
}


void Path::SetUnixPath(string path)
{
	m_PathStack.clear();

	if(path[0] == '/')
	{
		//absolute path
	}
	else
	{
		//relative to current processes path
		AppendUnixPath( P->m_UnixPwd.GetUnixPath() );
	}

	AppendUnixPath(path);

	ktrace("path: %s\n", path.c_str());
	ktrace("  --> %s\n", GetUnixPath().c_str());
	ktrace("  --> %s\n", GetWin32Path().c_str());
}



void Path::FollowSymLinks(bool follow)
{
	m_FollowSymLinks = follow;
}


void Path::AppendUnixPath(string unixp)
{
	//reset to root? (absolute path)
	if(unixp[0] == '/')
	{
		m_PathStack.clear();
	}

	//add individual path elements
	int pos = 0;
	int next_pos;
	string element;

	for(;;)
	{
		next_pos = unixp.find('/', pos);
		if(next_pos < 0)
		{
			//end of the path
			element = unixp.substr(pos);
		}
		else
		{
			//peice in the middle of the path
			element = unixp.substr(pos, next_pos-pos);
		}

		if(element.compare(".") == 0)
		{
			//ignore redundant reference to current dir
		}
		else
		if(element.compare("..") == 0)
		{
			//pop an element
			if(!m_PathStack.empty())
				m_PathStack.pop_back();
		}
		else
		if(!element.empty())
		{
			//add and element to the path
			m_PathStack.push_back(element);
		}

		if(next_pos < 0)
			break; //at end

		//skip over the slash we last found
		pos = next_pos + 1;
	}
}


//helper to turn a list into a final path
//
string Path::JoinList(Path::ElementList& lst, char delimiter)
{
	string final;

	Path::ElementList::iterator it;
	for(it=lst.begin(); it!=lst.end(); ++it)
	{
		const string& element = *it;
		if(!final.empty())
			final += delimiter;
		final += element;
	}

	return final;
}


// build the unix path from the elements of the path
//
string Path::GetUnixPath()
{
	return string("/") + JoinList(m_PathStack, '/');
}


// build the win32 path from the elements of the path
// processing mount points as we go
//
string Path::GetWin32Path()
{
	TranverseMountPoints();
	return GetFinalPath();
}

string Path::GetFinalPath()
{
	return m_strMountRealPath
		 + (m_pFinalMountPoint ? m_pFinalMountPoint->GetFilesystem()->GetPathSeperator() : "/" )
		 + m_strPathInMountPoint;
}

// Follows the path across mount points
void Path::TranverseMountPoints()
{
	//start at root
	m_pFinalMountPoint = g_pKernelTable->m_pRootMountPoint;
	m_strPathInMountPoint = "";
	m_strMountRealPath = m_pFinalMountPoint ? m_pFinalMountPoint->GetDestination() : "";

	//follow path elements
	ElementList::iterator it;
	int cnt;
	for(it=m_PathStack.begin(),cnt=1; it!=m_PathStack.end(); ++it,++cnt)
	{
		const string& element = *it;

		//Check if this is a new point point to follow
		KernelTable::MountPointList::iterator mnt_it;
		bool bMountFound = false;
		for(mnt_it=g_pKernelTable->m_MountPoints.begin(); mnt_it!=g_pKernelTable->m_MountPoints.end(); ++mnt_it)
		{
			MountPoint* pMP = *mnt_it;

			if(pMP->GetUnixMountPoint().EqualsPartialPath(*this, cnt))
			{
				//a mount matches this current path
				m_pFinalMountPoint = pMP;
				m_strMountRealPath = pMP->GetDestination();
				m_strPathInMountPoint = "";
				bMountFound = true;
			}
		}
		if(bMountFound)
			continue;

		//Check if this is a link to follow
		if(m_FollowSymLinks)
		{
			//path as it currently has been calculated
			string curpath = GetFinalPath()
				           + m_pFinalMountPoint->GetFilesystem()->GetPathSeperator()
						   + element;

			string dest = m_pFinalMountPoint->GetFilesystem()->GetLinkDestination(curpath);

			if(!dest.empty())
			{
				//TODO: handle links ACROSS filesystems - How?

				if(m_pFinalMountPoint->GetFilesystem()->IsRelativePath(dest))
				{
					m_strPathInMountPoint += m_pFinalMountPoint->GetFilesystem()->GetPathSeperator();
					m_strPathInMountPoint += dest;
				}
				else
				{
					m_strMountRealPath = "";
					m_strPathInMountPoint = dest;
				}
				continue;
			}
		}

		//just a normal element
		m_strPathInMountPoint += m_pFinalMountPoint->GetFilesystem()->GetPathSeperator();
		m_strPathInMountPoint += element;
	}

}

// tests to see if this path (in it's entirety) equals the portion of the other path
//
bool Path::EqualsPartialPath(const Path& other, int otherLen) const
{
	if(otherLen != m_PathStack.size()		//we aren't correct length to match
	|| otherLen > other.m_PathStack.size()) //otherlen is invalid
	{
		//no way this path is long enough to match
		return false;
	}

	ElementList::iterator itThis, itOther;
	int cnt;
	for(cnt=0, itThis=m_PathStack.begin(), itOther=other.m_PathStack.begin();
	    cnt<otherLen;
	    ++cnt, ++itThis, ++itOther)
	{
			if(*itThis != *itOther)
				return false; //mismatch
	}

	//appears to match
	return true;
}



bool Path::IsSymbolicLink()
{
	if(m_FollowSymLinks)
		return false; //it's not because we'll always resolve them to a real location

	TranverseMountPoints();
	return m_pFinalMountPoint->GetFilesystem()->IsSymbolicLink( GetFinalPath() );
}


//Returns filesystem that the path unlimately resolves to
Filesystem * Path::GetFinalFilesystem()
{
	return m_pFinalMountPoint->GetFilesystem();
}
