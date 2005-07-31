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

#include "kernel.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


//bit manipulation helpers for fd_set etc
//based on linux kernel versions
inline void LINUX_FD_SET(int fd, linux::fd_set * pSet) {
	__asm {
		push ebx
		mov eax,fd
		mov ebx, pSet
		bts [ebx],eax
		pop ebx
	}
}
inline void LINUX_FD_CLR(int fd, linux::fd_set * pSet) {
	__asm {
		push ebx
		mov eax,fd
		mov ebx, pSet
		btr [ebx],eax
		pop ebx
	}
}
inline BYTE LINUX_FD_ISSET(int fd, linux::fd_set * pSet) {
	BYTE result;
	__asm {
		push ebx
		mov eax,fd
		mov ebx, pSet;
		bt [ebx],eax
		setb result
		pop ebx
	}
	return result;
}
inline void LINUX_FD_ZERO(linux::fd_set * pSet) {
	memset(pSet, 0, __FDSET_LONGS);
}
/*
#define __FD_SET(fd,fdsetp) \
		__asm__ __volatile__("btsl %1,%0": \
			"=m" (*(__kernel_fd_set *) (fdsetp)):"r" ((int) (fd)))

#define __FD_CLR(fd,fdsetp) \
		__asm__ __volatile__("btrl %1,%0": \
			"=m" (*(__kernel_fd_set *) (fdsetp)):"r" ((int) (fd)))

#define __FD_ISSET(fd,fdsetp) (__extension__ ({ \
		unsigned char __result; \
		__asm__ __volatile__("btl %1,%2 ; setb %0" \
			:"=q" (__result) :"r" ((int) (fd)), \
			"m" (*(__kernel_fd_set *) (fdsetp))); \
		__result; }))

#define __FD_ZERO(fdsetp) \
do { \
	int __d0, __d1; \
	__asm__ __volatile__("cld ; rep ; stosl" \
			:"=m" (*(__kernel_fd_set *) (fdsetp)), \
			  "=&c" (__d0), "=&D" (__d1) \
			:"a" (0), "1" (__FDSET_LONGS), \
			"2" ((__kernel_fd_set *) (fdsetp)) : "memory"); \
} while (0)
*/



/*
 * write bytes to a handle
 * sys_write(handle,text,len)
 */
void sys_write(CONTEXT* pCtx)
{
	DWORD dwWritten;
	int fd;
	IOHandler * ioh;
	
	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh == NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	if(!ioh->Write((void*)pCtx->Ecx, pCtx->Edx, &dwWritten))
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	pCtx->Eax = dwWritten;
}

/*****************************************************************************/

/*
 * unsigned long sys_read(int fd, char*buf, unsigned long size)
 */
void  sys_read(CONTEXT* pCtx)
{
	DWORD dwRead;
	int fd;
	IOHandler * ioh;
	
	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh == NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	if(!ioh->Read((void*)pCtx->Ecx, pCtx->Edx, &dwRead))
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	pCtx->Eax = dwRead;
}

/*****************************************************************************/

/*
 * access(filename, perms)
 */
void  sys_access(CONTEXT* pCtx)
{
	Path p;
	DWORD attr;
	char ok = 1;
	DWORD check = pCtx->Ecx;

	p.SetUnixPath((const char*)pCtx->Ebx);

	attr = p.GetFileAttributes();
	if(attr==INVALID_FILE_ATTRIBUTES)
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	//read is ok if access already succeeded

	if( (check & W_OK) && (attr&FILE_ATTRIBUTE_READONLY) )
		ok = 0;

	if(ok)
		pCtx->Eax = 0;
	else
		pCtx->Eax = -EACCES;

}

/*****************************************************************************/


/*
 * int open(path, access, perms)
 * open or create a file
 */
