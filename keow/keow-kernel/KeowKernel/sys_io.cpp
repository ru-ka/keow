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

#include "includes.h"
#include "SysCalls.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 * write bytes to a handle
 * sys_write(handle,text,len)
 */
void SysCalls::sys_write(CONTEXT &ctx)
{
	int fd;
	IOHandler * ioh;
	
	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ctx.Eax = SysCallDll::write(ioh->GetRemoteWriteHandle(), (void*)ctx.Ecx, ctx.Edx);
	if(ctx.Eax==0)
		ctx.Eax = Win32ErrToUnixError(SysCallDll::GetLastError());
}

/*
 * int writev(int filedes, struct iovec* v, int count)
 * this is a write from several buffers
 */
void SysCalls::sys_writev(CONTEXT &ctx)
{
	linux::iovec *pV = (linux::iovec *)ctx.Ecx;
	int count = ctx.Edx;
	int fd;
	IOHandler * ioh;
	
	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ctx.Eax = SysCallDll::writev(ioh->GetRemoteWriteHandle(), pV, count);
	if(ctx.Eax==0)
		ctx.Eax = Win32ErrToUnixError(SysCallDll::GetLastError());
}

/*****************************************************************************/

/*
 * unsigned long sys_read(int fd, char*buf, unsigned long size)
 */
void SysCalls::sys_read(CONTEXT &ctx)
{
	int fd;
	IOHandler * ioh;

	ktrace("read(fd %d, len %d into 0x%p)\n", ctx.Ebx, ctx.Edx, ctx.Ecx);

	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ctx.Eax = SysCallDll::read(ioh->GetRemoteReadHandle(), (void*)ctx.Ecx, ctx.Edx);
	if(ctx.Eax==0)
		ctx.Eax = Win32ErrToUnixError(SysCallDll::GetLastError());
}

/*****************************************************************************/

/*
 * access(filename, perms)
 */
void SysCalls::sys_access(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	Path p;
	DWORD attr;
	char ok = 1;
	DWORD check = ctx.Ecx;

	p.SetUnixPath((const char*)ctx.Ebx);

	attr = p.GetFileAttributes();
	if(attr==INVALID_FILE_ATTRIBUTES)
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	//read is ok if access already succeeded

	if( (check & W_OK) && (attr&FILE_ATTRIBUTE_READONLY) )
		ok = 0;

	if(ok)
		ctx.Eax = 0;
	else
		ctx.Eax = -EACCES;
#endif
}

/*****************************************************************************/


/*
 * int open(path, access, perms)
 * open or create a file
 */
void SysCalls::sys_open(CONTEXT &ctx)
{
	Path p;
	DWORD access = ctx.Ecx;
	DWORD perms = ctx.Edx;
	int fd;
	DWORD win32access, win32share, disposition, flags;
	IOHandler * ioh;

	string s = MemoryHelper::ReadString(P->m_Win32PInfo.hProcess, (ADDR)ctx.Ebx);
	ktrace("open(%s, 0x%lx, 0%lo)\n", s.c_str(), access, perms);
	p.SetUnixPath(s);

	//find free handle entry
	fd = P->FindFreeFD();
	if(fd==-1)
	{
		ctx.Eax = -EMFILE; //too many open files
		return;
	}

	//calc flags
	if((access&O_ACCMODE) == O_WRONLY)
		win32access = GENERIC_WRITE;
	else
		if((access&O_ACCMODE) == O_RDONLY)
			win32access = GENERIC_READ;
		else
			win32access = GENERIC_READ|GENERIC_WRITE;
	if(access & O_EXCL)
		win32share = 0;
	else
		win32share = FILE_SHARE_READ|FILE_SHARE_WRITE;
	disposition = OPEN_EXISTING;
	if(access & O_CREAT)
		disposition = OPEN_ALWAYS;
	flags = 0;


	//open
	ioh = IOHandler::CreateForPath(p);
	if(ioh==NULL)
	{
		ctx.Eax = -ENXIO; //no device available?
		return;
	}

	bool ok = ioh->Open(win32access, win32share, disposition, flags);
	if(!ok)
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
		delete ioh;
		return;
	}


	P->m_OpenFiles[fd] = ioh;
	ctx.Eax = fd;

	//more flags
	if(access & O_TRUNC) 
	{
		SysCallDll::SetFilePointer(ioh->GetRemoteWriteHandle(), 0,0, FILE_BEGIN);
		SysCallDll::SetEndOfFile(ioh->GetRemoteWriteHandle());
	}
	if(access & O_APPEND)
	{
		SysCallDll::SetFilePointer(ioh->GetRemoteWriteHandle(), 0,0, FILE_END);
	}

}


