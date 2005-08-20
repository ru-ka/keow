// File.cpp: implementation of the File class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "File.h"

//////////////////////////////////////////////////////////////////////

File::File()
{
	m_Handle = INVALID_HANDLE_VALUE;
}

File::~File()
{
	CloseHandle(m_Handle);
}


bool File::Write(LPVOID buffer, DWORD count, DWORD& written)
{
	return WriteFile(m_Handle, buffer, count, &written, NULL) != 0;
}

bool File::Read(LPVOID buffer, DWORD count, DWORD& read)
{
	return ReadFile(m_Handle, buffer, count, &read, NULL) != 0;
}
