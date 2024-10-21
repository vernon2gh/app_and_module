#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef DEBUG
#define pr_debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

enum demo_entry {
	DEMO_MALLOC,
	DEMO_MMAP_ANON,
	DEMO_MMAP_FILE,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"malloc",
	"mmap_anon",
	"mmap_file",
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

/*
 * Use the following command to count the consumption time of pagefaults for
 * different allocation methods:
 *
 * bpftrace -e 'uprobe:./a.out:test_common { @start[arg0]=nsecs; } uretprobe:./a.out:test_common /@start[retval]/{ @ns[func] = avg(nsecs - @start[retval]); delete(@start[retval]);}'
 */
static char *test_common(char *buf)
{
	char i;

	for (i = 0; i < 16; i++) {
		buf[4096 * i] = i;
		pr_debug("buf[4096 * %d] = %d\n", i, buf[4096 * i]);
	}

	return buf;
}

void test_malloc(void)
{
	char *buf;

	buf = malloc(4096 * 16);
	test_common(buf);
	free(buf);
}

void test_mmap_anon(void)
{
	char *buf;

	buf = mmap(NULL, 4096 * 16, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	test_common(buf);
	munmap(buf, 4096 * 16);
}

void test_mmap_file(void)
{
	char *buf;
	int fd;

	fd = open("testfile", O_CREAT | O_RDWR, 0664);
	ftruncate(fd, 4096 * 16);

	buf = mmap(NULL, 4096 * 16, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FILE, fd, 0);
	test_common(buf);
	munmap(buf, 4096 * 16);

	close(fd);
	remove("testfile");
}

int main(int argc, char *argv[])
{
	printf("PAGESIZE %d\n", getpagesize());

	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_MALLOC:
			test_malloc();
			break;
		case DEMO_MMAP_ANON:
			test_mmap_anon();
			break;
		case DEMO_MMAP_FILE:
			test_mmap_file();
			break;
	}

	return 0;
}