/*****************************************************************************/


/*
 * int close(handle)
 */
void SysCalls::sys_close(CONTEXT &ctx)
{
	int fd;
	IOHandler * ioh;
	
	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	//free (does the close too)
	delete ioh;
	P->m_OpenFiles[fd] = NULL;

	ctx.Eax = 0;
}


/*****************************************************************************/


/*
 * int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
 */
void SysCalls::sys_select(CONTEXT &ctx)
{
	sys__newselect(ctx);
}


/*****************************************************************************/


/*
 * int ioctl(int fd, int request, ...)
 */
void SysCalls::sys_ioctl(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	int fd;
	IOHandler * ioh;
	
	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = pProcessData->FileHandlers[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ctx.Eax = ioh->ioctl( ctx.Ecx, ctx.Edx );
#endif
}


/*****************************************************************************/


/*
 * int getcwd(char* buf, int size)
 */
void SysCalls::sys_getcwd(CONTEXT &ctx)
{
	int cnt = (int)ctx.Ecx;
	string cwd = P->m_UnixPwd.GetUnixPath();
	if(cnt < 1+cwd.length())
	{
		cnt = cwd.length();
		cwd[cnt-1] = 0;
	}
	P->WriteMemory((ADDR)ctx.Ebx, cnt, (char*)cwd.c_str());
	ctx.Eax = 0;
}



/*****************************************************************************/


/*
 * int chdir(const char* path)
 */
void SysCalls::sys_chdir(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	Path p;
	p.SetUnixPath((const char*)ctx.Ebx);

	if(!SetCurrentDirectory(p.Win32Path()))
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	ktrace("chdir %s\n", p.UnixPath());

	StringCbCopy(pProcessData->unix_pwd, sizeof(pProcessData->unix_pwd), p.UnixPath());
	ctx.Eax = 0;
#endif
}

/*
 * int fchdir(int fd)
 */
void SysCalls::sys_fchdir(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	int fd;
	IOHandler * ioh;

	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = pProcessData->FileHandlers[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	//delegate
	ctx.Ebx = (DWORD)ioh->GetUnixPath();
	sys_chdir(pCtx);

	//restore overwritten data
	ctx.Ebx = fd; 
#endif
}


/*****************************************************************************/


/*
 * int fcntl(int fd, int cmd, ...)
 */
void SysCalls::sys_fcntl(CONTEXT &ctx)
{
	int fd;
	IOHandler * ioh;
	
	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ctx.Eax = -EINVAL;

	switch(ctx.Ecx)
	{
	case F_GETFD:	/* get close_on_exec */
		ctx.Eax = ioh->GetInheritable() ? 0 : 1;
		break;

	case F_SETFD:   /* set/clear close_on_exec */
		ioh->SetInheritable(ctx.Edx==0);
		ctx.Eax = 0;
		break;

	case F_GETFL:	/* get file->f_flags */
		ctx.Eax = ioh->GetFlags();
		break;

	case F_SETFL:	/* set file->f_flags */
		ioh->SetFlags(ctx.Edx);
		ctx.Eax = 0;
		break;

	case F_DUPFD:	/* like dup2() but use any fd<=arg and close-on-exec flag of copy is off */
		{
			int maxfd = ctx.Edx;
			int fdnew;
			DWORD OldEcx;

			//find free handle entry
			fdnew = P->FindFreeFD();
			if(fdnew==-1)
			{
				ctx.Eax = -EMFILE; //too many open files
				return;
			}
			if(fdnew > maxfd)
			{
				ctx.Eax = EINVAL;
				return;
			}

			//use dup2 for actual work
			OldEcx = ctx.Ecx;
			ctx.Ecx = fdnew;
			sys_dup2(ctx);
			ctx.Ecx = OldEcx;
			P->m_OpenFiles[fdnew]->SetInheritable(false);
			//eax is already set with the result
		}
		break;

	default:
		ktrace("IMPLEMENT sys_fcntl 0x%lx\n", ctx.Ecx);
		ctx.Eax = -ENOSYS;
		break;
	}
}


/*****************************************************************************/


/*
 * int dup(int fd)
 */
void SysCalls::sys_dup(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	int fdnew;
	DWORD OldEcx;

	//find free handle entry
	fdnew = FindFreeFD();
	if(fdnew==-1)
	{
		ctx.Eax = -EMFILE; //too many open files
		return;
	}

	//use dup2 for actual work
	OldEcx = ctx.Ecx;
	ctx.Ecx = fdnew;
	sys_dup2(pCtx);
	ctx.Ecx = OldEcx;
#endif
}


/*****************************************************************************/


/*
 * int dup2(int fd, int newfd)
 */
void SysCalls::sys_dup2(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	int fdold, fdnew;
	IOHandler *ioh_old;

	//validate old handle
	fdold = ctx.Ebx;
	if(fdold<0 || fdold>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh_old = pProcessData->FileHandlers[fdold];
	if(ioh_old == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	//validate new handle
	fdnew = ctx.Ecx;
	if(fdnew<0 || fdnew>MAX_OPEN_FILES || fdnew==fdold)
	{
		ctx.Eax = -EBADF;
		return;
	}

	//close any existing
	if(pProcessData->FileHandlers[fdnew] != NULL)
	{
		pProcessData->FileHandlers[fdnew]->Close();
		delete pProcessData->FileHandlers[fdnew];
		pProcessData->FileHandlers[fdnew] = NULL;
	}
	//make duplicate
	IOHandler *ioh_new = ioh_old->Duplicate(GetCurrentProcess(), GetCurrentProcess());
	if(ioh_new==NULL)
	{
		ctx.Eax = Win32ErrToUnixError(GetLastError());
		return;
	}
	pProcessData->FileHandlers[fdnew] = ioh_new;
	ctx.Eax = fdnew;
#endif
}


/*****************************************************************************/


/*
 * int getdents64(int fd, dirent64* d, int bytes)
 */
void SysCalls::sys_getdents64(CONTEXT &ctx)
{
	linux::dirent64 * s = (linux::dirent64 *)ctx.Ecx;
	int maxbytes = ctx.Edx;
	int filled = 0;
	int fd;
	IOHandler * ioh;

	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ktrace("getdents64(fd %d, %p, max %d)\n", fd, s, maxbytes);

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	//read locally, copy to process
	LPBYTE buf = new BYTE[maxbytes];
	memset(buf,0,maxbytes);
	//P->ReadMemory(buf, (ADDR)s, maxbytes);

	filled = ioh->GetDirEnts64((linux::dirent64 *)buf, maxbytes);
	if(filled >= 0)
	{
		if(filled>0)
			P->WriteMemory((ADDR)s, filled, buf);
		ctx.Eax = filled;
	}
	else
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
	}
	
	delete buf;
}


/*****************************************************************************/


/*
 * int stat(const char * name,  struct stat* buf)
 */
void SysCalls::sys_stat(CONTEXT &ctx)
{
	string fname = MemoryHelper::ReadString(P->m_Win32PInfo.hProcess, (ADDR)ctx.Ebx);
	linux::stat * pBuf = (linux::stat*)ctx.Ecx;
	linux::stat buf;

	//DO resolve symbolic links for stat
	Path p(true);
	p.SetUnixPath(fname);

	IOHandler * ioh = IOHandler::CreateForPath(p);
	if(ioh==NULL)
	{
		ctx.Eax = -ENOMEM;
		return;
	}

	if(ioh->Stat(&buf))
	{
		P->WriteMemory((ADDR)pBuf, sizeof(buf), &buf);
		ctx.Eax = 0;
	}
	else
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int lstat(const char * name,  struct stat* buf)
 */
void SysCalls::sys_lstat(CONTEXT &ctx)
{
	string fname = MemoryHelper::ReadString(P->m_Win32PInfo.hProcess, (ADDR)ctx.Ebx);
	linux::stat * pBuf = (linux::stat*)ctx.Ecx;
	linux::stat buf;

	//don't resolve symbolic links for lstat
	Path p(false);
	p.SetUnixPath(fname);

	IOHandler * ioh = IOHandler::CreateForPath(p);
	if(ioh==NULL)
	{
		ctx.Eax = -ENOMEM;
		return;
	}

	if(ioh->Stat(&buf))
	{
		P->WriteMemory((ADDR)pBuf, sizeof(buf), &buf);
		ctx.Eax = 0;
	}
	else
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int fstat(int fd,  struct stat* buf)
 */
void SysCalls::sys_fstat(CONTEXT &ctx)
{
	linux::stat * pBuf = (linux::stat*)ctx.Ecx;
	linux::stat buf;
	int fd;
	IOHandler * ioh;

	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	if(ioh->Stat(&buf))
	{
		P->WriteMemory((ADDR)pBuf, sizeof(buf), &buf);
		ctx.Eax = 0;
	}
	else
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
	}
}


/*****************************************************************************/


/*
 * int stat64(const char * name,  struct stat64* buf)
 */
void SysCalls::sys_stat64(CONTEXT &ctx)
{
	string fname = MemoryHelper::ReadString(P->m_Win32PInfo.hProcess, (ADDR)ctx.Ebx);
	linux::stat64 * pBuf = (linux::stat64*)ctx.Ecx;
	linux::stat64 buf;

	//DO resolve symbolic links for stat
	Path p(true);
	p.SetUnixPath(fname);

	IOHandler * ioh = IOHandler::CreateForPath(p);
	if(ioh==NULL)
	{
		ctx.Eax = -ENOMEM;
		return;
	}

	if(ioh->Stat64(&buf))
	{
		P->WriteMemory((ADDR)pBuf, sizeof(buf), &buf);
		ctx.Eax = 0;
	}
	else
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int lstat64(const char * name,  struct stat64* buf)
 */
void SysCalls::sys_lstat64(CONTEXT &ctx)
{
	string fname = MemoryHelper::ReadString(P->m_Win32PInfo.hProcess, (ADDR)ctx.Ebx);
	linux::stat64 * pBuf = (linux::stat64*)ctx.Ecx;
	linux::stat64 buf;

	//don't resolve symbolic links for lstat
	Path p(false);
	p.SetUnixPath(fname);

	IOHandler * ioh = IOHandler::CreateForPath(p);
	if(ioh==NULL)
	{
		ctx.Eax = -ENOMEM;
		return;
	}

	if(ioh->Stat64(&buf))
	{
		P->WriteMemory((ADDR)pBuf, sizeof(buf), &buf);
		ctx.Eax = 0;
	}
	else
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int fstat64(int fd,  struct stat64* buf)
 */
void SysCalls::sys_fstat64(CONTEXT &ctx)
{
	linux::stat64 * pBuf = (linux::stat64*)ctx.Ecx;
	linux::stat64 buf;
	int fd;
	IOHandler * ioh;

	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = P->m_OpenFiles[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	if(ioh->Stat64(&buf))
	{
		P->WriteMemory((ADDR)pBuf, sizeof(buf), &buf);
		ctx.Eax = 0;
	}
	else
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
	}
}


/*****************************************************************************/

/*
 * int readlink(path, buf, size)
 */
void SysCalls::sys_readlink(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	int c;

	//don't follow symlinks
	Path p;
	p.FollowSymLinks(false);
	p.SetUnixPath((const char*)ctx.Ebx);

	if(p.GetFileAttributes()==INVALID_FILE_ATTRIBUTES)
	{
		ctx.Eax = -ENOENT; //no file
	}
	else
	if(!p.IsSymbolicLink())
	{
		ctx.Eax = -EINVAL; //not a link
	}
	else
	{
		//want to follow it now
		p.FollowSymLinks(true);

		//readlink does not copy the null!
		char *pD = (char*)ctx.Ecx;
		const char *pS = p.UnixPath();
		int max = (int)ctx.Edx;
		for(c=0; *pS!=0 && c<max; ++c,++pD,++pS)
		{
			*pD = *pS;
		}

		ctx.Eax = c; //ok
	}
#endif
}


/*****************************************************************************/

/*
 * int pipe(int filesdes[2])
 */
void SysCalls::sys_pipe(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	int *fds = (int*)ctx.Ebx;

	//handles to open
	HANDLE hRead, hWrite;

	if(!CreatePipe(&hRead, &hWrite, NULL, 0))
	{
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	//make then inheritable
	HANDLE h;
	h=hRead;
	DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &hRead, 0, TRUE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
	h=hWrite;
	DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &hWrite, 0, TRUE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);

	
	//return fd entries
	fds[0] = FindFreeFD();
	pProcessData->FileHandlers[fds[0]] = new PipeIOHandler(hRead);
	fds[1] = FindFreeFD();
	pProcessData->FileHandlers[fds[1]] = new PipeIOHandler(hWrite);

	ctx.Eax = 0;
#endif
}


/*****************************************************************************/

/*
 * int link(olfpath, newpath)
 */
void SysCalls::sys_link(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	Path OldP(false), NewP(false);

	OldP.SetUnixPath((const char*)ctx.Ebx);
	NewP.SetUnixPath((const char*)ctx.Ecx);

	if(CreateHardLink(NewP.Win32Path(), OldP.Win32Path(), NULL))
		ctx.Eax = 0;
	else
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
#endif
}


/*
 * int unlink(path)
 */
void SysCalls::sys_unlink(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	Path p(false);
	p.SetUnixPath((const char *)ctx.Ebx);

	if(DeleteFile(p.Win32Path()))
	{
		ctx.Eax = 0;
		return;
	}

	DWORD err = GetLastError();
	ctx.Eax = -Win32ErrToUnixError(err);

	if(err!=ERROR_SHARING_VIOLATION)
		return; //return this error


	//file is still being used. In unix you can delete such a file
	//but not in windows. However in windows it can often be successfully
	//renamed within the same directory, so try to do this with a temporary
	//name and retry later
	/*
	for(int i=0; i<10000; ++i) //if 10000 files have been deleted we will fail - on well :-)
	{
		char p2[MAX_PATH];

	}
	*/
	return; //return the original error
#endif
}


/*****************************************************************************/

/*
 * int _llseek(int fd, ulong offset_high, ulong offset_low, loff_t* result, uint whence)
 */
void SysCalls::sys__llseek(CONTEXT &ctx)	
{
	Unhandled(ctx);
#if 0
	int fd = ctx.Ebx;
	LONG offset_high = ctx.Ecx;
	LONG offset_low = ctx.Edx;
	linux::loff_t* result = (linux::loff_t*)ctx.Esi;
	DWORD whence = ctx.Edi;

	IOHandler * ioh;

	fd = ctx.Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		ctx.Eax = -EBADF;
		return;
	}

	ioh = pProcessData->FileHandlers[fd];
	if(ioh == NULL)
	{
		ctx.Eax = -EBADF;
		return;
	}

	DWORD method;
	if(whence==SEEK_SET)
		method=FILE_BEGIN;
	else
	if(whence==SEEK_END)
		method=FILE_END;
	else
	if(whence==SEEK_CUR)
		method=FILE_CURRENT;
	else
	{
		ctx.Eax = -EINVAL;
		return;
	}


	SetLastError(0);

	ULARGE_INTEGER li;
	li.LowPart = offset_low;
	li.HighPart = offset_high;
	*result = ioh->Seek(li.QuadPart, method);

	if(GetLastError()==0)
		ctx.Eax = 0;
	else
		ctx.Eax = -Win32ErrToUnixError(GetLastError());
#endif
}


/*
 * off_t lseek(int fildes, off_t offset, int whence)
 */
void SysCalls::sys_lseek(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	//use llseek
	CONTEXT ctx2;
	linux::loff_t result;

	ctx2.Ebx = ctx.Ebx; //fd
	ctx2.Ecx = 0; //offset, high
	ctx2.Edx = ctx.Ecx; //offset, low
	ctx2.Esi = (DWORD)(&result);
	ctx2.Edi = ctx.Edx; //whence

	sys__llseek(&ctx2);
	if(ctx2.Eax!=0)
	{
		//error
		ctx.Eax = ctx2.Eax;
		return;
	}

	ctx.Eax = (DWORD)(result & 0x7FFFFFFF);
#endif
}


/*****************************************************************************/

/*
 * int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
 */
void SysCalls::sys__newselect(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	int numFds = ctx.Ebx;
	linux::fd_set *pReadFds = (linux::fd_set*)ctx.Ecx;
	linux::fd_set *pWriteFds = (linux::fd_set*)ctx.Edx;
	linux::fd_set *pExceptFds = (linux::fd_set*)ctx.Esi;
	linux::timeval *pTimeout = (linux::timeval*)ctx.Edi;

	linux::fd_set ReadResults, WriteResults, ExceptResults;
	LINUX_FD_ZERO(&ReadResults);
	LINUX_FD_ZERO(&WriteResults);
	LINUX_FD_ZERO(&ExceptResults);


	DWORD dwWait;
	if(pTimeout==0) {
		dwWait = INFINITE;
		ktrace("new_select wait forever\n");
	}
	else {
		dwWait = pTimeout->tv_sec*1000L + pTimeout->tv_usec/1000; //correct?
		ktrace("new_select wait (%ld,%ld) = %ldms\n", pTimeout->tv_sec, pTimeout->tv_usec, dwWait);
	}

	DWORD dwEnd = GetTickCount() + dwWait; //wrap around will cause a quick return - oops :-)
	bool foundData = false;
	while(dwEnd > GetTickCount())
	{
		for(int fd=0; fd<numFds; ++fd)
		{
			if( pReadFds
			&&  LINUX_FD_ISSET(fd, pReadFds)
			&&  pProcessData->FileHandlers[fd]!=NULL
			&&  pProcessData->FileHandlers[fd]->CanRead() ) {
				LINUX_FD_SET(fd, &ReadResults);
				foundData = true;
			}

			if( pWriteFds
			&&  LINUX_FD_ISSET(fd, pWriteFds)
			&&  pProcessData->FileHandlers[fd]!=NULL
			&&  pProcessData->FileHandlers[fd]->CanWrite() ) {
				LINUX_FD_SET(fd, &WriteResults);
				foundData = true;
			}

			if( pExceptFds
			&&  LINUX_FD_ISSET(fd, pExceptFds)
			&&  pProcessData->FileHandlers[fd]!=NULL
			&&  pProcessData->FileHandlers[fd]->HasException() ) {
				LINUX_FD_SET(fd, &ExceptResults);
				foundData = true;
			}
		}
		if(foundData)
			break;

		//loop
		if(dwWait==0)
			break;
		//Sleep( (pTimeout->tv_sec*1000L + pTimeout->tv_usec) / 10 );
		Sleep(50);
	}

	ktrace("new_select found data: %d\n", foundData);

	//store results
	if(pReadFds)
		*pReadFds = ReadResults;
	if(pWriteFds)
		*pWriteFds = WriteResults;
	if(pExceptFds)
		*pExceptFds = ExceptResults;
	ctx.Eax = 0;
#endif
}

/*****************************************************************************/


/*
 * int mkdir(path)
 * create a new directory
 */
void SysCalls::sys_mkdir(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	DWORD mode = ctx.Ecx;

	Path p;
	p.SetUnixPath((const char *)ctx.Ebx);

	if(CreateDirectory(p.Win32Path(), NULL))
	{
		ctx.Eax = 0;
		return;
	}

	DWORD err = GetLastError();
	ctx.Eax = -Win32ErrToUnixError(err);
#endif
}

/*
 * int rmdir(path)
 * remove a directory - it must be empty
 */
void SysCalls::sys_rmdir(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	Path p(false);
	p.SetUnixPath((const char *)ctx.Ebx);

	if(RemoveDirectory(p.Win32Path()))
	{
		ctx.Eax = 0;
		return;
	}

	DWORD err = GetLastError();
	ctx.Eax = -Win32ErrToUnixError(err);

	if(err!=ERROR_SHARING_VIOLATION)
		return; //return this error


	//file is still being used. In unix you can delete such a file
	//but not in windows. However in windows it can often be successfully
	//renamed within the same directory, so try to do this with a temporary
	//name and retry later
	/*
	for(int i=0; i<10000; ++i) //if 10000 files have been deleted we will fail - on well :-)
	{
		char p2[MAX_PATH];

	}
	*/
	return; //return the original error
#endif
}


/*****************************************************************************/


/*
 * int rename(oldpath, newpath)
 */
void SysCalls::sys_rename(CONTEXT &ctx)
{
	Unhandled(ctx);
#if 0
	Path OldP, NewP;

	OldP.SetUnixPath((const char *)ctx.Ebx);
	NewP.SetUnixPath((const char *)ctx.Ecx);

	char * p2 = NULL;
	if(OldP.IsSymbolicLink())
	{
		int len = strlen(p2);
		len+=4;
		p2 = new char[len];
		StringCbPrintf(p2, len, "%s.lnk", NewP.Win32Path);
	}

	if(MoveFileEx(OldP.Win32Path(), p2?p2:NewP.Win32Path(), MOVEFILE_REPLACE_EXISTING))
		ctx.Eax = 0;
	else
		ctx.Eax = -Win32ErrToUnixError(GetLastError());

	if(p2)
		delete p2;
#endif
}

