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

#ifndef LINUX_INCLUDES_H
#define LINUX_INCLUDES_H

#pragma pack(push,1)


//all vars in own namespace to ease co-existence with windows ones
namespace linux {

//get kernel stuff
#define __KERNEL__

//for use to skip things we don't want
#define __KERNEL_EMULATION_ON_WINDOWS__


//gcc specific stuff to alter
#define __signed__ signed
#define __inline__ inline
#define __const__ const

#define __extension__
#define __attribute__
#define __aligned__


#define __WORDSIZE 32

//usr/include/fcntl.h
/* Values for the second argument to access.
   These may be OR'd together.  */
#  define R_OK	4		/* Test for read permission.  */
#  define W_OK	2		/* Test for write permission.  */
#  define X_OK	1		/* Test for execute permission.  */
#  define F_OK	0		/* Test for existence.  */


typedef __int64 __s64;
typedef unsigned __int64 __u64;
typedef __int64	__kernel_loff_t;
typedef __kernel_loff_t loff_t;

struct mmap_arg_struct {
	unsigned long addr;
	unsigned long len;
	unsigned long prot;
	unsigned long flags;
	unsigned long fd;
	unsigned long offset;
};


#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/signal.h>
#include <linux/stat.h>
#include <linux/elf.h>
#include <linux/fcntl.h>
#include <linux/uio.h>
#include <linux/mman.h>
#include <linux/termios.h>
#include <linux/resource.h>
#include <linux/reboot.h>
#include <linux/sys.h>
#include <linux/dirent.h>
#include <linux/wait.h>
#include <asm-i386/ucontext.h>
#include <linux/user.h>

};



//bit manipulation helpers for fd_set etc
//based on linux kernel versions
inline void LINUX_FD_SET(int fd, linux::fd_set * pSet) {
	__asm {
		push ebx
		mov eax,fd
		mov ebx, pSet
		bts [ebx],eax
		pop ebx
	}
}
inline void LINUX_FD_CLR(int fd, linux::fd_set * pSet) {
	__asm {
		push ebx
		mov eax,fd
		mov ebx, pSet
		btr [ebx],eax
		pop ebx
	}
}
inline BYTE LINUX_FD_ISSET(int fd, linux::fd_set * pSet) {
	BYTE result;
	__asm {
		push ebx
		mov eax,fd
		mov ebx, pSet;
		bt [ebx],eax
		setb result
		pop ebx
	}
	return result;
}
inline void LINUX_FD_ZERO(linux::fd_set * pSet) {
	memset(pSet, 0, __FDSET_LONGS);
}
/*
#define __FD_SET(fd,fdsetp) \
		__asm__ __volatile__("btsl %1,%0": \
			"=m" (*(__kernel_fd_set *) (fdsetp)):"r" ((int) (fd)))

#define __FD_CLR(fd,fdsetp) \
		__asm__ __volatile__("btrl %1,%0": \
			"=m" (*(__kernel_fd_set *) (fdsetp)):"r" ((int) (fd)))

#define __FD_ISSET(fd,fdsetp) (__extension__ ({ \
		unsigned char __result; \
		__asm__ __volatile__("btl %1,%2 ; setb %0" \
			:"=q" (__result) :"r" ((int) (fd)), \
			"m" (*(__kernel_fd_set *) (fdsetp))); \
		__result; }))

#define __FD_ZERO(fdsetp) \
do { \
	int __d0, __d1; \
	__asm__ __volatile__("cld ; rep ; stosl" \
			:"=m" (*(__kernel_fd_set *) (fdsetp)), \
			  "=&c" (__d0), "=&D" (__d1) \
			:"a" (0), "1" (__FDSET_LONGS), \
			"2" ((__kernel_fd_set *) (fdsetp)) : "memory"); \
} while (0)
*/


#pragma pack(pop)

#endif // LINUX_INCLUDES_H