void  sys_open(CONTEXT* pCtx)
{
	Path p;
	DWORD access = pCtx->Ecx;
	DWORD perms = pCtx->Edx;
	int fd;
	DWORD win32access, win32share, disposition, flags;
	IOHandler * ioh;

	ktrace("open(%s, 0x%lx, 0%lo)\n", pCtx->Ebx, access, perms);

	p.SetUnixPath((const char*)pCtx->Ebx);

	//find free handle entry
	fd = FindFreeFD();
	if(fd==-1)
	{
		pCtx->Eax = -EMFILE; //too many open files
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
	ioh = p.CreateIOHandler();
	if(ioh==NULL)
	{
		pCtx->Eax = -ENXIO; //no device available?
		return;
	}

	bool ok = ioh->Open(p, win32access, win32share, disposition, flags);
	if(!ok)
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		delete ioh;
		return;
	}


	KeowProcess()->FileHandlers[fd] = ioh;
	pCtx->Eax = fd;

	//more flags
	if(access & O_TRUNC) 
	{
		SetFilePointer(ioh->GetHandle(), 0, 0, FILE_BEGIN);
		SetEndOfFile(ioh->GetHandle());
	}
	if(access & O_APPEND)
	{
		SetFilePointer(ioh->GetHandle(), 0, 0, FILE_END);
	}
}


/*****************************************************************************/


/*
 * int close(handle)
 */
void  sys_close(CONTEXT* pCtx)
{
	int fd;
	IOHandler * ioh;
	
	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh == NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	if(!ioh->Close())
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	//free 
	delete ioh;
	KeowProcess()->FileHandlers[fd] = NULL;

	pCtx->Eax = 0;
}

/*****************************************************************************/


/*
 * int writev(int filedes, struct iovec* v, int count)
 * this is a write from several buffers
 */
void  sys_writev(CONTEXT* pCtx)
{
	linux::iovec *v = (linux::iovec *)pCtx->Ecx;
	int count = pCtx->Edx;
	DWORD dwWritten, dwTotal;
	int fd;
	IOHandler * ioh;
	
	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh == NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	dwTotal = 0;
	while(count>0)
	{
		dwWritten = 0;
		if(!ioh->Write(v->iov_base, v->iov_len, &dwWritten))
		{
			pCtx->Eax = -Win32ErrToUnixError(GetLastError());
			return;
		}
		count--;
		v++;
		dwTotal += dwWritten;
	}

	pCtx->Eax = dwWritten;

}


/*****************************************************************************/


/*
 * int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
 */
void  sys_select(CONTEXT* pCtx)
{
	sys__newselect(pCtx);
}


/*****************************************************************************/


/*
 * int ioctl(int fd, int request, ...)
 */
void  sys_ioctl(CONTEXT* pCtx)
{
	int fd;
	IOHandler * ioh;
	
	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh == NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	pCtx->Eax = ioh->ioctl( pCtx->Ecx, pCtx->Edx );
}


/*****************************************************************************/


/*
 * int getcwd(char* buf, int size)
 */
void  sys_getcwd(CONTEXT* pCtx)
{
	StringCbCopy((char*)pCtx->Ebx, pCtx->Ecx, KeowProcess()->unix_pwd);
	pCtx->Eax = 0;
}



/*****************************************************************************/


/*
 * int chdir(const char* path)
 */
void  sys_chdir(CONTEXT* pCtx)
{
	Path p;
	p.SetUnixPath((const char*)pCtx->Ebx);

	if(!SetCurrentDirectory(p.Win32Path()))
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		return;
	}

	ktrace("chdir %s\n", p.UnixPath());

	StringCbCopy(KeowProcess()->unix_pwd, sizeof(KeowProcess()->unix_pwd), p.UnixPath());
	pCtx->Eax = 0;
}

/*
 * int fchdir(int fd)
 */
void sys_fchdir(CONTEXT* pCtx)
{
	int fd;
	IOHandler * ioh;

	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];

	//delegate
	pCtx->Ebx = (DWORD)ioh->GetUnixPath();
	sys_chdir(pCtx);

	//restore overwritten data
	pCtx->Ebx = fd; 
}


