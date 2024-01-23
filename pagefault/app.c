#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "dynamic_debug.h"

static int pagesize;

static void test_private_anon(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	dynamic_debug_control("file memory.c +p");
	tmp = *buf;
	*buf = 0x1;
	dynamic_debug_control("file memory.c -p");

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

static void test_private_file(bool random)
{
	char *buf;
	char tmp;
	int fd;

	fd = open("testfile", O_RDWR | O_CREAT, 0666);
	ftruncate(fd, pagesize);

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FILE, fd, 0);

	if (random)
		madvise(buf, pagesize, MADV_RANDOM);

	dynamic_debug_control("file filemap.c +p");
	tmp = *buf;
	*buf = 'a';
	dynamic_debug_control("file filemap.c -p");

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

static void __test_thp(void *start_vaddr, int pagenum)
{
	char *buf;
	int i;

	buf = mmap(start_vaddr, pagesize * pagenum,
		   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	dynamic_debug_control("file *memory.c +p");
	for (i = 0; i < pagenum; i++)
		buf[pagesize * i] = 0x12;
	dynamic_debug_control("file *memory.c -p");

	munmap(buf, pagesize * pagenum);
}

void test_thp(void)
{
	__test_thp((void *)0x7f987f200000, 512);
}

void test_multi_size_thp(void)
{
	__test_thp((void *)0x7f9877ff0000, 16);
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();
	pagesize = getpagesize();

	if (!argv[1])
		goto DEFAULT;

	if (!strncmp(argv[1], "private_anon", strlen("private_anon")))
		goto DEFAULT;
	else if (!strncmp(argv[1], "share_anon", strlen("share_anon")))
		test_share_anon();
	else if (!strncmp(argv[1], "private_file_random", strlen("private_file_random")))
		test_private_file(true);
	else if (!strncmp(argv[1], "private_file", strlen("private_file")))
		test_private_file(false);
	else if (!strncmp(argv[1], "share_file", strlen("share_file")))
		test_share_file();
	else if (!strncmp(argv[1], "thp", strlen("thp")))
		test_thp();
	else if (!strncmp(argv[1], "mthp", strlen("mthp")))
		test_multi_size_thp();
	else
DEFAULT:	test_private_anon();

	dynamic_debug_end();

	return 0;
}
