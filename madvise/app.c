#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "dynamic_debug.h"

#define PAGESIZE	4096

enum demo_entry {
	DEMO_SHARELIB_EXE,
	DEMO_SHARELIB_NOEXE,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"sharelib_exe",
	"sharelib_noexe",
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

static inline int get_file_pages(int fd)
{
	struct stat st;

	if(fstat(fd, &st)) {
		printf("Get file size failed.\n");
		return -1;
	}

	return st.st_size / PAGESIZE;
}

static void test_sharelib(bool exe)
{
	char *filepath;
	int fd, nr_pages, i;
	char *buf;
	char tmp;

	if (exe)
		filepath = "/usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so";
	else
		filepath = "/usr/lib/x86_64-linux-gnu/audit/sotruss-lib.so";

	fd = open(filepath, O_RDONLY);
	nr_pages = get_file_pages(fd);

	buf = mmap(0, nr_pages * PAGESIZE, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
	for (i = 0; i < nr_pages; i++)
		tmp = buf[i * PAGESIZE];

	dynamic_debug_control("file madvise.c +p");
	dynamic_debug_control("file vmscan.c +p");
	madvise(buf, nr_pages * PAGESIZE, MADV_PAGEOUT);
	dynamic_debug_control("file vmscan.c -p");
	dynamic_debug_control("file madvise.c -p");

	munmap(buf, nr_pages * PAGESIZE);
	close(fd);
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();

	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_SHARELIB_EXE:
			test_sharelib(true);
			break;
		case DEMO_SHARELIB_NOEXE:
			test_sharelib(false);
			break;
	}

	dynamic_debug_end();

	return 0;
}