/*****************************************************************************/


/*
 * int fcntl(int fd, int cmd, ...)
 */
void  sys_fcntl(CONTEXT* pCtx)
{
	int fd;
	IOHandler * ioh;
	
	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh == NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	pCtx->Eax = -EINVAL;

	switch(pCtx->Ecx)
	{
	case F_GETFD:	/* get close_on_exec */
		pCtx->Eax = ioh->GetInheritable() ? 0 : 1;
		break;

	case F_SETFD:   /* set/clear close_on_exec */
		ioh->SetInheritable(pCtx->Edx==0);
		pCtx->Eax = 0;
		break;

	case F_GETFL:	/* get file->f_flags */
		pCtx->Eax = ioh->GetFlags();
		break;

	case F_SETFL:	/* set file->f_flags */
		ioh->SetFlags(pCtx->Edx);
		pCtx->Eax = 0;
		break;

	case F_DUPFD:	/* like dup2() but use any fd<=arg and close-on-exec flag of copy is off */
		{
			int maxfd = pCtx->Edx;
			int fdnew;
			DWORD OldEcx;

			//find free handle entry
			fdnew = FindFreeFD();
			if(fdnew==-1)
			{
				pCtx->Eax = -EMFILE; //too many open files
				return;
			}
			if(fdnew > maxfd)
			{
				pCtx->Eax = EINVAL;
				return;
			}

			//use dup2 for actual work
			OldEcx = pCtx->Ecx;
			pCtx->Ecx = fdnew;
			sys_dup2(pCtx);
			pCtx->Ecx = OldEcx;
			KeowProcess()->FileHandlers[fdnew]->SetInheritable(false);
			//eax is already set with the result
		}
		break;

	default:
		ktrace("IMPLEMENT sys_fcntl 0x%lx\n", pCtx->Ecx);
		pCtx->Eax = -ENOSYS;
		break;
	}
}


/*****************************************************************************/


/*
 * int dup(int fd)
 */
void  sys_dup(CONTEXT* pCtx)
{
	int fdnew;
	DWORD OldEcx;

	//find free handle entry
	fdnew = FindFreeFD();
	if(fdnew==-1)
	{
		pCtx->Eax = -EMFILE; //too many open files
		return;
	}

	//use dup2 for actual work
	OldEcx = pCtx->Ecx;
	pCtx->Ecx = fdnew;
	sys_dup2(pCtx);
	pCtx->Ecx = OldEcx;
}


/*****************************************************************************/


/*
 * int dup2(int fd, int newfd)
 */
void  sys_dup2(CONTEXT* pCtx)
{
	int fdold, fdnew;
	IOHandler *ioh_old;

	//validate old handle
	fdold = pCtx->Ebx;
	if(fdold<0 || fdold>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh_old = KeowProcess()->FileHandlers[fdold];
	if(ioh_old == NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	//validate new handle
	fdnew = pCtx->Ecx;
	if(fdnew<0 || fdnew>MAX_OPEN_FILES || fdnew==fdold)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	//close any existing
	if(KeowProcess()->FileHandlers[fdnew] != NULL)
	{
		KeowProcess()->FileHandlers[fdnew]->Close();
		delete KeowProcess()->FileHandlers[fdnew];
		KeowProcess()->FileHandlers[fdnew] = NULL;
	}
	//make duplicate
	IOHandler *ioh_new = ioh_old->Duplicate(GetCurrentProcess(), GetCurrentProcess());
	if(ioh_new==NULL)
	{
		pCtx->Eax = Win32ErrToUnixError(GetLastError());
		return;
	}
	KeowProcess()->FileHandlers[fdnew] = ioh_new;
	pCtx->Eax = fdnew;
}


/*****************************************************************************/


/*
 * int getdents64(int fd, dirent64* d, int bytes)
 */
void sys_getdents64(CONTEXT* pCtx)
{
	linux::dirent64 * s = (linux::dirent64 *)pCtx->Ecx;
	int maxbytes = pCtx->Edx;
	int filled = 0;
	int fd;
	IOHandler * ioh;

	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];

	filled = ioh->GetDirEnts64(s, maxbytes);
	if(filled >= 0)
	{
		pCtx->Eax = filled;
	}
	else
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
	}
}


