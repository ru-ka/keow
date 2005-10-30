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

// LegacyWindows.h: interface for the LegacyWindows class.
//  This is windows functions that need emulating in Win95 etc
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LEGACYWINDOWS_H__E7DD2E50_8394_481B_95DE_6B192387DA06__INCLUDED_)
#define AFX_LEGACYWINDOWS_H__E7DD2E50_8394_481B_95DE_6B192387DA06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class LegacyWindows  
{
public:
	static BOOL IsProcessorFeaturePresent(DWORD dwFeature);

	static BOOL GetFileAttributesEx(LPCTSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);

	static LPVOID VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	static BOOL VirtualFreeEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
	static DWORD VirtualQueryEx(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength);

	static BOOL CreateHardLink(LPCSTR lpNewFile, LPCSTR lpOldFile);
};

#endif // !defined(AFX_LEGACYWINDOWS_H__E7DD2E50_8394_481B_95DE_6B192387DA06__INCLUDED_)
