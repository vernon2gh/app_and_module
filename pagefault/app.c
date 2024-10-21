#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "dynamic_debug.h"

static int pagesize;

enum demo_entry {
	DEMO_PRIVATE_ANON,
	DEMO_SHARE_ANON,
	DEMO_PRIVATE_FILE,
	DEMO_PRIVATE_FILE_RANDOM,
	DEMO_SHARE_FILE,
	DEMO_THP,
	DEMO_MTHP,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"private_anon",
	"share_anon",
	"private_file",
	"private_file_random",
	"share_file",
	"thp",
	"mthp",
};

static enum demo_entry test_demo_entry(char *entry)
{
	int i;

	if (!entry)
		return 0;

	for (i = 0; i < DEMO_ENTRY_MAX; i++) {
		if (!strncmp(entry, string[i], strlen(entry)))
			return i;
	}

	printf("Don't look for right demo entry.\n");
	return -EINVAL;
}

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

	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_PRIVATE_ANON:
			test_private_anon();
			break;
		case DEMO_SHARE_ANON:
			test_share_anon();
		case DEMO_PRIVATE_FILE:
			test_private_file(false);
		case DEMO_PRIVATE_FILE_RANDOM:
			test_private_file(true);
		case DEMO_SHARE_FILE:
			test_share_file();
		case DEMO_THP:
			test_thp();
		case DEMO_MTHP:
			test_multi_size_thp();
	}

	dynamic_debug_end();

	return 0;
}
