//Simple String implementation (like STL) (see comment in includes.h)
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRING_H__4305518F_D005_41B7_8993_8F7E70C9A0AC__INCLUDED_)
#define AFX_STRING_H__4305518F_D005_41B7_8993_8F7E70C9A0AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "String.h"


class string
{
public:
	string();
	string(const string& str);
	string(const char *str);
	~string();

	int length() const;
	int capacity() const;
	bool empty() const;
	void reserve(int size);

	string operator + (string str);
	string& operator += (char c);
	string& operator += (const char * str);
	string& operator += (const string& str);
	string& operator = (const char * str);
	string& operator = (const string& str);

	char operator [] (int pos);

	bool operator == (const char * str) const;
	bool operator == (const string& str) const;
	bool operator != (const char * str) const;
	bool operator != (const string& str) const;
	int compare(const char * str) const;
	int compare(const string& str) const;

	const char * c_str() const;
	int find(char c, int pos=0) const;
	string substr(int pos, int len) const;
	string substr(int pos) const;

protected:
	char * m_pChars;
	int m_nLen, m_nCapacity;

	void null_terminate();
};

/////////////////////////////////////////////////////////////////

#endif // !defined(AFX_STRING_H__4305518F_D005_41B7_8993_8F7E70C9A0AC__INCLUDED_)
