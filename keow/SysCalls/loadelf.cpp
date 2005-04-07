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
#include "loadelf.h"


const unsigned char ELF_SIGNATURE[16] = { 0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };



//
// Read the ELF header
// and verify it is an x86 ELF 32bit executable 
//
int LoadELFHeader(FILE * fElf, PELF_Data pElf)
{
	fseek(fElf, 0, SEEK_SET);
	fread(&pElf->hdr, sizeof(pElf->hdr), 1, fElf);


	if(pElf->hdr.e_ident[EI_MAG0]  != ELFMAG0
	|| pElf->hdr.e_ident[EI_MAG1]  != ELFMAG1
	|| pElf->hdr.e_ident[EI_MAG2]  != ELFMAG2
	|| pElf->hdr.e_ident[EI_MAG3]  != ELFMAG3
	|| pElf->hdr.e_ident[EI_CLASS] != ELFCLASS32
	|| pElf->hdr.e_ident[EI_DATA]  != ELFDATA2LSB
	)
	{
		ktrace("not a 32bit LSB ELF file\n");
		return -1;
	}
	if(pElf->hdr.e_type != ET_EXEC
	&& pElf->hdr.e_type != ET_DYN)
	{
		ktrace("not an ELF executable type\n");
		return -1;
	} 
	if(pElf->hdr.e_machine != EM_386)
	{
		ktrace("not for Intel 386 architecture (needed for syscall interception)\n");
		return -1;
	}
	if(pElf->hdr.e_version != EV_CURRENT
	|| pElf->hdr.e_ident[EI_VERSION] != EV_CURRENT )
	{
		ktrace("not version %d\n", EV_CURRENT);
		return -1;
	}

	return 0;
}


ADDR CalculateVirtualAddress(struct linux::elf32_phdr * phdr, ADDR pBaseAddr)
{
	//virtual address this program segment wants to load at

	//may need an offset

	return (phdr->p_vaddr + pBaseAddr);
}

DWORD ElfProtectionToWin32Protection(linux::Elf32_Word prot)
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