/*****************************************************************************/


/*
 * int stat(const char * name,  struct stat* buf)
 */
void sys_stat(CONTEXT* pCtx)
{
	const char * fname = (const char*)pCtx->Ebx;
	linux::stat * buf = (linux::stat*)pCtx->Ecx;
	Path p;

	p.SetUnixPath(fname);

	IOHandler * ioh = p.CreateIOHandler();
	if(ioh==NULL)
	{
		pCtx->Eax = -ENOMEM;
		return;
	}

	if(ioh->Open(p, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS))
	{
		if(ioh->Stat(buf))
		{
			pCtx->Eax = 0;
		}
		else
		{
			pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		}
	}
	else
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int lstat(const char * name,  struct stat* buf)
 */
void sys_lstat(CONTEXT* pCtx)
{
	const char * fname = (const char*)pCtx->Ebx;
	linux::stat * buf = (linux::stat*)pCtx->Ecx;

	//don't resolve symbolic links for lstat
	Path p(false);
	p.SetUnixPath(fname);

	IOHandler * ioh = p.CreateIOHandler();
	if(ioh==NULL)
	{
		pCtx->Eax = -ENOMEM;
		return;
	}

	if(ioh->Open(p, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS))
	{
		if(ioh->Stat(buf))
		{
			pCtx->Eax = 0;
		}
		else
		{
			pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		}
	}
	else
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int fstat(int fd,  struct stat* buf)
 */
void sys_fstat(CONTEXT* pCtx)
{
	linux::stat * buf = (linux::stat*)pCtx->Ecx;
	int fd;
	IOHandler * ioh;

	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];

	if(ioh->Stat(buf))
	{
		pCtx->Eax = 0;
	}
	else
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
	}

}


/*****************************************************************************/


/*
 * int stat64(const char * name,  struct stat64* buf)
 */
void sys_stat64(CONTEXT* pCtx)
{
	const char * fname = (const char*)pCtx->Ebx;
	linux::stat64 * buf = (linux::stat64*)pCtx->Ecx;

	Path p;
	p.SetUnixPath(fname);

	IOHandler * ioh = p.CreateIOHandler();
	if(ioh==NULL)
	{
		pCtx->Eax = -ENOMEM;
		return;
	}

	if(ioh->Open(p, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS))
	{
		if(ioh->Stat64(buf))
		{
			pCtx->Eax = 0;
		}
		else
		{
			pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		}
	}
	else
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int lstat64(const char * name,  struct stat64* buf)
 */
void sys_lstat64(CONTEXT* pCtx)
{
	const char * fname = (const char*)pCtx->Ebx;
	linux::stat64 * buf = (linux::stat64*)pCtx->Ecx;

	//don't resolve symbolic links for lstat
	Path p(false);
	p.SetUnixPath(fname);

	IOHandler * ioh = p.CreateIOHandler();
	if(ioh==NULL)
	{
		pCtx->Eax = -ENOMEM;
		return;
	}

	if(ioh->Open(p, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS))
	{
		if(ioh->Stat64(buf))
		{
			pCtx->Eax = 0;
		}
		else
		{
			pCtx->Eax = -Win32ErrToUnixError(GetLastError());
		}
	}
	else
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
	}

	delete ioh;
}


/*
 * int fstat64(int fd,  struct stat64* buf)
 */
