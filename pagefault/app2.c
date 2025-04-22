#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "dynamic_debug.h"

#define PAGESIZE	4096
#define SHMSIZE		(100 * 1024 * 1024)

#define print_meminfo(format, ...)				\
do {								\
	printf(format, ##__VA_ARGS__);				\
	system("cat /proc/meminfo | grep -E \"file|anon\"");	\
} while(0)

enum demo_entry {
	DEMO_READ,
	DEMO_WRITE,
	DEMO_MMAP_FILE_SHARE_READ,
	DEMO_MMAP_FILE_SHARE_WRITE,
	DEMO_MMAP_FILE_PRIVATE_READ,
	DEMO_MMAP_FILE_PRIVATE_WRITE,
	DEMO_MMAP_ANON_SHARE_READ,
	DEMO_MMAP_ANON_SHARE_WRITE,
	DEMO_MMAP_ANON_PRIVATE_READ,
	DEMO_MMAP_ANON_PRIVATE_WRITE,
	DEMO_SHM_SYSTEMV_READ,
	DEMO_SHM_SYSTEMV_WRITE,
	DEMO_SHM_POSIX_READ,
	DEMO_SHM_POSIX_WRITE,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"read",
	"write",
	"mmap_file_share_read",
	"mmap_file_share_write",
	"mmap_file_private_read",
	"mmap_file_private_write",
	"mmap_anon_share_read",
	"mmap_anon_share_write",
	"mmap_anon_private_read",
	"mmap_anon_private_write",
	"shm_systemv_read",
	"shm_systemv_write",
	"shm_posix_read",
	"shm_posix_write",
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

static void test_read_syscall(void)
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
	print_meminfo("meminfo after first read syscall\n");

	lseek(fd, 0, SEEK_SET);
	for (i = 0; i < nr_pages; i++) {
		ret = read(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Read file failed.\n");
			break;
		}
	}
	print_meminfo("meminfo after second read syscall\n");

	close(fd);
}

static void test_write_syscall(void)
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
	print_meminfo("meminfo after first write syscall\n");

	lseek(fd, 0, SEEK_SET);
	for (i = 0; i < nr_pages; i++) {
		ret = write(fd, buf, PAGESIZE);
		if (ret == -1) {
			printf("Write file failed.\n");
			break;
		}
	}
	print_meminfo("meminfo after second write syscall\n");

	close(fd);
}

static void test_mmap_syscall(int flags, bool write)
{
	bool file = !(flags & MAP_ANON);
	bool share = !!(flags & MAP_SHARED);
	int fd, nr_pages, i;
	char *buf;
	char tmp;

	system("cp /proc/meminfo origin_meminfo.txt");
	system("cp /proc/vmstat  origin_vmstat.txt");

	if (file) {
		fd = open("testfile", O_RDWR);
		nr_pages = get_file_pages(fd);
	} else {
		fd = -1;
		nr_pages = 100 * 1024 * 1024 / PAGESIZE;
	}

	buf = mmap(0, nr_pages * PAGESIZE, PROT_READ | PROT_WRITE, flags, fd, 0);

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'a';
		else
			tmp = buf[i * PAGESIZE];
	}
	system("cp /proc/meminfo access_meminfo.txt");
	system("cp /proc/vmstat  access_vmstat.txt");

	munmap(buf, nr_pages * PAGESIZE);
	system("cp /proc/meminfo munmap_meminfo.txt");
	system("cp /proc/vmstat  munmap_vmstat.txt");

	if (file)
		close(fd);
}

static void test_shm_systemv(bool write)
{
	int nr_pages = SHMSIZE / PAGESIZE;
 	int shmid, i;
	char *buf;
	char tmp;

	shmid = shmget(ftok("shmtest1", 0), SHMSIZE, IPC_CREAT|0666);

	buf = shmat(shmid, NULL, 0);

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'a';
		else
			tmp = buf[i * PAGESIZE];
	}
	print_meminfo("meminfo after first %s from systemv shm syscall\n",
			write ? "write" : "read");

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'b';
		else
			tmp = buf[i * PAGESIZE];
	}
	print_meminfo("meminfo after second %s from systemv shm syscall\n",
			write ? "write" : "read");

	shmdt(buf);
	print_meminfo("meminfo after shmdt syscall\n");

	// shmctl(shmid, IPC_RMID, NULL);
}

static void test_shm_posix(bool write)
{
	int nr_pages = SHMSIZE / PAGESIZE;
	int fd, i;
	char *buf;
	char tmp;

	fd = shm_open("shmtest2", O_RDWR | O_CREAT, 0777);
	ftruncate(fd, SHMSIZE);

	buf = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'a';
		else
			tmp = buf[i * PAGESIZE];
	}
	print_meminfo("meminfo after first %s from posix shm syscall\n",
			write ? "write" : "read");

	for (i = 0; i < nr_pages; i++) {
		if (write)
			buf[i * PAGESIZE] = 'b';
		else
			tmp = buf[i * PAGESIZE];
	}
	print_meminfo("meminfo after second %s from posix shm syscall\n",
			write ? "write" : "read");

	munmap(buf, SHMSIZE);
	print_meminfo("meminfo after munmap syscall\n");

	// shm_unlink("shmtest2");
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();

	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_READ:
			test_read_syscall();
			break;
		case DEMO_WRITE:
			test_write_syscall();
			break;
		case DEMO_MMAP_FILE_SHARE_READ:
			test_mmap_syscall(MAP_FILE | MAP_SHARED, 0);
			break;
		case DEMO_MMAP_FILE_SHARE_WRITE:
			test_mmap_syscall(MAP_FILE | MAP_SHARED, 1);
			break;
		case DEMO_MMAP_FILE_PRIVATE_READ:
			test_mmap_syscall(MAP_FILE | MAP_PRIVATE, 0);
			break;
		case DEMO_MMAP_FILE_PRIVATE_WRITE:
			test_mmap_syscall(MAP_FILE | MAP_PRIVATE, 1);
			break;
		case DEMO_MMAP_ANON_SHARE_READ:
			test_mmap_syscall(MAP_ANON | MAP_SHARED, 0);
			break;
		case DEMO_MMAP_ANON_SHARE_WRITE:
			test_mmap_syscall(MAP_ANON | MAP_SHARED, 1);
			break;
		case DEMO_MMAP_ANON_PRIVATE_READ:
			test_mmap_syscall(MAP_ANON | MAP_PRIVATE, 0);
			break;
		case DEMO_MMAP_ANON_PRIVATE_WRITE:
			test_mmap_syscall(MAP_ANON | MAP_PRIVATE, 1);
			break;
		case DEMO_SHM_SYSTEMV_READ:
			test_shm_systemv(0);
			break;
		case DEMO_SHM_SYSTEMV_WRITE:
			test_shm_systemv(1);
			break;
		case DEMO_SHM_POSIX_READ:
			test_shm_posix(0);
			break;
		case DEMO_SHM_POSIX_WRITE:
			test_shm_posix(1);
			break;
	}

	dynamic_debug_end();

	return 0;
}