//
// Load the program into memory
//
int LoadProgram(FILE * fElf, PELF_Data pElf, bool LoadAsLibrary)
{
	int i;
	linux::Elf32_Phdr * phdr; //Program header
	ADDR pMem, pWantMem;
	DWORD protection;//, oldprot;
	char loadok;
	char Interpreter[MAX_PATH] = "";
	ADDR pMemTemp = NULL;
	ADDR pBaseAddr = 0;

	if(pElf->hdr.e_phoff == 0)
		return -1; //no header

	//what about these??
	if(LoadAsLibrary || pElf->hdr.e_type==ET_DYN)
	{
		if(pElf->last_lib_addr == 0)
			pBaseAddr = (ADDR)0x40000000L; //linux 2.4 x86 uses this for start of libs?
		else
			pBaseAddr = pElf->last_lib_addr;
		//align to next 64k boundary
		pBaseAddr = (ADDR)( ((DWORD)pBaseAddr + (SIZE64k-1)) & (~(SIZE64k-1)) ); 
		pElf->interpreter_base = pBaseAddr;
	}

	pElf->start_addr = pElf->hdr.e_entry + pBaseAddr;

	loadok=1;

	phdr = (linux::Elf32_Phdr*)new char[pElf->hdr.e_phentsize];
	if(phdr==0)
		loadok=0;

	//load program header segments
	//initial load - needs writable memory pages
	for(i=0; loadok && i<pElf->hdr.e_phnum; ++i)
	{
		fseek(fElf, pElf->hdr.e_phoff + (i*pElf->hdr.e_phentsize), SEEK_SET);
		fread(phdr, pElf->hdr.e_phentsize, 1, fElf);


		if(phdr->p_type == PT_LOAD
		|| phdr->p_type == PT_PHDR)
		{
			//load segment into memory
			pWantMem = CalculateVirtualAddress(phdr, pBaseAddr);
			protection = ElfProtectionToWin32Protection(phdr->p_flags);
			//phdr->p_memsz = (phdr->p_memsz + (phdr->p_align-1)) & (~(phdr->p_align-1)); //round up to alignment boundary
			pMem = AllocateMemAndProtectProcess(pElf->pinfo.hProcess, pWantMem, phdr->p_memsz, PAGE_EXECUTE_READWRITE);
			if(pMem!=pWantMem)
			{
				DWORD err = GetLastError();
				ktrace("error 0x%lx in load program segment @ 0x%lx (got 0x%lx)\n", err,pWantMem, pMem);
				loadok = 0;
				break;
			}
			//need to load into our memory, then copy to process
			//seems you cannot write across a 4k boundary, so write in blocks
			if(phdr->p_filesz != 0)
			{
				//int offset, size, write;

				pMemTemp = (ADDR)realloc(pMemTemp, phdr->p_filesz);
				fseek(fElf, phdr->p_offset, SEEK_SET);
				fread(pMemTemp, 1, phdr->p_filesz, fElf);

				if(!WriteMemory(pElf->pinfo.hProcess, pMem, phdr->p_filesz, pMemTemp))
				{
					DWORD err = GetLastError();
					ktrace("error 0x%lx in write program segment\n", err);
					loadok = 0;
					break;
				}
			}


			if(!LoadAsLibrary)
			{
				//keep track largest filled size and allocated size
				//between start_bss and last_bss is the bs section.
				if(pMem+phdr->p_filesz  > pElf->bss_start)
					pElf->bss_start = pMem + phdr->p_filesz;
				if(pMem+phdr->p_memsz  > pElf->brk)
					pElf->brk = pMem + phdr->p_memsz;

				//keep track in min/max addresses
				if(pMem<pElf->program_base || pElf->program_base==0)
					pElf->program_base = pMem;
				if(pMem+phdr->p_memsz > pElf->program_max)
					pElf->program_max = pMem+phdr->p_memsz;
			}
			else
			{
				if(pMem+phdr->p_memsz  > pElf->last_lib_addr)
					pElf->last_lib_addr = pMem + phdr->p_memsz;
			}

			//need program header location for possible interpreter
			if(phdr->p_type == PT_PHDR)
			{
				pElf->phdr_addr = pMem;
			}
		}
		else
		if(phdr->p_type == PT_INTERP)
		{
			fseek(fElf, phdr->p_offset, SEEK_SET);
			fread(Interpreter, 1, phdr->p_filesz, fElf);
		}
	}

	//align brk to a page boundary
	//NO!! //pElf->brk = (void*)( ((DWORD)pElf->brk + (SIZE4k-1)) & (~(SIZE4k-1)) ); 

	//load interpreter?
	if(Interpreter[0]!=0)
	{
		PELF_Data pElf2;

		if(strcmp(Interpreter,"/usr/lib/libc.so.1") == 0
		|| strcmp(Interpreter,"/usr/lib/ld.so.1") == 0 )
		{
			//IBCS2 interpreter? (tst from linux binfmt_elf.c)
			//I think these expect to use intel Call Gates 
			//we cannot (yet?) do these (only int80) so fail
			perror("Cannot handle IBCS executables");
			loadok = 0;
		}

		pElf2 = new ELF_Data;
		if(pElf2==NULL)
		{
			perror("cannot allocate ELF_Data");
			loadok = 0;
		}
		else
		{
			*pElf2 = *pElf;
			LoadELFFile(pElf2, Interpreter, true);
			pElf->interpreter_start = pElf2->start_addr;
			pElf->interpreter_base = pElf2->interpreter_base;
			//pElf->brk = pElf2->brk;
			pElf->last_lib_addr = pElf2->last_lib_addr;
			delete pElf2;
		}
	}

	//protect all the pages
	/* does not work?
	for(i=0; loadok && i<pElf->hdr.e_phnum; ++i)
	{
		if(phdr->p_type == PT_LOAD
		|| phdr->p_type == PT_PHDR)
		{
			fseek(fElf, pElf->hdr.e_phoff + (i*pElf->hdr.e_phentsize), SEEK_SET);
			fread(phdr, pElf->hdr.e_phentsize, 1, fElf);

			if(phdr->p_vaddr == 0
			|| phdr->p_memsz == 0 )
				continue;

			pMem = CalculateVirtualAddress(phdr, pBaseAddr);
			protection = ElfProtectionToWin32Protection(phdr->p_flags);
			if(!VirtualProtectEx(pElf->pinfo.hProcess, pMem, phdr->p_memsz, protection, &oldprot))
			{
				DWORD err = GetLastError();
				ktrace("error 0x%lx in protect program segment @ 0x%lx (got 0x%lx)\n", err,pWantMem, pMem);
				loadok = 0;
				break;
			}
		}
	}
	*/

	delete phdr;
	free(pMemTemp); //used 'realloc', not 'new'
	if(loadok)
		return 0;

	ktrace("program load failed\n");
	return -1;
}


/*
 * Load an ELF file into memory
 */
_declspec(dllexport) int LoadELFFile(PELF_Data pElf, const char * filename, bool LoadAsLibrary)
{

	FILE * fElf;
	int rc;

	ktrace("LoadELFFile %s\n", filename);

	Path p;
	p.SetUnixPath(filename);
	strncpy(pElf->filepath, p.Win32Path(), sizeof(pElf->filepath));

	fElf = fopen(pElf->filepath,"rb");
	if(fElf==NULL)
	{
		perror(filename);
		return 0;
	}


	// Read relavent headers
	rc=0;
	if(rc==0)
		rc = LoadELFHeader(fElf, pElf);
	if(rc==0)
		rc = LoadProgram(fElf, pElf, LoadAsLibrary);
	else
	{
		//may be a shell script - try that
		char buf[MAX_COMMANDLINE_LEN+MAX_PATH];
		fseek(fElf, 0, SEEK_SET);
		fread(buf, 1, sizeof(buf), fElf);
		if(strncmp(buf,"#!",2)==0)
		{
			//yes - it is a shell script with a header
			// eg: #!/bin/sh
			//Use that program as the process and give it the args we specified
			char * space = strchr(buf, ' ');
			char * nl = strchr(buf, 0x0a);
			if(nl==NULL)
			{
				buf[sizeof(buf)-1] = NULL;
				space = NULL;
			}
			else
			{
				*nl = NULL; //line ends here

				char * args;
				if(space==NULL || space>nl)
					args = NULL;
				else
					args = space+1;

				//skip #! and any whitespace
				char * interp = buf+2;
				while(*interp == ' ')
					interp++;

				rc = LoadELFFile(pElf, interp, LoadAsLibrary);

				if(args)
					strncpy(pElf->InterpreterCommandLine, args, MAX_COMMANDLINE_LEN);
			}
		}
	}
	

	fclose(fElf);
	return rc==0;
}

