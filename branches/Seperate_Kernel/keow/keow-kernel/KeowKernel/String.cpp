//Simple String implementation (like STL) (see comment in includes.h)

#include "includes.h"
#include "String.h"

/////////////////////////////////////////////////////////////////

string::string()
{
	m_pChars=0;
	m_nLen=m_nCapacity=0;
}
string::string(const string& str)
{
	m_pChars=0;
	m_nLen=m_nCapacity=0;
	*this = str;
}
string::string(const char *str)
{
	m_pChars=0;
	m_nLen=m_nCapacity=0;
	*this = str;
}

string::~string()
{
	if(m_pChars)
		delete m_pChars;
}

void string::null_terminate()
{
	reserve(m_nLen+1);
	m_pChars[m_nLen] = 0;
}

bool string::empty() const
{
	return m_pChars==0 || m_nLen==0;
}
int string::length() const
{
	return m_nLen;
}
int string::capacity() const
{
	return m_nCapacity;
}
void string::reserve(int size)
{
	if(m_nCapacity >= size)
		return;

	while(m_nCapacity < size)
		m_nCapacity += 128; //what to do here? try something!

	char * p2 = new char[m_nCapacity];
	if(m_pChars)
	{
		memcpy(p2,m_pChars, m_nLen);
		delete m_pChars;
	}
	m_pChars = p2;
}

string string::operator+(string str)
{
	string s2(*this);
	return s2+=str;
}

string& string::operator+=(char c)
{
	reserve(m_nLen+2);
	m_pChars[m_nLen] = c;
	m_nLen++;
	null_terminate();
	return *this;
}
string& string::operator += (const char * str)
{
	int len = strlen(str);
	reserve(m_nLen+len);
	memcpy(&m_pChars[m_nLen], str, len);
	m_nLen+=len;
	null_terminate();
	return *this;
}
string& string::operator += (const string& str)
{
	if(str.empty())
		return *this;
	reserve(m_nLen+str.m_nLen);
	memcpy(&m_pChars[m_nLen], str.m_pChars, str.m_nLen);
	m_nLen+=str.m_nLen;
	null_terminate();
	return *this;
}

const char * string::c_str() const
{
	return m_pChars;
}

int string::find(char c, int pos) const
{
	if(empty())
		return -1;

	for(int n=pos; n<m_nLen; ++n)
	{
		if(m_pChars[n]==c)
			return n;
	}
	return -1; //not found
}

string& string::operator = (const char * str)
{
	int len = strlen(str);
	reserve(len);
	memcpy(m_pChars, str, len);
	m_nLen = len;
	null_terminate();
	return *this;
}
string& string::operator = (const string& str)
{
	reserve(str.m_nLen);
	if(str.m_pChars)
		memcpy(m_pChars, str.m_pChars, str.m_nLen);
	m_nLen = str.m_nLen;
	null_terminate();
	return *this;
}

string string::substr(int pos, int len) const
{
	string s;

	if(pos<m_nLen)
	{
		if(pos+len > m_nLen)
			len = m_nLen - pos;
		s.reserve(len);
		memcpy(s.m_pChars, &m_pChars[pos], len);
		s.m_nLen = len;
		s.null_terminate();
	}

	return s;
}
string string::substr(int pos) const
{
	return substr(pos,m_nLen);
}

bool string::operator == (const char * str) const
{
	return compare(str)==0;
}
bool string::operator == (const string& str) const
{
	return compare(str)==0;
}
bool string::operator != (const char * str) const
{
	return compare(str)!=0;
}
bool string::operator != (const string& str) const
{
	return compare(str)!=0;
}

int string::compare(const char * str) const
{
	return strcmp(c_str(), str);
}
int string::compare(const string& str) const
{
	return strcmp(c_str(), str.c_str());
}

char string::operator [] (int pos)
{
	if(pos>=m_nLen)
		return 0;
	return m_pChars[pos];
}