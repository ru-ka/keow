#ifndef LOADELF_H
#define LOADELF_H

#include "linux_includes.h"
typedef unsigned char* ADDR;

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
};
typedef struct ELF_Data * PELF_Data;


_declspec(dllexport) int LoadELFFile(PELF_Data pElf, const char * filename);


#endif //LOADELF_H