void sys_fstat64(CONTEXT* pCtx)
{
	linux::stat64 * buf = (linux::stat64*)pCtx->Ecx;
	int fd;
	IOHandler * ioh;

	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh==NULL)
	{
		pCtx->Eax = -EBADF;
		return;
	}


	if(ioh->Stat64(buf))
	{
		pCtx->Eax = 0;
	}
	else
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
	}

}


/*****************************************************************************/

/*
 * int readlink(path, buf, size)
 */
void sys_readlink(CONTEXT* pCtx)
{
	int c;

	//don't follow symlinks
	Path p;
	p.FollowSymLinks(false);
	p.SetUnixPath((const char*)pCtx->Ebx);

	if(p.GetFileAttributes()==INVALID_FILE_ATTRIBUTES)
	{
		pCtx->Eax = -ENOENT; //no file
	}
	else
	if(!p.IsSymbolicLink())
	{
		pCtx->Eax = -EINVAL; //not a link
	}
	else
	{
		//want to follow it now
		p.FollowSymLinks(true);

		//readlink does not copy the null!
		char *pD = (char*)pCtx->Ecx;
		const char *pS = p.UnixPath();
		int max = (int)pCtx->Edx;
		for(c=0; *pS!=0 && c<max; ++c,++pD,++pS)
		{
			*pD = *pS;
		}

		pCtx->Eax = c; //ok
	}

}


/*****************************************************************************/

/*
 * int pipe(int filesdes[2])
 */
void sys_pipe(CONTEXT* pCtx)
{
	int *fds = (int*)pCtx->Ebx;

	//handles to open
	HANDLE hRead, hWrite;

	if(!CreatePipe(&hRead, &hWrite, NULL, 0))
	{
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
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
	KeowProcess()->FileHandlers[fds[0]] = new PipeIOHandler(hRead);
	fds[1] = FindFreeFD();
	KeowProcess()->FileHandlers[fds[1]] = new PipeIOHandler(hWrite);

	pCtx->Eax = 0;
}


/*****************************************************************************/

/*
 * int link(olfpath, newpath)
 */
void sys_link(CONTEXT* pCtx)
{
	Path OldP(false), NewP(false);

	OldP.SetUnixPath((const char*)pCtx->Ebx);
	NewP.SetUnixPath((const char*)pCtx->Ecx);

	if(CreateHardLink(NewP.Win32Path(), OldP.Win32Path(), NULL))
		pCtx->Eax = 0;
	else
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
}


/*
 * int unlink(path)
 */
void sys_unlink(CONTEXT* pCtx)
{
	Path p(false);
	p.SetUnixPath((const char *)pCtx->Ebx);

	if(DeleteFile(p.Win32Path()))
	{
		pCtx->Eax = 0;
		return;
	}

	DWORD err = GetLastError();
	pCtx->Eax = -Win32ErrToUnixError(err);

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
}


/*****************************************************************************/

/*
 * int _llseek(int fd, ulong offset_high, ulong offset_low, loff_t* result, uint whence)
 */
void sys__llseek(CONTEXT* pCtx)	
{
	int fd = pCtx->Ebx;
	LONG offset_high = pCtx->Ecx;
	LONG offset_low = pCtx->Edx;
	linux::loff_t* result = (linux::loff_t*)pCtx->Esi;
	DWORD whence = pCtx->Edi;

	IOHandler * ioh;

	fd = pCtx->Ebx;
	if(fd<0 || fd>MAX_OPEN_FILES)
	{
		pCtx->Eax = -EBADF;
		return;
	}

	ioh = KeowProcess()->FileHandlers[fd];
	if(ioh == NULL)
	{
		pCtx->Eax = -EBADF;
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
		pCtx->Eax = -EINVAL;
		return;
	}


	SetLastError(0);

	ULARGE_INTEGER li;
	li.LowPart = offset_low;
	li.HighPart = offset_high;
	*result = ioh->Seek(li.QuadPart, method);

	if(GetLastError()==0)
		pCtx->Eax = 0;
	else
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());
}


/*
 * off_t lseek(int fildes, off_t offset, int whence)
 */
