#ifndef LINUX_INCLUDES_H
#define LINUX_INCLUDES_H

// include windows here so we catch #define redefinitions
// in linux includes and then we can fix them

//for correct exception CONTEXT struct (?) for windows includes
#define _X86_

#include <windows.h>
#include <shlwapi.h>

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

#pragma pack(pop)

#include <strsafe.h>

#endif // LINUX_INCLUDES_H
