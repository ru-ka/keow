/*
 * generic IO handler routines
 */
#include "kernel.h"
#include "iohandler.h"


IOHandler::IOHandler(int size)
{
	m_ClassSize = size;
	m_Handle = INVALID_HANDLE_VALUE;
	m_Inheritable = true; //default
	m_Flags = 0;
	m_Path[0] = 0;
}

IOHandler::~IOHandler()
{
	Close();
}

bool IOHandler::DupBaseData(IOHandler& from, HANDLE hFromProcess, HANDLE hToProcess)
{
	m_Inheritable = from.m_Inheritable;
	m_Flags = from.m_Flags;
	StringCbCopy(m_Path, sizeof(m_Path), from.m_Path);

	//dup with inheritance allowed
	if(from.m_Handle==INVALID_HANDLE_VALUE)
	{
		if(m_Handle!=INVALID_HANDLE_VALUE)
			CloseHandle(m_Handle);
		m_Handle = INVALID_HANDLE_VALUE;
	}
	else
	{
		if(!DuplicateHandle(hFromProcess, from.m_Handle, hToProcess, &m_Handle, NULL, TRUE, DUPLICATE_SAME_ACCESS))
			return false;
	}
	return true;
}

bool IOHandler::Close()
{
	if(m_Handle!=INVALID_HANDLE_VALUE)
		if(CloseHandle(m_Handle))
			m_Handle = INVALID_HANDLE_VALUE;
	return m_Handle==INVALID_HANDLE_VALUE;
}

const char * IOHandler::GetWin32Path()
{
	return m_Path;
}
const char * IOHandler::GetUnixPath()
{
	return &m_Path[pKernelSharedData->LinuxFileSystemRootLen];
}


/*
 * can be generic - relies on virtual Stat64
 */
bool IOHandler::Stat(linux::stat* s)
{
	linux::stat64 s64;

	bool ret = this->Stat64(&s64);

	s->st_atime = s64.st_atime;
	s->st_ctime = s64.st_ctime;
	s->st_mtime = s64.st_mtime;

	s->st_dev = s64.st_dev;
	s->st_rdev = s64.st_rdev;
	s->st_ino = (unsigned long)s64.st_ino;

	s->st_mode = s64.st_mode;
	s->st_nlink = s64.st_nlink;

	s->st_uid = (unsigned short)s64.st_uid;
	s->st_gid = (unsigned short)s64.st_gid;

	s->st_size = (unsigned long)s64.st_size > (unsigned long)-1 ? (unsigned long)-1 : (unsigned long)s64.st_size;
	s->st_blksize = s64.st_blksize;
	s->st_blocks = s64.st_blocks;


	s->__pad1 = s->__pad2 = 0;

	return ret;
}


/*
 * helper - minimal Stat64
 */
void IOHandler::BasicStat64(linux::stat64 * s, int file_type)
{
	//pading fields are very sensisitive to contents?!

	//memset(s, 0, sizeof(struct linux::stat64));
	//memset(s->__pad0, 0, sizeof(s->__pad0));
	//memset(s->__pad3, 0, sizeof(s->__pad3));
	s->__pad4 = s->__pad5 = s->__pad6 = 0;


	//basic details

	s->st_mode = 0755;  // rwxrwxrwx
	if(file_type)
		s->st_mode |= file_type;
	else
		s->st_mode |= GetUnixFileType(m_Path);

	s->st_nlink = 1;
	s->st_uid = 0;
	s->st_gid = 0;

	s->st_dev = 3<<8|4;	//dummy
	s->st_rdev = 3<<8|4;

	s->st_ino = 1999;    //dummy inode
	s->__st_ino = 1999;

	s->st_size = 0;

	s->st_blksize = 512; //block size for efficient IO
	
	s->st_blocks = 0;

	s->st_atime = 0;//FILETIME_TO_TIME_T(fi.ftLastAccessTime);
	s->st_mtime = 0;//FILETIME_TO_TIME_T(fi.ftLastWriteTime);
	s->st_ctime = 0;//FILETIME_TO_TIME_T(fi.ftCreationTime);
}

