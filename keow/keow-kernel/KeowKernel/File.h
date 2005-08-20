// File.h: interface for the File class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__5FA578F4_1C37_466C_BED7_3CE1E76298CB__INCLUDED_)
#define AFX_FILE_H__5FA578F4_1C37_466C_BED7_3CE1E76298CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class File  
{
public:
	File();
	virtual ~File();

	bool Write(void* buffer, DWORD count, DWORD& written);
	bool Read(void* buffer, DWORD count, DWORD& read);

	HANDLE m_Handle;
};

#endif // !defined(AFX_FILE_H__5FA578F4_1C37_466C_BED7_3CE1E76298CB__INCLUDED_)
