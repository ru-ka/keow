/* a simple C testbed with NO c runtime to test api calls */
#include <unistd.h>

int _start()
{
	write(1,"Hello\n",6);
	_exit(3);
}
