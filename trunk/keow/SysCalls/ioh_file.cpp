/*
 * and IO handler for regular files
 */
#include "kernel.h"
#include "iohandler.h"



FileIOHandler::FileIOHandler() 
: IOHandler(sizeof(FileIOHandler))
{
	m_IsADirectory = false;
	m_hFindData = INVALID_HANDLE_VALUE;
}

bool FileIOHandler::Close()
{
	if(m_hFindData != INVALID_HANDLE_VALUE)
		FindClose(m_hFindData);
	m_hFindData = INVALID_HANDLE_VALUE;

	return IOHandler::Close();
}


bool FileIOHandler::Open(const char * filename, DWORD access, DWORD ShareMode, DWORD disposition, DWORD flags)
{
	StringCbCopy(m_Path, sizeof(m_Path), filename);

	if(m_hFindData != INVALID_HANDLE_VALUE)
		FindClose(m_hFindData);
	m_hFindData = INVALID_HANDLE_VALUE;

	DWORD attr = GetFileAttributes(filename);
	if((attr!=INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		m_IsADirectory = true;
		flags = FILE_FLAG_BACKUP_SEMANTICS;
	}

	HANDLE h = CreateFile(filename, access, ShareMode, NULL, disposition, flags, NULL);
	//can inherit handle....
	if(h==INVALID_HANDLE_VALUE)
	{
		if(m_Handle!=INVALID_HANDLE_VALUE)
			CloseHandle(m_Handle);
		m_Handle = INVALID_HANDLE_VALUE;
	}
	else
		DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &m_Handle, 0, TRUE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);

	return m_Handle!=INVALID_HANDLE_VALUE;
}


bool FileIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	return ReadFile(m_Handle, address, size, pRead, NULL)!=FALSE;
}


bool FileIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	return WriteFile(m_Handle, address, size, pWritten, NULL)!=FALSE;
}


IOHandler* FileIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	FileIOHandler *ionew = new FileIOHandler();
	if(ionew)
	{
		ionew->m_IsADirectory = m_IsADirectory;

		if(m_hFindData==INVALID_HANDLE_VALUE)
			ionew->m_hFindData = m_hFindData;
		else
		{
			//Can't dup a find handle, so recreate the find
			if(hToProcess != GetCurrentProcess())
			{
				//can't dup this way around
				delete ionew;
				return NULL;
			}

			linux::dirent64 tmp;
			ionew->GetDirEnts64(&tmp, sizeof(tmp));
			ionew->m_nFindCount = 0;
			while(ionew->m_nFindCount < m_nFindCount)
				ionew->GetDirEnts64(&tmp, sizeof(tmp));

		}


		if(ionew->DupBaseData(*this, hFromProcess, hToProcess))
			return ionew;

		delete ionew;
	}
	return NULL;
}


bool FileIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;


	BY_HANDLE_FILE_INFORMATION fi;
	ULARGE_INTEGER i;


	IOHandler::BasicStat64(s, 0);


	if(!GetFileInformationByHandle(m_Handle, &fi))
		return false;


	if(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		s->st_mode = 0555;  // r-xr-xr-x
	else
		s->st_mode = 0755;  // rwxrwxrwx
	s->st_mode |= GetUnixFileType(m_Path);

	s->st_nlink = fi.nNumberOfLinks;
	s->st_uid = 0;
	s->st_gid = 0;

	s->st_dev = 3<<8|4;
	s->st_rdev = 3<<8|4;

	s->st_ino = 1999;    //dummy inode
	s->__st_ino = 1999;

	i.LowPart = fi.nFileSizeLow;
	i.HighPart = fi.nFileSizeHigh;
	s->st_size = i.QuadPart;

	s->st_blksize = 512; //block size for efficient IO
	
	s->st_blocks = (unsigned long)((s->st_size+511) / 512); //size in 512 byte blocks

	s->st_atime = FILETIME_TO_TIME_T(fi.ftLastAccessTime);
	s->st_mtime = FILETIME_TO_TIME_T(fi.ftLastWriteTime);
	s->st_ctime = FILETIME_TO_TIME_T(fi.ftCreationTime);


	return true;
}


