#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dynamic_debug.h"
#include "trace.h"

static int pagesize;

static void test_private_anon(void)
{
	char *buf;
	char tmp;

	dynamic_debug_control("file mmap.c +p");
	trace_on();
	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	trace_off();
	dynamic_debug_control("file mmap.c -p");

	tmp = *buf;
	*buf = 0x1;

	munmap(buf, pagesize);
}

static void test_share_anon(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	tmp = *buf;
	*buf = 0x2;
	munmap(buf, pagesize);
}

static void test_private_file(void)
{
	char *buf;
	char tmp;
	int fd;

	fd = open("testfile", O_RDWR | O_CREAT, 0666);
	ftruncate(fd, pagesize);

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FILE, fd, 0);
	tmp = *buf;
	*buf = 'a';
	munmap(buf, pagesize);

	close(fd);
	remove("testfile");
}

static void test_share_file(void)
{
	char *buf;
	char tmp;
	int fd;

	fd = open("testfile", O_RDWR | O_CREAT, 0666);
	ftruncate(fd, pagesize);

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_FILE, fd, 0);
	tmp = *buf;
	*buf = 'b';
	munmap(buf, pagesize);

	close(fd);
	remove("testfile");
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();
	trace_configure(getpid(), "get_unmapped_area");
	pagesize = getpagesize();

	if (!argv[1])
		goto DEFAULT;

	if (!strncmp(argv[1], "private_anon", strlen("private_anon")))
		goto DEFAULT;
	else if (!strncmp(argv[1], "share_anon", strlen("share_anon")))
		test_share_anon();
	else if (!strncmp(argv[1], "private_file", strlen("private_file")))
		test_private_file();
	else if (!strncmp(argv[1], "share_file", strlen("share_file")))
		test_share_file();
	else
DEFAULT:	test_private_anon();

	trace_exit();
	dynamic_debug_end();

	return 0;
}
