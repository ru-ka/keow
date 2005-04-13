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

/*
 * an IO handler for /proc filesystem
 */
#include "kernel.h"
#include "iohandler.h"


ProcIOHandler::ProcIOHandler(Path path) 
: IOHandler(sizeof(ProcIOHandler))
, m_Path(path)
, m_pProcObjectData(NULL)
{
	CalculateProcObject();
}

bool ProcIOHandler::Open(Path& filepath, DWORD access, DWORD ShareMode, DWORD disposition, DWORD flags)
{
	Close();

	m_Path = filepath;

	if(!CalculateProcObject())
	{
		SetLastError(ERROR_PATH_NOT_FOUND);
		return false;
	}

	return true;
}

bool ProcIOHandler::Close()
{
	if(m_pProcObjectData)
		delete m_pProcObjectData;
	m_pProcObjectData = NULL;

	return IOHandler::Close();
}

bool ProcIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	//EOF?
	if(m_dwDataOffset >= m_dwDataSize
	|| m_pProcObjectData == NULL )
	{
		if(pRead)
			*pRead = 0;
		return true;
	}

	DWORD dwAvail = m_dwDataSize-m_dwDataOffset;
	if(size > dwAvail)
	{
		memcpy(address, m_pProcObjectData+m_dwDataOffset, dwAvail);
		if(pRead)
			*pRead = dwAvail;
		m_dwDataOffset += dwAvail;
	}
	else
	{
		memcpy(address, m_pProcObjectData+m_dwDataOffset, size);
		if(pRead)
			*pRead = size;
		m_dwDataOffset += size;
	}
	return true;
}


bool ProcIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	//we don't support updating proc entries
	if(pWritten)
		*pWritten = 0;
	return false;
}


IOHandler* ProcIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	//can recreate most of the state from the filename
	ProcIOHandler * h = new ProcIOHandler(m_Path);

	//but need to know how much has been accessed already
	h->m_dwDataOffset = m_dwDataOffset;

	return h;
}


DWORD ProcIOHandler::ioctl(DWORD request, DWORD data)
{
	//ignore all
	return 0;
}

bool ProcIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, 0);

	//if(writable)
	//	s->st_mode = 0744;  // rwxr--r--
	//else
		s->st_mode = 0444;  // r--r--r--

	switch(m_ProcObjectType)
	{
	case TypeDir:
		s->st_mode |= S_IFDIR;
		s->st_nlink = 1; //should really be number of entries in dir?
		s->st_size = m_dwDataSize; //??
		break;

	case TypeSymLink:
		s->st_mode |= S_IFLNK;
		s->st_nlink = 1;
		s->st_size = m_dwDataSize; //??
		break;

	case TypeData:
	default:
		//regular file data
		s->st_mode |= S_IFREG;
		s->st_nlink = 1;
		s->st_size = m_dwDataSize;
		break;
	}


	s->st_uid = 0;
	s->st_gid = 0;

	s->st_dev = 3<<8|4;  //????
	s->st_rdev = 3<<8|4;

	s->st_ino = 1999;    //dummy inode
	s->__st_ino = 1999;

	s->st_blksize = 512; //block size for efficient IO
	
	s->st_blocks = (unsigned long)((s->st_size+511) / 512); //size in 512 byte blocks

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	s->st_atime = FILETIME_TO_TIME_T(ft);
	s->st_mtime = FILETIME_TO_TIME_T(ft);
	s->st_ctime = FILETIME_TO_TIME_T(ft);


	return true;
}

bool ProcIOHandler::CanRead()
{
	//always?
	return true;
}

bool ProcIOHandler::CanWrite()
{
	//we don't support updating proc entries
	return false;
}

bool ProcIOHandler::HasException()
{
	//always ok?
	return false;
}


int ProcIOHandler::GetDirEnts64(linux::dirent64 * de, int maxbytes)
{
	if(m_ProcObjectType != TypeDir)
		return 0;

	if(m_dwDataOffset >= m_dwDataSize)
		return 0;

	//return one entry

	de->d_ino = 1; //dummy value

	de->d_off = m_dwDataOffset; //????

	de->d_reclen = sizeof(linux::dirent64);

	de->d_type = 0; //not provided on linux x86 32bit?  (GetUnixFileType(p);

	StringCbCopy(de->d_name, sizeof(de->d_name), (char*)&m_pProcObjectData[m_dwDataOffset]);

//	filled += de->d_reclen;
	while(m_pProcObjectData[m_dwDataOffset]!=NULL)
		m_dwDataOffset++;
	m_dwDataOffset++; //skip over the null terminator

	return 1;
}

ULONGLONG ProcIOHandler::Length()
{
	return m_dwDataSize;
}

