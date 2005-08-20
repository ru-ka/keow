// ConstantMapping.cpp: implementation of the ConstantMapping class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "ConstantMapping.h"

//////////////////////////////////////////////////////////////////////

DWORD ConstantMapping::ElfProtectionToWin32Protection(linux::Elf32_Word prot)
{
	DWORD win32prot;
	win32prot = 0;
	if(prot == (PF_R) )
		win32prot = PAGE_READONLY;
	else
	if(prot == (PF_W)
	|| prot == (PF_W|PF_R) )
		win32prot = PAGE_READWRITE;
	else
	if(prot == (PF_X) )
		//NOT honoring this competely, should be PAGE_EXECUTE but this is better for debug
		win32prot = PAGE_EXECUTE_READ; 
	else
	if(prot == (PF_X|PF_R) )
		win32prot = PAGE_EXECUTE_READ;
	else
	if(prot == (PF_W|PF_X)
	|| prot == (PF_W|PF_X|PF_R) )
		win32prot = PAGE_EXECUTE_READWRITE;
	else
	{
		win32prot = PAGE_EXECUTE_READWRITE;
		ktrace("unhandled protection 0x%d, loading page R+W+X\n");
	}
	return win32prot;
}


/*
 * translate win32 gtlasterror value to linux errno
 */
int ConstantMapping::Win32ErrToUnixError(DWORD err)
{
	switch(err)
	{
	case ERROR_SUCCESS:
		return 0;

	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return ENOENT;

	case ERROR_TOO_MANY_OPEN_FILES:
		return EMFILE;

	case ERROR_ACCESS_DENIED:
		return EACCES;

	case ERROR_INVALID_HANDLE:
		return EBADF;

	case ERROR_ARENA_TRASHED:
	case ERROR_INVALID_BLOCK:
		return EFAULT;

	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return ENOMEM;

	case ERROR_INVALID_FUNCTION:
		return ENOSYS;

	case ERROR_BROKEN_PIPE:
		return EIO;

	case ERROR_BAD_FORMAT:
		return ENOEXEC;

	default:
		ktrace("Unhandled Win32 Error code %ld\n", err);
		return EPERM; //generic
	}
}
