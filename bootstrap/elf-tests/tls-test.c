/* a simple C testbed with NO c runtime to test api calls */

#include <unistd.h>
#include <linux/unistd.h>
#include <asm/ldt.h>


unsigned char buff[4096];
struct user_desc ud;

//nodefaultlibs, so need to supply the write() system call
int _write(int fd, char*s, int n)
{
    int nr = __NR_write;
    asm (
        "int $0x80 \n\t"
        :
        :"a"(nr), "b"(fd), "c"(s), "d"(n)
        );
    return 0;
}

int _syscall(int n, void*p)
{
    asm (
        "int $0x80 \n\t"
        :
        :"a"(n), "b"(p)
        );
    return 0;
}

void exit_n(int n) {
    _syscall(__NR_exit, (void*)n);
}


void print(char *s)
{
    int i=0;
    while(s[i]) ++i;
    _write(1,s,i);
}

void print_dec(long n)
{
   char buf[20];
   int i;

   if(n<0) {
     _write(1,"-",1);
     n = -n;
   }

   i=19;
   buf[i]=0;
   do {
     --i;
     buf[i] = '0' + (n%10);
     n /= 10;
   } while(n!=0);
   _write(1,&buf[i],20-i);
}

void print_hex(unsigned long n)
{
   char buf[20];
   int i;

   print("0x");

   i=19;
   buf[i]=0;
   do {
     --i;
     buf[i] = ((n&0xF)<=9 ? '0' : ('A'-10)) + (n&0xF);
     n>>=4;
   } while(n);
   _write(1,&buf[i],20-i);
}

int _start()
{
    int selector;
    int i;

	print("tls-test start\n");

    for(i=0; i<sizeof(buff); ++i) {
        buff[i] = i&0xFF;
    }
    print("buff ");
    print_hex((unsigned long)&buff);
    print("\n");


    //Setup TLS
	ud.entry_number = -1;
	ud.base_addr = (unsigned long)&buff;
	ud.limit = sizeof(buff);
	ud.seg_32bit = 1;
	ud.contents = 0;
	ud.read_exec_only = 0;
	ud.limit_in_pages = 0;
	ud.seg_not_present = 0;
	ud.useable = 1;
    //set_thread_area(&buff);
    i = _syscall(__NR_set_thread_area, &ud);
    if(i<0) {
	    print("error\n");
	    exit_n(2);
    }
    print("entry_num ");
    print_dec(ud.entry_number);
    print("\n");

    //assign to gs, and then use it
    selector = (ud.entry_number<<3) | 0x3;
    print("selector ");
    print_hex(selector);
    print("\n");
    asm (
        "movw %%ds, %%ax \n\t"
        "movw %%ax, %%fs \n\t"
        "movl %0, %%eax \n\t"
        "movw %%ax, %%gs \n\t"
        :
        :"r"(selector)
        :"%eax"
        );
    print("gs=");
    asm("movw %%gs, %%ax" :"=r"(i));
    print_hex(i);
    print("\n");


    //read [13]
    print("buff[13]=");
    print_hex(buff[13]);
    print(",  ");
    asm (
        "movl %%gs:13, %%eax \n\t"
        "movl %%eax, %0 \n\t"
        :"=r"(i)
        :"r"(selector)
        :"%eax"
        );
    print_hex(i);
    print("\n");

    //write [13]
    print("attempting write access\n");
    asm ("movb $0x91, %%gs:13" :);
    print("new buff[13]=");
    print_hex(buff[13]);
    print("\n");



	print("end\n");
	exit_n(0);

    asm ( "pop %gs \n\t ret $16 \n\t" );
}
