#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define PAGESIZE	4096

static int get_file_pages(int fd)
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

	fd = open("testfile", O_RDONLY);
	nr_pages = get_file_pages(fd);
	while (nr_pages--) {
		ret = read(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Read file failed.\n");
			break;
		}
	}
	close(fd);
}

static void test_promote_active_list_from_write_syscall(void)
{
	char buf[PAGESIZE] = { 0 };
	int nr_pages;
	int fd;
	int ret;

	fd = open("testfile", O_WRONLY);
	nr_pages = get_file_pages(fd);
	while (nr_pages--) {
		ret = write(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Write file failed.\n");
			break;
		}
	}
	close(fd);
}

static void test_promote_active_list_from_mmap_syscall(bool write)
{
	char *buf;
	char tmp;
	int fd;
	int nr_pages, i;

	fd = open("testfile", O_RDWR);
	nr_pages = get_file_pages(fd);
	buf = mmap(0, nr_pages * PAGESIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FILE, fd, 0);

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'a';
		else
			tmp = buf[i * PAGESIZE];
	}

	munmap(buf, nr_pages * PAGESIZE);
	close(fd);
}

int main(int argc, char *argv[])
{
	if (!argv[1])
		goto DEFAULT;

	if (!strncmp(argv[1], "read", strlen("read")))
DEFAULT:	test_promote_active_list_from_read_syscall();
	else if (!strncmp(argv[1], "write", strlen("write")))
		test_promote_active_list_from_write_syscall();
	else if (!strncmp(argv[1], "mmap_read", strlen("mmap_read")))
		test_promote_active_list_from_mmap_syscall(0);
	else if (!strncmp(argv[1], "mmap_write", strlen("mmap_write")))
		test_promote_active_list_from_mmap_syscall(1);
	else
		goto DEFAULT;

	return 0;
}
