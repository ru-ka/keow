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

// Terminal.h: interface for the Terminal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TERMINAL_H__8DA03BC5_F1B5_4DF2_B7C7_4F92F9C1B8C2__INCLUDED_)
#define AFX_TERMINAL_H__8DA03BC5_F1B5_4DF2_B7C7_4F92F9C1B8C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "kernel.h"


//
// info needed to be kept for terminal devices
//

class Terminal  
{
public:
	void printf(const char * format, ...);

	Terminal();
	virtual ~Terminal();

	//What process is running the terminal (eg KeowConsole)
	DWORD	dwTerminalProcess;
	//Need read/write handles for the terminal
	HANDLE	hTerminalRead, hTerminalWrite;


	//Linux process information for the terminal
	linux::termios	TermIOs;
	DWORD			ProcessGroup;

};

#endif // !defined(AFX_TERMINAL_H__8DA03BC5_F1B5_4DF2_B7C7_4F92F9C1B8C2__INCLUDED_)
