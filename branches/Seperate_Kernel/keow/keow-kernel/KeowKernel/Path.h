// Path.h: interface for the Path class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PATH_H__318FAE4B_1C88_435D_A35C_D1849ACD47AA__INCLUDED_)
#define AFX_PATH_H__318FAE4B_1C88_435D_A35C_D1849ACD47AA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class MountPoint;

class Path  
{
public:
	Path(bool FollowSymLinks = true);
	Path(string UnixPath, bool FollowSymLinks = true);
	Path(LPCSTR UnixPath, bool FollowSymLinks = true);
	virtual ~Path();

	Path& operator=(const Path& other);

	void SetUnixPath(string path);
	void AppendUnixPath(string unixp);

	bool EqualsPartialPath(const Path& other, int ElementCount) const;

	void FollowSymLinks(bool follow);

	bool IsSymbolicLink();
	int GetUnixFileType();
	DWORD GetWin32FileAttributes();

	string GetUnixPath();
	string GetWin32Path();

	static string GetShortCutTarget(string& path);
	static HRESULT CreateLink(const string& LinkPath, const string& DestPath, const string& Description);

protected:
	typedef list<const string> ElementList;

	static string JoinList(Path::ElementList& list, char delimiter);

protected:
	ElementList m_PathStack;

	bool m_FollowSymLinks;
};

#endif // !defined(AFX_PATH_H__318FAE4B_1C88_435D_A35C_D1849ACD47AA__INCLUDED_)
