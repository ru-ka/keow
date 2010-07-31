
all: test tls-test


CFLAGS_TEST= -g -nostartfiles
test: test.c makefile
# make .S file
	cc -S $(CFLAGS_TEST) test.c
# make elf
	cc $(CFLAGS_TEST) test.s -o test


CFLAGS_TLS_TEST= -O0 -g -static -nostartfiles -nodefaultlibs -fno-stack-protector
tls-test: tls-test.c makefile
# make .S file
	gcc -S $(CFLAGS_TLS_TEST) tls-test.c
# make elf
	gcc $(CFLAGS_TLS_TEST) tls-test.s -o tls-test
