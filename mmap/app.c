#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dynamic_debug.h"
#include "trace.h"

static int pagesize;

enum demo_entry {
	DEMO_PRIVATE_ANON,
	DEMO_SHARE_ANON,
	DEMO_PRIVATE_FILE,
	DEMO_SHARE_FILE,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"private_anon",
	"share_anon",
	"private_file",
	"share_file",
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

	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_PRIVATE_ANON:
			test_private_anon();
			break;
		case DEMO_SHARE_ANON:
			test_share_anon();
			break;
		case DEMO_PRIVATE_FILE:
			test_private_file();
			break;
		case DEMO_SHARE_FILE:
			test_share_file();
			break;
	}

	trace_exit();
	dynamic_debug_end();

	return 0;
}
