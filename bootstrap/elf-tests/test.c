/* a simple C testbed with NO c runtime to test api calls */

int _start()
{
	write(1,"Hello\n",6);
	_exit(3);
}
