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
	m_dwDataOffset = 0;

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
	if(m_dwDataOffset > m_dwDataSize
	|| m_pProcObjectData == NULL )
	{
		if(pRead)
			*pRead = 0;
		return false;
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

	IOHandler::BasicStat64(s, S_IFREG);

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

	//return one entry

	de->d_ino = 1; //dummy value

	de->d_off = m_dwDataOffset; //????

	de->d_reclen = sizeof(linux::dirent64);

	de->d_type = 0; //not provided on linux x86 32bit?  (GetUnixFileType(p);

	StringCbCopy(de->d_name, sizeof(de->d_name), (char*)&m_pProcObjectData[m_dwDataOffset]);

//	filled += de->d_reclen;

	return 1;
}

DWORD ProcIOHandler::Length()
{
	return m_dwDataSize;
}

DWORD ProcIOHandler::Seek(DWORD offset, DWORD method)
{
	switch(method) //Win32 methods
	{
	case FILE_END:
		m_dwDataOffset = m_dwDataSize - offset;
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

	char * pProcPath = strdup(m_Path.CurrentFileSystemUnixPath());
	if(!pProcPath)
		return false;

	char * p = strtok(pProcPath+1, "/"); //+1 to skip the initial '/'
	bool ok = false;

	if(p==NULL)
	{
		//the proc root directory - contents is pid's and system entries

		m_ProcObjectType = TypeDir;

		//make a dummy directory contents - just filenames
		const char proc_root_common[] = "cpuinfo\0meminfo\0\0";

		//allow a 5byte pid and a null per process + common stuff
		m_pProcObjectData = new BYTE[MAX_PROCESSES*6 + sizeof(proc_root_common)];

		//populate buffer
		m_dwDataSize = 0; //not filled yet
		for(int i=0; i<MAX_PROCESSES; ++i)
		{
			if(pKernelSharedData->ProcessTable[i].in_use)
			{
				m_dwDataSize += StringCbPrintfA((char*)&m_pProcObjectData[m_dwDataOffset], 6, "%d", i);
				m_dwDataSize++; //include the null
			}
		}
		//add constant stuff
		memcpy(&m_pProcObjectData[m_dwDataOffset], proc_root_common, sizeof(proc_root_common));
		m_dwDataSize += sizeof(proc_root_common);

	}
	else
	if(isdigit(*p))
	{
		//is a pid sub-directory
		int pid = atoi(p);

		//what pid object?
		p = strtok(NULL, "/");

		if(p==NULL)
		{
			//just the pid directory
			m_ProcObjectType = TypeDir;

			//make a dummy directory contents - just filenames
			const char proc_pid_files[] = "cmdline\0cwd\0environ\0exe\0fd\0maps\0mem\0root\0stat\0statm\0status\0\0";

			m_dwDataSize = sizeof(proc_pid_files);
			m_pProcObjectData = new BYTE[m_dwDataSize];
			memcpy(m_pProcObjectData, proc_pid_files, m_dwDataSize);
		}
		else
		if(strcmp(p,"cmdline")==0)
		{
			m_dwDataSize = 0;
			for(int i=0; pKernelSharedData->ProcessTable[pid].argv[i]!=0; ++i) {
				//include room for a space
				if(i>0)
					m_dwDataSize++;
				m_dwDataSize += strlen(pKernelSharedData->ProcessTable[pid].argv[i]);
			}

			m_pProcObjectData = new BYTE[m_dwDataSize];
			char *p = (char*)m_pProcObjectData;
			for(i=0; pKernelSharedData->ProcessTable[pid].argv[i]!=0; ++i) {
				//space seperated
				if(i>0) {
					*p = ' ';
					p++;
				}
				int len = strlen(pKernelSharedData->ProcessTable[pid].argv[i]);
				memcpy(p, pKernelSharedData->ProcessTable[pid].argv[i], len+1); //include the null
				p+=len;
			}

		}
		else
		{
			//unknown
			ok = false;
		}
	}
	else
	{
		//system-wide proc entry
	}

	free(pProcPath);
	return ok;
}

