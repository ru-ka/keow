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

// SocketCalls.h: interface for the SocketCalls class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOCKETCALLS_H__BEFB0766_48E6_4C88_A8C1_74FD826CD1DD__INCLUDED_)
#define AFX_SOCKETCALLS_H__BEFB0766_48E6_4C88_A8C1_74FD826CD1DD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class SocketCalls  
{
public:
	static void sys_socketcall(CONTEXT &ctx);

	static int sys_socket(int domain, int type, int protocol);
	static int sys_bind(int sockfd, linux::sockaddr *my_addr, linux::socklen_t addrlen);
	static int sys_connect(int  sockfd,  const  struct sockaddr *serv_addr, socklen_t addrlen);
	static int sys_listen(a0,a1);
	static int sys_accept(a0,(struct sockaddr __user *)a1, (static int __user *)a[2]);
	static int sys_getsockname(a0,(struct sockaddr __user *)a1, (static int __user *)a[2]);
	static int sys_getpeername(a0, (struct sockaddr __user *)a1, (static int __user *)a[2]);
	static int sys_socketpair(a0,a1, a[2], (static int __user *)a[3]);
	static int sys_send(a0, (void __user *)a1, a[2], a[3]);
	static int sys_sendto(a0,(void __user *)a1, a[2], a[3], (struct sockaddr __user *)a[4], a[5]);
	static int sys_recv(a0, (void __user *)a1, a[2], a[3]);
	static int sys_recvfrom(a0, (void __user *)a1, a[2], a[3], (struct sockaddr __user *)a[4], (static int __user *)a[5]);
	static int sys_shutdown(a0,a1);
	static int sys_setsockopt(a0, a1, a[2], (char __user *)a[3], a[4]);
	static int sys_getsockopt(a0, a1, a[2], (char __user *)a[3], (static int __user *)a[4]);
	static int sys_sendmsg(a0, (struct msghdr __user *) a1, a[2]);
	static int sys_recvmsg(a0, (struct msghdr __user *) a1, a[2]);

private:
	SocketCalls() {}
};

#endif // !defined(AFX_SOCKETCALLS_H__BEFB0766_48E6_4C88_A8C1_74FD826CD1DD__INCLUDED_)
