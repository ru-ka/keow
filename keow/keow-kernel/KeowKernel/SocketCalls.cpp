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

// SocketCalls.cpp: implementation of the SocketCalls class.
//
//////////////////////////////////////////////////////////////////////

#include "includes.h"
#include "SocketCalls.h"


//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// This section was cobbled from the linux kernel net/socket.c function sys_socketcall

/* Argument list sizes for sys_socketcall */
#define AL(x) ((x) * sizeof(unsigned long))
static unsigned char nargs[18]={AL(0),AL(3),AL(3),AL(3),AL(2),AL(3),
				AL(3),AL(3),AL(4),AL(4),AL(4),AL(6),
				AL(6),AL(2),AL(5),AL(5),AL(3),AL(3)};
#undef AL

void SocketCalls::sys_socketcall(CONTEXT &ctx)
{
	unsigned long a[6];
	unsigned long a0,a1;
	int err;

	DWORD call = ctx.Ebx;

	if(call<1||call>linux::SYS_RECVMSG) {
		ctx.Eax=-linux::EINVAL;
		return;
	}

	//get all the possible arguments
	if(!P->ReadMemory(a, (ADDR)ctx.Ecx, nargs[call])) {
		ctx.Eax=-linux::EFAULT;
		return;
	}
		
	a0=a[0];
	a1=a[1];
	
	switch(call) 
	{
		case linux::SYS_SOCKET:
			err = sys_socket(a0,a1,a[2]);
			break;
		case linux::SYS_BIND:
			err = sys_bind(a0,(struct sockaddr __user *)a1, a[2]);
			break;
		case linux::SYS_CONNECT:
			err = sys_connect(a0, (struct sockaddr __user *)a1, a[2]);
			break;
		case linux::SYS_LISTEN:
			err = sys_listen(a0,a1);
			break;
		case linux::SYS_ACCEPT:
			err = sys_accept(a0,(struct sockaddr __user *)a1, (int __user *)a[2]);
			break;
		case linux::SYS_GETSOCKNAME:
			err = sys_getsockname(a0,(struct sockaddr __user *)a1, (int __user *)a[2]);
			break;
		case linux::SYS_GETPEERNAME:
			err = sys_getpeername(a0, (struct sockaddr __user *)a1, (int __user *)a[2]);
			break;
		case linux::SYS_SOCKETPAIR:
			err = sys_socketpair(a0,a1, a[2], (int __user *)a[3]);
			break;
		case linux::SYS_SEND:
			err = sys_send(a0, (void __user *)a1, a[2], a[3]);
			break;
		case linux::SYS_SENDTO:
			err = sys_sendto(a0,(void __user *)a1, a[2], a[3],
					 (struct sockaddr __user *)a[4], a[5]);
			break;
		case linux::SYS_RECV:
			err = sys_recv(a0, (void __user *)a1, a[2], a[3]);
			break;
		case linux::SYS_RECVFROM:
			err = sys_recvfrom(a0, (void __user *)a1, a[2], a[3],
					   (struct sockaddr __user *)a[4], (int __user *)a[5]);
			break;
		case linux::SYS_SHUTDOWN:
			err = sys_shutdown(a0,a1);
			break;
		case linux::SYS_SETSOCKOPT:
			err = sys_setsockopt(a0, a1, a[2], (char __user *)a[3], a[4]);
			break;
		case linux::SYS_GETSOCKOPT:
			err = sys_getsockopt(a0, a1, a[2], (char __user *)a[3], (int __user *)a[4]);
			break;
		case linux::SYS_SENDMSG:
			err = sys_sendmsg(a0, (struct msghdr __user *) a1, a[2]);
			break;
		case linux::SYS_RECVMSG:
			err = sys_recvmsg(a0, (struct msghdr __user *) a1, a[2]);
			break;
		default:
			err = -linux::EINVAL;
			break;
	}
	ctx.Eax = err;
}

//////////////////////////////////////////////////////////////////////
