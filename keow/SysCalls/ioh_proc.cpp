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

static const int Jiffies = 100;

/******************************************************************************/
/* Populators for each /proc entry */

typedef bool (*PopulateDataProc)(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize);


static bool Populate_proc_(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//the proc root directory - contents is pid's and system entries

	//make a dummy directory contents - just filenames (include . and ..)
	const char proc_root_common[] = ".\0..\0self\0cpuinfo\0meminfo\0uptime\0stat";

	//allow a 5byte pid and a null per process + common stuff
	pProcObjectData = new BYTE[MAX_PROCESSES*6 + sizeof(proc_root_common)];

	//populate buffer
	dwDataSize = 0; //not filled yet
	for(int i=0; i<MAX_PROCESSES; ++i)
	{
		if(pKernelSharedData->ProcessTable[i].in_use)
		{
			StringCbPrintfA((char*)&pProcObjectData[dwDataSize], 6, "%d", i);
			while(pProcObjectData[dwDataSize]!=NULL) {
				dwDataSize++;
			}
			dwDataSize++; //include the null
		}
	}
	//add constant stuff
	memcpy(&pProcObjectData[dwDataSize], proc_root_common, sizeof(proc_root_common));
	dwDataSize += sizeof(proc_root_common);

	return true;
}

static bool Populate_proc_pid_(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//make a dummy directory contents - just filenames (include . and ..)
	const char proc_pid_files[] = ".\0..\0cmdline\0cwd\0environ\0exe\0fd\0maps\0mem\0stat\0statm\0status";

	dwDataSize = sizeof(proc_pid_files);
	pProcObjectData = new BYTE[dwDataSize];
	memcpy(pProcObjectData, proc_pid_files, dwDataSize);

	return true;
}

static bool Populate_proc_pid_cmdline(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//TODO: get full commandline
	dwDataSize = sizeof(pKernelSharedData->ProcessTable[pid].ProgramPath);
	pProcObjectData = new BYTE[dwDataSize];
	memcpy(pProcObjectData, pKernelSharedData->ProcessTable[pid].ProgramPath, dwDataSize);
	dwDataSize = strlen((char*)pProcObjectData)+1; //include null

	return true;
}

static bool Populate_proc_pid_cwd(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	dwDataSize = sizeof(pKernelSharedData->ProcessTable[pid].unix_pwd);
	pProcObjectData = new BYTE[dwDataSize];
	memcpy(pProcObjectData, pKernelSharedData->ProcessTable[pid].unix_pwd, dwDataSize);
	dwDataSize = strlen((char*)pProcObjectData)+1; //include null

	return true;
}

static bool Populate_proc_pid_exe(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	dwDataSize = sizeof(pKernelSharedData->ProcessTable[pid].ProgramPath);
	pProcObjectData = new BYTE[dwDataSize];
	memcpy(pProcObjectData, pKernelSharedData->ProcessTable[pid].ProgramPath, dwDataSize);
	dwDataSize = strlen((char*)pProcObjectData)+1; //include null

	return true;
}

static bool Populate_proc_pid_stat(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	dwDataSize = MAX_PATH + 4000; //more than enough?
	pProcObjectData = new BYTE[dwDataSize];

	DWORD starttime = FILETIME_TO_TIME_T(pKernelSharedData->ProcessTable[pid].StartedTime) * Jiffies;
	int priority=10, nice=0;

	//TODO: fill out missing stat fields
	StringCbPrintf((char*)pProcObjectData, dwDataSize,
			"%d (%s) %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld 0 %ld %lu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d\x0a"
			,pid
			,pKernelSharedData->ProcessTable[pid].ProgramPath
			,'R'  //state, just using Running for now
			,pKernelSharedData->ProcessTable[pid].ParentPID
			,pKernelSharedData->ProcessTable[pid].ProcessGroupPID
			,0    //session id
			,0	  //tty number
			,pKernelSharedData->ProcessTable[pid].ProcessGroupPID //tpgid 
			,0    //flags
			,0	//minflt %lu\x0a,
			,0	//cminflt %lu\x0a,
			,0	//majflt %lu\x0a,
			,0	//cmajflt %lu\x0a,
			,0	//user time utime %lu\x0a,
			,0	//system time stime %lu\x0a,
			,0	//cutime %ld\x0a,
			,0	//cstime %ld\x0a,
			,priority
			,nice
			//,0 %ld\x0a,  //removed field
			,0	//itrealvalue %ld\x0a,
			,starttime
			,0	//vsize
			,0	//rss %ld\x0a,
			,0	//rlim %lu\x0a,
			,pKernelSharedData->ProcessTable[pid].program_base //??
			,pKernelSharedData->ProcessTable[pid].brk_base //??
			,pKernelSharedData->ProcessTable[pid].original_stack_esp
			,0	//current stack ESP  kstkesp %lu\x0a,
			,0	//current EIP  kstkeip %lu\x0a,
			,0	// .signal %lu\x0a,
			,0	//blocked %lu\x0a,
			,0	//~(pKernelSharedData->ProcessTable[pid].sigmask) //sigignore
			,0	//pKernelSharedData->ProcessTable[pid].sigmask //sigcatch %lu\x0a,
			,0	//wchan %lu\x0a,
			,0	//nswap %lu\x0a,
			,0	//cnswap %lu\x0a,
			,SIGCHLD	//exit_signal %d\x0a,
			,0	//processor %d\x0a,
			);

	dwDataSize = strlen((char*)pProcObjectData);

	return true;
}

