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

#include <Shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>

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
	//start at root
	string w32path;
	MountPoint * pMount = g_pKernelTable->m_pRootMountPoint;
	if(pMount)
		w32path = pMount->GetDestination();

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
				pMount = pMP;
				w32path = pMP->GetDestination();
				bMountFound = true;
			}
		}
		if(bMountFound)
			continue;

		//Check if this is a link to follow
		if(m_FollowSymLinks)
		{
			string w2 = w32path;
			w2 += '\\';
			w2 += element;
			w2 += ".lnk";
			if(GetFileAttributes(w2) != INVALID_FILE_ATTRIBUTES)
			{
				//possibly a link, check it fully

				string Dest = GetShortCutTarget(w2);

				if(!Dest.empty())
				{
					if(PathIsRelative(Dest.c_str()))
					{
						w32path += '\\';
						w32path += Dest;
					}
					else
					{
						w32path = Dest;
					}
					continue;
				}
			}
		}

		//just a normal element
		w32path += '\\';
		w32path += element;
	}

	return w32path;
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


//
// If lpszLinkFile is a shortcut then return the target it points to
// otherwise return the original path
//
string Path::GetShortCutTarget(string& path) 
{ 
    HRESULT hres; 
    IShellLinkA* psl = 0; 
    IPersistFile* ppf = 0;
    WIN32_FIND_DATA wfd; 
	string Dest;
    wchar_t * pWsz = new wchar_t[MAX_PATH+1];
    char * pSz = new char[MAX_PATH+1];

    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IShellLinkA, (LPVOID*)&psl); 
    if(FAILED(hres))  {
		ktrace("cocreate failed hr 0x%08lx\n", hres);
		goto cleanup;
	}

 
    // Get a pointer to the IPersistFile interface. 
    hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
    if(FAILED(hres))  {
		ktrace("query interface failed hr 0x%08lx\n", hres);
		goto cleanup;
	}
	
    // Need a wide version of the filename
	memset(pWsz,0,sizeof(pWsz[0])*MAX_PATH);
    if(MultiByteToWideChar(CP_ACP, 0, path.c_str(), path.length(), pWsz, MAX_PATH) == 0) {
		hres=E_FAIL;
		ktrace("MultiByteToWideChar failed hr 0x%08lx\n", hres);
		goto cleanup;
	}


    // Load the shortcut. 
    hres = ppf->Load(pWsz, STGM_READ); 
    if(FAILED(hres))  {
		ktrace("load failed hr 0x%08lx\n", hres);
		goto cleanup;
	}


	// Resolve the link
	//lots of debug code when getting this to run on W2k
    hres = psl->Resolve(NULL, SLR_NO_UI|SLR_NOLINKINFO|SLR_NOUPDATE|SLR_NOSEARCH|SLR_NOTRACK);
    if(FAILED(hres)) {
		ktrace("resolve failed hr 0x%08lx, retrying with different flags\n", hres);
		//retry without some flags
	    hres = psl->Resolve(NULL, SLR_NO_UI|SLR_ANY_MATCH);
		if(FAILED(hres)) {
			ktrace("resolve failed hr 0x%08lx\n", hres);
			//ignore error, use whatever path was already there
		}
	}

    // Get the path to the link target.
	pSz[0] = 0;
    hres = psl->GetPath(pSz, MAX_PATH,
                        (WIN32_FIND_DATA*)&wfd, 
                        SLGP_RAWPATH);
	if(hres==NOERROR && path[0]!=0)
    { 
        //success! lpszPath populated
    }
	else
	{
		//cygwin sym-link use desc as well as target, so use desc if target not available
		ktrace("read link path failed 0x%08lx, falling back to description\n");
		psl->GetDescription(pSz, MAX_PATH);
		if(FAILED(hres))  {
			ktrace("get desc failed hr 0x%08lx\n", hres);
			goto cleanup;
		}
	}
	Dest = pSz;


cleanup:
	if(ppf)
		ppf->Release();
	if(psl)
		psl->Release();
    delete pWsz;
    delete pSz;

//    if(TempPath[0] == 0) {
//		ktrace("GetShortCutTarget failed to get a path\n");
//		hres=E_FAIL;
//	}

	if(FAILED(hres))
		ktrace("GetShortCutTarget failed, hr=0x%08lx\n", hres);

    return Dest; 
}



// CreateLink - uses the Shell's IShellLink and IPersistFile interfaces 
//              to create and store a shortcut to the specified object. 
//
// Returns the result of calling the member functions of the interfaces. 
//

HRESULT Path::CreateLink(const string& LinkPath, const string& DestPath, const string& Description)
{ 
    HRESULT hres; 
    IShellLink* psl; 
 
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IShellLink, (LPVOID*)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target and add the description. 
        psl->SetPath(DestPath); 
        psl->SetDescription(Description); 
 
        // Query IShellLink for the IPersistFile interface for saving the 
        // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres)) 
        { 
            string wsz;
 
            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, LinkPath.c_str(), -1, (wchar_t*)wsz.GetBuffer(MAX_PATH), MAX_PATH); 

            // TODO: Check return value from MultiByteWideChar to ensure success.
 
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save((wchar_t*)wsz.c_str(), TRUE); 

			wsz.ReleaseBuffer();			

            ppf->Release(); 
        } 
        psl->Release(); 
    } 
    return hres; 
}
