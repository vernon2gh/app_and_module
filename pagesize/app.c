#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef DEBUG
#define pr_debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif


/*
 * Use the following command to count the consumption time of pagefaults for
 * different allocation methods:
 *
 * bpftrace -e 'uprobe:./a.out:test_common { @start[arg0]=nsecs; } uretprobe:./a.out:test_common /@start[retval]/{ @ns[func] = avg(nsecs - @start[retval]); delete(@start[retval]);}'
 */
char *test_common(char *buf)
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

	fd = open("file.txt", O_CREAT | O_RDWR, 0664);
	ftruncate(fd, 4096 * 16);

	buf = mmap(NULL, 4096 * 16, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FILE, fd, 0);
	test_common(buf);
	munmap(buf, 4096 * 16);

	close(fd);
	remove("file.txt");
}

int main(int argc, char *argv[])
{
	printf("PAGESIZE %d\n", getpagesize());

	while (1) {
		test_malloc();
		test_mmap_anon();
		test_mmap_file();
	}

	return 0;
}
