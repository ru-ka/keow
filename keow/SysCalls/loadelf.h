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

#ifndef LOADELF_H
#define LOADELF_H

#include "linux_includes.h"
typedef unsigned char* ADDR;

#define MAX_COMMANDLINE_LEN 1024

struct ELF_Data 
{
	struct linux::elf32_hdr hdr; //ELF header
	PROCESS_INFORMATION pinfo;
	STARTUPINFO sinfo;
	char filepath[MAX_PATH];
	ADDR phdr_addr;
	ADDR start_addr;
	ADDR program_base, program_max;
	ADDR interpreter_base; //, interpreter_max; is last_lib_addr
	ADDR interpreter_start;
	ADDR bss_start, brk;
	ADDR last_lib_addr;
	char InterpreterCommandLine[MAX_COMMANDLINE_LEN];
};
typedef struct ELF_Data * PELF_Data;


_declspec(dllexport) int LoadELFFile(PELF_Data pElf, const char * filename, bool LoadAsLibrary);


#endif //LOADELF_H
