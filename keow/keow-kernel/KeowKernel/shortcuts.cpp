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

#include <windows.h>
#include <Shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>

extern void _cdecl ktrace(const char * format, ...);

//
// If lpszLinkFile is a shortcut then return the target it points to
// otherwise return the original path
//
HRESULT GetShortCutTarget(LPCSTR lpszLinkFile, LPSTR lpszPath) 
{ 
    HRESULT hres; 
    IShellLinkA* psl = 0; 
    IPersistFile* ppf = 0;
    WIN32_FIND_DATA wfd; 
    WCHAR wsz[MAX_PATH+1]; 

    *lpszPath = 0; // assume failure 
 
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
    if(MultiByteToWideChar(CP_ACP, 0, lpszLinkFile, -1, wsz, MAX_PATH) == 0) {
		hres=E_FAIL;
		ktrace("MultiByteToWideChar failed hr 0x%08lx\n", hres);
		goto cleanup;
	}


    // Load the shortcut. 
    hres = ppf->Load(wsz, STGM_READ); 
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
    hres = psl->GetPath(lpszPath, 
                        MAX_PATH, 
                        (WIN32_FIND_DATA*)&wfd, 
                        SLGP_RAWPATH);
	if(hres==NOERROR && *lpszPath!=0)
    { 
        //success! lpszPath populated
    }
	else
	{
		//cygwin sym-link use desc as well as target, so use desc if target not available
		ktrace("read link path failed 0x%08lx, falling back to description\n");
		psl->GetDescription(lpszPath, MAX_PATH);
		if(FAILED(hres))  {
			ktrace("get desc failed hr 0x%08lx\n", hres);
			goto cleanup;
		}
	}


cleanup:
	if(ppf)
		ppf->Release();
	if(psl)
		psl->Release();

    if(*lpszPath == 0) {
		ktrace("GetShortCutTarget failed to get a path\n");
		hres=E_FAIL;
	}

	if(FAILED(hres))
		ktrace("GetShortCutTarget failed, hr=0x%08lx\n", hres);

    return hres; 
}



// CreateLink - uses the Shell's IShellLink and IPersistFile interfaces 
//              to create and store a shortcut to the specified object. 
//
// Returns the result of calling the member functions of the interfaces. 
//
// Parameters:
// lpszPathObj  - address of a buffer containing the path of the object. 
// lpszPathLink - address of a buffer containing the path where the 
//                Shell link is to be stored. 
// lpszDesc     - address of a buffer containing the description of the 
//                Shell link. 

HRESULT CreateLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc) 
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
        psl->SetPath(lpszPathObj); 
        psl->SetDescription(lpszDesc); 
 
        // Query IShellLink for the IPersistFile interface for saving the 
        // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres)) 
        { 
            WCHAR wsz[MAX_PATH]; 
 
            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH); 
			
            // TODO: Check return value from MultiByteWideChar to ensure success.
 
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save(wsz, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 
    } 
    return hres; 
}