static bool Populate_proc_pid_statm(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	dwDataSize = 100; //more than enough?
	pProcObjectData = new BYTE[dwDataSize];

	//TODO: fill out missing statm fields (measured in 4k pages?)
	StringCbPrintf((char*)pProcObjectData, dwDataSize,
			"%ld %ld %ld %ld %ld %ld %ld\x0a"
			,2	//size
			,1	//resident
			,0	//share
			,1	//trs
			,0	//drs
			,0	//lrs
			,0	//dt
			);

	dwDataSize = strlen((char*)pProcObjectData);

	return true;
}


static bool Populate_proc_meminfo(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//build a buffer of info

	dwDataSize = 4000; //more than enough?
	pProcObjectData = new BYTE[dwDataSize];

	MEMORYSTATUS ms;
	ms.dwLength = sizeof(ms);
	GlobalMemoryStatus(&ms);

	StringCbPrintf((char*)pProcObjectData, dwDataSize,
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

	dwDataSize = strlen((char*)pProcObjectData);

	return true;
}

static bool Populate_proc_cpuinfo(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//build a buffer of info

	dwDataSize = 4000; //more than enough?
	pProcObjectData = new BYTE[dwDataSize];

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	unsigned int dwLen = dwDataSize;
	char * pData = (char*)pProcObjectData;
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

	dwDataSize = strlen((char*)pProcObjectData);

	return true;
}

static bool Populate_proc_uptime(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//build a buffer of info

	dwDataSize = 1000; //more than enough?
	pProcObjectData = new BYTE[dwDataSize];

	float uptime = GetTickCount() / (float)1000;  //tick count is ms since booted
	float idletime = uptime/2;  //50% :-)   we aren't reading performance stuff yet

	StringCbPrintf((char*)pProcObjectData, dwDataSize,
			"%.2lf %.2lf\x0a"
			, uptime
			, idletime
			);

	dwDataSize = strlen((char*)pProcObjectData);

	return true;
}

static bool Populate_proc_stat(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//build a buffer of info

	dwDataSize = 1000; //more than enough?
	pProcObjectData = new BYTE[dwDataSize];

	DWORD uptime = GetTickCount() / 1000;  //tick count is ms since booted

	DWORD user, nice, sys, idle;
	user = nice = sys = 0;

	idle = uptime * Jiffies;

	StringCbPrintf((char*)pProcObjectData, dwDataSize,
			"cpu %ld %ld %ld %ld\x0a"
			"btime %ld\x0a"
			"processes %ld\x0a"
			, user, nice, sys, idle
			, GetTickCount()/1000
			, pKernelSharedData->ForksSinceBoot
			);

	dwDataSize = strlen((char*)pProcObjectData);

	return true;
}

static bool Populate_proc_loadavg(int pid, BYTE *& pProcObjectData, DWORD& dwDataOffset, DWORD &dwDataSize)
{
	//build a buffer of info

	dwDataSize = 100; //more than enough?
	pProcObjectData = new BYTE[dwDataSize];

	StringCbPrintf((char*)pProcObjectData, dwDataSize,
			"%ld %ld %ld\x0a"
			,1
			,1
			,1
			);

	dwDataSize = strlen((char*)pProcObjectData);

	return true;
}

static PopulateDataProc Populators[] = {
	Populate_proc_,

	Populate_proc_pid_,
	Populate_proc_pid_cmdline,
	Populate_proc_pid_cwd,
	Populate_proc_pid_exe,
	Populate_proc_pid_stat,
	Populate_proc_pid_statm,

	Populate_proc_meminfo,
	Populate_proc_cpuinfo,
	Populate_proc_uptime,
	Populate_proc_stat,
	Populate_proc_loadavg,

	NULL
};

static int GetPopulatorIndex(PopulateDataProc p)
{
	for(int i=0; Populators[i]!=NULL; ++i) {
		if(Populators[i]==p)
			return i;
	}
	DebugBreak(); //should never happen
	return -1;
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


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
	PopulateData();

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
	PopulateData();

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
	PopulateData();

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
	return 0;
}

ULONGLONG ProcIOHandler::Seek(ULONGLONG uoffset, DWORD method)
{
	PopulateData();

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
	m_bPopulated = false;
	m_nPopulator = -1;


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
		m_ProcObjectType = TypeDir;
		m_nPopulator = GetPopulatorIndex(Populate_proc_);
		ok = true;
	}
	else
	if(isdigit(*p)
	|| strcmp(p, "self")==0)
	{
		//is a pid sub-directory
		if(isdigit(*p))
			m_RelaventPid = atoi(p);
		else
			m_RelaventPid = pProcessData->PID; //self

		//what pid object?
		p = strtok(NULL, "/");

		if(p==NULL)
		{
			m_ProcObjectType = TypeDir;
			m_nPopulator = GetPopulatorIndex(Populate_proc_pid_);
			ok = true;
		}
		else
		if(strcmp(p,"cmdline")==0)
		{
			m_ProcObjectType = TypeData;
			m_nPopulator = GetPopulatorIndex(Populate_proc_pid_cmdline);
			ok = true;
		}
		else
		if(strcmp(p,"cwd")==0)
		{
			m_ProcObjectType = TypeSymLink;
			m_nPopulator = GetPopulatorIndex(Populate_proc_pid_cwd);
			ok = true;
		}
		else
		if(strcmp(p,"environ")==0)
		{
		}
		else
		if(strcmp(p,"exe")==0)
		{
			m_ProcObjectType = TypeSymLink;
			m_nPopulator = GetPopulatorIndex(Populate_proc_pid_exe);
			ok = true;
		}
		else
		if(strcmp(p,"fd")==0)
		{
			p = strtok(NULL, "/");
			//each open fd for the process
			//TODO:
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
		if(strcmp(p,"stat")==0)
		{
			m_ProcObjectType = TypeData;
			m_nPopulator = GetPopulatorIndex(Populate_proc_pid_stat);
			ok = true;
		}
		else
		if(strcmp(p,"statm")==0)
		{
			m_ProcObjectType = TypeData;
			m_nPopulator = GetPopulatorIndex(Populate_proc_pid_statm);
			ok = true;
		}
		else
		if(strcmp(p,"status")==0)
		{
		}
	}
	else
	if(strcmp(p,"meminfo")==0)
	{
		m_ProcObjectType = TypeData;
		m_nPopulator = GetPopulatorIndex(Populate_proc_meminfo);
		ok = true;
	}
	else
	if(strcmp(p,"cpuinfo")==0)
	{
		m_ProcObjectType = TypeData;
		m_nPopulator = GetPopulatorIndex(Populate_proc_cpuinfo);
		ok = true;
	}
	else
	if(strcmp(p,"uptime")==0)
	{
		m_ProcObjectType = TypeData;
		m_nPopulator = GetPopulatorIndex(Populate_proc_uptime);
		ok = true;
	}
	else
	if(strcmp(p,"stat")==0)
	{
		m_ProcObjectType = TypeData;
		m_nPopulator = GetPopulatorIndex(Populate_proc_stat);
		ok = true;
	}
	else
	if(strcmp(p,"loadavg")==0)
	{
		m_ProcObjectType = TypeData;
		m_nPopulator = GetPopulatorIndex(Populate_proc_loadavg);
		ok = true;
	}


	free(pProcPath);
	return ok;
}


bool ProcIOHandler::PopulateData()
{
	if(m_bPopulated)
		return true;

	if(m_nPopulator==-1)
		return false;

	return Populators[m_nPopulator](m_RelaventPid, m_pProcObjectData, m_dwDataOffset, m_dwDataSize);
}


