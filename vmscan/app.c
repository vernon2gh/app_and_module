#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "dynamic_debug.h"

#define PAGESIZE	4096

#define print_meminfo(format, ...)				\
do {								\
	printf(format, ##__VA_ARGS__);				\
	system("cat /proc/meminfo | grep -E \"file|anon\"");	\
} while(0)

static inline int get_file_pages(int fd)
{
	struct stat st;

	if(fstat(fd, &st)) {
		printf("Get file size failed.\n");
		return -1;
	}

	return st.st_size / PAGESIZE;
}

static void test_promote_active_list_from_read_syscall(void)
{
	char buf[PAGESIZE];
	int nr_pages;
	int fd;
	int ret;
	int i;

	fd = open("testfile", O_RDONLY);
	nr_pages = get_file_pages(fd);

	for (i = 0; i < nr_pages; i++) {
		ret = read(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Read file failed.\n");
			break;
		}
	}
	print_meminfo("meminfo after first read\n");

	lseek(fd, 0, SEEK_SET);
	for (i = 0; i < nr_pages; i++) {
		ret = read(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Read file failed.\n");
			break;
		}
	}
	print_meminfo("meminfo after second read\n");

	close(fd);
}

static void test_promote_active_list_from_write_syscall(void)
{
	char buf[PAGESIZE] = { 0 };
	int nr_pages;
	int fd;
	int ret;
	int i;

	fd = open("testfile", O_WRONLY);
	nr_pages = get_file_pages(fd);

	for (i = 0; i < nr_pages; i++) {
		ret = write(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Write file failed.\n");
			break;
		}
	}
	print_meminfo("meminfo after first write\n");

	lseek(fd, 0, SEEK_SET);
	for (i = 0; i < nr_pages; i++) {
		ret = write(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Write file failed.\n");
			break;
		}
	}
	print_meminfo("meminfo after second write\n");

	close(fd);
}

static void test_promote_active_list_from_mmap_syscall(bool share, bool write)
{
	char *buf;
	int flags;
	int fd;
	int nr_pages, i;
	char tmp;

	fd = open("testfile", O_RDWR);
	nr_pages = get_file_pages(fd);

	if (share)
		flags = MAP_FILE | MAP_SHARED;
	else
		flags = MAP_FILE | MAP_PRIVATE;

	buf = mmap(0, nr_pages * PAGESIZE, PROT_READ | PROT_WRITE, flags, fd, 0);

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'a';
		else
			tmp = buf[i * PAGESIZE];
	}
	print_meminfo("meminfo after first %s %s\n", share ? "share" : "private",
							write ? "write" : "read");

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'b';
		else
			tmp = buf[i * PAGESIZE];
	}
	print_meminfo("meminfo after first %s %s\n", share ? "share" : "private",
							write ? "write" : "read");

	dynamic_debug_control("file memory.c +p");
	munmap(buf, nr_pages * PAGESIZE);
	dynamic_debug_control("file memory.c -p");
	print_meminfo("meminfo after munmap\n");

	close(fd);
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();

	if (!argv[1]) {
		printf("Empty options to execute default read() SYSCALL.\n");
		goto DEFAULT;
	}

	if (!strncmp(argv[1], "read", strlen("read")))
DEFAULT:	test_promote_active_list_from_read_syscall();
	else if (!strncmp(argv[1], "write", strlen("write")))
		test_promote_active_list_from_write_syscall();
	else if (!strncmp(argv[1], "mmap_file_share_read", strlen("mmap_file_share_read")))
		test_promote_active_list_from_mmap_syscall(1, 0);
	else if (!strncmp(argv[1], "mmap_file_share_write", strlen("mmap_file_share_write")))
		test_promote_active_list_from_mmap_syscall(1, 1);
	else if (!strncmp(argv[1], "mmap_file_private_read", strlen("mmap_file_private_read")))
		test_promote_active_list_from_mmap_syscall(0, 0);
	else if (!strncmp(argv[1], "mmap_file_private_write", strlen("mmap_file_private_write")))
		test_promote_active_list_from_mmap_syscall(0, 1);
	else {
		printf("Not exit options to execute default read() SYSCALL.\n");
		goto DEFAULT;
	}

	dynamic_debug_end();

	return 0;
}
