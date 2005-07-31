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
#include "utils.h"


/*
 * translate win32 gtlasterror value to linux errno
 */
int Win32ErrToUnixError(DWORD err)
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

	default:
		ktrace("Unhandled Win32 Error code %ld\n", err);
		return EPERM; //generic
	}
}

