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

#include "includes.h"
#include "SysCalls.h"

// eax is the syscall number
// ebx,ecx,edx,esi,edi,ebp are up to 6(max) parameters
// any more parameters and the caller just puts a struct pointer in one of these
// eax is the return value


/*
 * we don't handle realtime signals
 * stub out here to remove IMPLEMENT... messages from ktrace
 */

void SysCalls::sys_rt_sigreturn(CONTEXT &ctx)		{ctx.Eax = -ENOSYS;}
void SysCalls::sys_rt_sigaction(CONTEXT &ctx)		{ctx.Eax = -ENOSYS;}
void SysCalls::sys_rt_sigprocmask(CONTEXT &ctx)		{ctx.Eax = -ENOSYS;}
void SysCalls::sys_rt_sigpending(CONTEXT &ctx)		{ctx.Eax = -ENOSYS;}
void SysCalls::sys_rt_sigtimedwait(CONTEXT &ctx)	{ctx.Eax = -ENOSYS;}
void SysCalls::sys_rt_sigqueueinfo(CONTEXT &ctx)	{ctx.Eax = -ENOSYS;}
void SysCalls::sys_rt_sigsuspend(CONTEXT &ctx)		{ctx.Eax = -ENOSYS;}
