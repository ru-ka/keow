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

#ifndef KEOW_CONSOLE_TERMDATA_H
#define KEOW_CONSOLE_TERMDATA_H

#include "linux_includes.h"


extern HANDLE g_hKernelTextOutput;
extern HANDLE g_hKernelTextInput;
extern HANDLE g_hStdIn, g_hStdOut;


extern DWORD			g_InputState; //for shared escape code handling
extern DWORD			g_OutputState; //for shared escape code handling
extern BYTE			g_InputStateData[32];
extern BYTE			g_OutputStateData[32];

#endif // KEOW_CONSOLE_TERMDATA_H