DWORD FileIOHandler::Length()
{
	return GetFileSize(m_Handle, NULL);
}



DWORD FileIOHandler::Seek(DWORD offset, DWORD method)
{
	return SetFilePointer(m_Handle, offset, 0, method);
}


DWORD FileIOHandler::ioctl(DWORD request, DWORD data)
{
	switch(request)
	{
		/*
	case TCGETS:
		{
			linux::termios * arg = (linux::termios*)pCtx->Edx;
			arg->c_iflag = 0;		/* input mode flags *
			arg->c_oflag = 0;		/* output mode flags *
			arg->c_cflag = 0;		/* control mode flags *
			arg->c_lflag = 0;		/* local mode flags *
			arg->c_line = 0;			/* line discipline *
			//arg->c_cc[NCCS];		/* control characters *
			return 0;
		}
		break;
		*/
	case 0:
	default:
		ktrace("IMPLEMENT sys_ioctl 0x%lx for FileIOHandler\n", request);
		return -ENOSYS;
	}
}


int FileIOHandler::GetDirEnts64(linux::dirent64 *de, int maxbytes)
{
	DWORD err = 0;
	WIN32_FIND_DATA wfd;
	char p[MAX_PATH], up[MAX_PATH];

	int filled = 0;
	while(filled+(int)sizeof(linux::dirent64) < maxbytes)
	{
		if(m_hFindData==INVALID_HANDLE_VALUE)
		{
			char DirPattern[MAX_PATH];
			StringCbPrintf(DirPattern, sizeof(DirPattern), "%s/*.*", m_Path);

			m_hFindData = FindFirstFile(DirPattern, &wfd);
			//can't inherit handle, so set count of 'find' so child processes can recreate our state
			m_nFindCount = 0;

			if(m_hFindData==INVALID_HANDLE_VALUE)
				return -1;
		}
		else
		{
			m_nFindCount++;
			if(!FindNextFile(m_hFindData, &wfd))
			{
				err = GetLastError();
				if(err==ERROR_NO_MORE_FILES)
				{
					//only close if returning no results - otherwise leave open
					//for the next call to detect end of data.
					if(filled==0)
					{
						FindClose(m_hFindData);
						m_hFindData = INVALID_HANDLE_VALUE;
					}
					err = 0;
				}
				break;
			}
		}

		de->d_ino = 1; //dummy value

		de->d_off = filled+sizeof(linux::dirent64); //????

		de->d_reclen = sizeof(linux::dirent64);

		StringCbCopy(up, sizeof(up), GetUnixPath());
		StringCbCat(up, sizeof(up)-strlen(up), "/");
		StringCbCat(up, sizeof(up)-strlen(up), wfd.cFileName);
		MakeWin32Path(up, p, sizeof(p), false);

		de->d_type = 0; //not provided on linux x86 32bit?  (GetUnixFileType(p);

		StringCbCopy(de->d_name, sizeof(de->d_name), wfd.cFileName);

		if(IsSymbolicLink(p))
		{
			//ensure name we return does not end in .lnk
			int e = strlen(de->d_name) - 4;
			if(e>0 && stricmp(&de->d_name[e], ".lnk")==0)
				de->d_name[e] = 0;
		}

		filled += de->d_reclen;

	}

	if(err!=0)
	{
		filled = -1;
		SetLastError(err);
	}
	return filled;
}


bool FileIOHandler::CanRead()
{
	//ok if we are not at eof
	DWORD dwRead = 0;
	BYTE buf;
	if(!ReadFile(m_Handle, &buf, 0, &dwRead, NULL))
		return false;
	return true;
}

bool FileIOHandler::CanWrite()
{
	//always except to be able to write to the File?
	return true;
}

bool FileIOHandler::HasException()
{
	//TODO: what could this be?
	return false;
}