ULONGLONG ProcIOHandler::Seek(ULONGLONG uoffset, DWORD method)
{
	DWORD offset;
	if(uoffset > ULONG_MAX)
		offset = ULONG_MAX;
	else
		offset = (DWORD)uoffset;

	switch(method) //Win32 methods
	{
	case FILE_END:
		m_dwDataOffset = (m_dwDataSize-1) - offset;
		if(m_dwDataOffset > m_dwDataSize)
			m_dwDataOffset = m_dwDataSize;
		break;
	case FILE_BEGIN:
		m_dwDataOffset = offset;
		if(m_dwDataOffset > m_dwDataSize)
			m_dwDataOffset = m_dwDataSize;
		break;
	case FILE_CURRENT:
		m_dwDataOffset += offset;
		if(m_dwDataOffset > m_dwDataSize)
			m_dwDataOffset = m_dwDataSize;
		break;
	}
	return m_dwDataOffset;
}


/*
 * determine what this /proc file is, and it's contents
 */
bool ProcIOHandler::CalculateProcObject()
{
	if(m_pProcObjectData)
		delete m_pProcObjectData;
	m_pProcObjectData = NULL;
	m_dwDataSize = 0;
	m_dwDataOffset = 0;

	char * pProcPath = strdup(m_Path.CurrentFileSystemUnixPath());
	if(!pProcPath)
		return false;

	//possible that we have not path, so don't need p
	char * p = NULL;
	if(*pProcPath) {
		p = strtok(pProcPath+1, "/"); //+1 to skip the initial '/'
	}

	bool ok = false;

	if(*pProcPath==0
	|| p==0)
	{
		//the proc root directory - contents is pid's and system entries

		m_ProcObjectType = TypeDir;

		//make a dummy directory contents - just filenames (include . and ..)
		const char proc_root_common[] = ".\0..\0self\0cpuinfo\0meminfo\0uptime\0stat";

		//allow a 5byte pid and a null per process + common stuff
		m_pProcObjectData = new BYTE[MAX_PROCESSES*6 + sizeof(proc_root_common)];

		//populate buffer
		m_dwDataSize = 0; //not filled yet
		for(int i=0; i<MAX_PROCESSES; ++i)
		{
			if(pKernelSharedData->ProcessTable[i].in_use)
			{
				StringCbPrintfA((char*)&m_pProcObjectData[m_dwDataSize], 6, "%d", i);
				while(m_pProcObjectData[m_dwDataSize]!=NULL) {
					m_dwDataSize++;
				}
				m_dwDataSize++; //include the null
			}
		}
		//add constant stuff
		memcpy(&m_pProcObjectData[m_dwDataSize], proc_root_common, sizeof(proc_root_common));
		m_dwDataSize += sizeof(proc_root_common);

		ok = true;
	}
	else
	if(isdigit(*p)
	|| strcmp(p, "self")==0)
	{
		//is a pid sub-directory
		int pid;
		if(isdigit(*p))
			pid = atoi(p);
		else
			pid = pProcessData->PID; //self

		//what pid object?
		p = strtok(NULL, "/");

		if(p==NULL)
		{
			//just the pid directory
			m_ProcObjectType = TypeDir;

			//make a dummy directory contents - just filenames (include . and ..)
			const char proc_pid_files[] = ".\0..\0cmdline\0cwd\0environ\0exe\0fd\0maps\0mem\0root\0stat\0statm\0status";

			m_dwDataSize = sizeof(proc_pid_files);
			m_pProcObjectData = new BYTE[m_dwDataSize];
			memcpy(m_pProcObjectData, proc_pid_files, m_dwDataSize);

			ok = true;
		}
		else
		if(strcmp(p,"cmdline")==0)
		{
			//TODO: get full commandline
			m_ProcObjectType = TypeData;
			m_dwDataSize = sizeof(pKernelSharedData->ProcessTable[pid].ProgramPath);
			m_pProcObjectData = new BYTE[m_dwDataSize];
			memcpy(m_pProcObjectData, pKernelSharedData->ProcessTable[pid].ProgramPath, m_dwDataSize);
			m_dwDataSize = strlen((char*)m_pProcObjectData)+1; //include null
			ok = true;
		}
		else
		if(strcmp(p,"cwd")==0)
		{
			m_ProcObjectType = TypeSymLink;
			m_dwDataSize = sizeof(pKernelSharedData->ProcessTable[pid].unix_pwd);
			m_pProcObjectData = new BYTE[m_dwDataSize];
			memcpy(m_pProcObjectData, pKernelSharedData->ProcessTable[pid].unix_pwd, m_dwDataSize);
			m_dwDataSize = strlen((char*)m_pProcObjectData)+1; //include null
			ok = true;
		}
		else
		if(strcmp(p,"environ")==0)
		{
		}
		else
		if(strcmp(p,"exe")==0)
		{
		}
		else
		if(strcmp(p,"fd")==0)
		{
		}
		else
		if(strcmp(p,"maps")==0)
		{
		}
		else
		if(strcmp(p,"mem")==0)
		{
		}
		else
		if(strcmp(p,"root")==0)
		{
		}
		else
		if(strcmp(p,"stat")==0)
		{
		}
		else
		if(strcmp(p,"statm")==0)
		{
		}
		else
		if(strcmp(p,"status")==0)
		{
		}
	}
	else
	if(strcmp(p,"meminfo")==0)
	{
		//build a buffer of info
		//eg:
		//           total:      used:      free:
		//  Mem:   737132544  370999296  366133248
		//  Swap:  301989888   32059392  269930496
		//  MemTotal:         719856 kB
		//  MemFree:          357552 kB
		//  MemShared:             0 kB
		//  SwapTotal:        294912 kB
		//  SwapFree:         263604 kB

		m_ProcObjectType = TypeData;

		m_dwDataSize = 4000; //more than enough?
		m_pProcObjectData = new BYTE[m_dwDataSize];

		MEMORYSTATUS ms;
		ms.dwLength = sizeof(ms);
		GlobalMemoryStatus(&ms);

		StringCbPrintf((char*)m_pProcObjectData, m_dwDataSize,
				"\t  total:  \t   used:  \t   free: \x0a"
				"Mem: \t%10ld \t%10ld \t%10ld \x0a"
				"Swap:\t%10ld \t%10ld \t%10ld \x0a"
				"MemTotal: \t%10ld kB\x0a"
				"MemFree:  \t%10ld kB\x0a"
				"MemShared:\t%10ld kB\x0a"
				"SwapTotal:\t%10ld kB\x0a"
				"SwapFree: \t%10ld kB\x0a"
				, ms.dwTotalPhys, ms.dwTotalPhys-ms.dwAvailPhys, ms.dwAvailPhys
				, ms.dwTotalPageFile, ms.dwTotalPageFile-ms.dwAvailPageFile, ms.dwAvailPageFile
				, ms.dwTotalPhys / 1024
				, ms.dwAvailPhys / 1024
				, 0 //shared how to determine?
				, ms.dwTotalPageFile / 1024
				, ms.dwAvailPageFile / 1024
				);

		m_dwDataSize = strlen((char*)m_pProcObjectData);

		ok = true;
	}
	else
	if(strcmp(p,"cpuinfo")==0)
	{
		//build a buffer of info
		//eg:
		//  processor       : 0
		//  vendor_id       : GenuineIntel
		//  bogomips        : 2193
		//  fpu             : yes

		m_ProcObjectType = TypeData;

		m_dwDataSize = 4000; //more than enough?
		m_pProcObjectData = new BYTE[m_dwDataSize];

		SYSTEM_INFO si;
		GetSystemInfo(&si);

		unsigned int dwLen = m_dwDataSize;
		char * pData = (char*)m_pProcObjectData;
		char * pDataEnd = 0;

		for(DWORD cpu=0; cpu<si.dwNumberOfProcessors; ++cpu)
		{
			StringCbPrintfEx(pData, dwLen, &pDataEnd, &dwLen, 0,
					"processor : %d\x0a"
					"vendor_id : %s\x0a"
					"bobomips  : %d\x0a"
					"cpu count : %d\x0a"
					"fpu       : %s\x0a"
					, cpu
					, "Intel compatible (keow)"
					, pKernelSharedData->BogoMips
					, si.dwNumberOfProcessors
					, IsProcessorFeaturePresent(PF_FLOATING_POINT_EMULATED) ? "no" : "yes"
					);
		}

		m_dwDataSize = strlen((char*)m_pProcObjectData);

		ok = true;
	}
	else
	if(strcmp(p,"uptime")==0)
	{
		//build a buffer of info

		m_ProcObjectType = TypeData;

		m_dwDataSize = 1000; //more than enough?
		m_pProcObjectData = new BYTE[m_dwDataSize];

		float uptime = GetTickCount() / (float)1000;  //tick count is ms since booted
		float idletime = uptime/2;  //50% :-)   we aren't reading performance stuff yet

		StringCbPrintf((char*)m_pProcObjectData, m_dwDataSize,
				"%.2lf %.2lf\x0a"
				, uptime
				, idletime
				);

		m_dwDataSize = strlen((char*)m_pProcObjectData);

		ok = true;
	}
	else
	if(strcmp(p,"stat")==0)
	{
		//build a buffer of info

		m_ProcObjectType = TypeData;

		m_dwDataSize = 1000; //more than enough?
		m_pProcObjectData = new BYTE[m_dwDataSize];

		DWORD user, nice, sys, idle;
		user = nice = sys = idle = 0;

		StringCbPrintf((char*)m_pProcObjectData, m_dwDataSize,
				"cpu %ld %ld %ld %ld\x0a"
				"btime %ld\x0a"
				"processes %ld\x0a"
				, user, nice, sys, idle
				, GetTickCount()/1000
				, pKernelSharedData->ForksSinceBoot
				);

		m_dwDataSize = strlen((char*)m_pProcObjectData);

		ok = true;
	}


	free(pProcPath);
	return ok;
}