void sys_lseek(CONTEXT* pCtx)
{
	//use llseek
	CONTEXT ctx2;
	linux::loff_t result;

	ctx2.Ebx = pCtx->Ebx; //fd
	ctx2.Ecx = 0; //offset, high
	ctx2.Edx = pCtx->Ecx; //offset, low
	ctx2.Esi = (DWORD)(&result);
	ctx2.Edi = pCtx->Edx; //whence

	sys__llseek(&ctx2);
	if(ctx2.Eax!=0)
	{
		//error
		pCtx->Eax = ctx2.Eax;
		return;
	}

	pCtx->Eax = (DWORD)(result & 0x7FFFFFFF);
}


/*****************************************************************************/

/*
 * int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
 */
void sys__newselect(CONTEXT* pCtx)
{
	int numFds = pCtx->Ebx;
	linux::fd_set *pReadFds = (linux::fd_set*)pCtx->Ecx;
	linux::fd_set *pWriteFds = (linux::fd_set*)pCtx->Edx;
	linux::fd_set *pExceptFds = (linux::fd_set*)pCtx->Esi;
	linux::timeval *pTimeout = (linux::timeval*)pCtx->Edi;

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
			&&  KeowProcess()->FileHandlers[fd]!=NULL
			&&  KeowProcess()->FileHandlers[fd]->CanRead() ) {
				LINUX_FD_SET(fd, &ReadResults);
				foundData = true;
			}

			if( pWriteFds
			&&  LINUX_FD_ISSET(fd, pWriteFds)
			&&  KeowProcess()->FileHandlers[fd]!=NULL
			&&  KeowProcess()->FileHandlers[fd]->CanWrite() ) {
				LINUX_FD_SET(fd, &WriteResults);
				foundData = true;
			}

			if( pExceptFds
			&&  LINUX_FD_ISSET(fd, pExceptFds)
			&&  KeowProcess()->FileHandlers[fd]!=NULL
			&&  KeowProcess()->FileHandlers[fd]->HasException() ) {
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
	pCtx->Eax = 0;
}

/*****************************************************************************/


/*
 * int mkdir(path)
 * create a new directory
 */
void  sys_mkdir(CONTEXT* pCtx)
{
	DWORD mode = pCtx->Ecx;

	Path p;
	p.SetUnixPath((const char *)pCtx->Ebx);

	if(CreateDirectory(p.Win32Path(), NULL))
	{
		pCtx->Eax = 0;
		return;
	}

	DWORD err = GetLastError();
	pCtx->Eax = -Win32ErrToUnixError(err);
}

/*
 * int rmdir(path)
 * remove a directory - it must be empty
 */
void  sys_rmdir(CONTEXT* pCtx)
{
	Path p(false);
	p.SetUnixPath((const char *)pCtx->Ebx);

	if(RemoveDirectory(p.Win32Path()))
	{
		pCtx->Eax = 0;
		return;
	}

	DWORD err = GetLastError();
	pCtx->Eax = -Win32ErrToUnixError(err);

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
}


/*****************************************************************************/


/*
 * int rename(oldpath, newpath)
 */
void sys_rename(CONTEXT* pCtx)
{
	Path OldP, NewP;

	OldP.SetUnixPath((const char *)pCtx->Ebx);
	NewP.SetUnixPath((const char *)pCtx->Ecx);

	char * p2 = NULL;
	if(OldP.IsSymbolicLink())
	{
		int len = strlen(p2);
		len+=4;
		p2 = new char[len];
		StringCbPrintf(p2, len, "%s.lnk", NewP.Win32Path);
	}

	if(MoveFileEx(OldP.Win32Path(), p2?p2:NewP.Win32Path(), MOVEFILE_REPLACE_EXISTING))
		pCtx->Eax = 0;
	else
		pCtx->Eax = -Win32ErrToUnixError(GetLastError());

	if(p2)
		delete p2;
}

