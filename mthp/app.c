#define _GNU_SOURCE
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

enum demo_entry {
	DEMO_MMAP_ANON_PRIVATE_WRITE,
	DEMO_SHM_ANON_WRITE,
	DEMO_SHM_POSIX_WRITE,
	DEMO_SHM_SYSTEMV_WRITE,
	DEMO_KHUGEPAGED,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"mmap_anon_private_write",
	"shm_anon_write",
	"shm_posix_write",
	"shm_systemv_write",
	"khugepaged",
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

static void test_mmap_syscall(void)
{
	int size = 1024 * 1024 * 1024; // 1G
	char *buf;

	buf = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	memset(buf, 'A', size);

	munmap(buf, size);
}

static void test_shm_anon(void)
{
	int size = 1024 * 1024 * 1024; // 1G
	char *buf;
	int fd;

	fd = memfd_create("shm_anon", 0);
	ftruncate(fd, size);

	buf = mmap((void *)0x7fff00000000, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memset(buf, 'A', size);

	munmap(buf, size);
	close(fd);
}

static void test_shm_posix(void)
{
	int size = 1024 * 1024 * 1024; // 1G
	char *buf;
	int fd;

	fd = shm_open("shm_posix", O_RDWR | O_CREAT, 0777);
	ftruncate(fd, size);

	buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memset(buf, 'A', size);

	munmap(buf, size);
	shm_unlink("shm_posix");
}

static void test_shm_systemv(void)
{
	int size = 1024 * 1024 * 1024; // 1G
	int shmid;
	char *buf;

	shmid = shmget(ftok("shm_systemv", 0), size, IPC_CREAT|0666);

	buf = shmat(shmid, NULL, 0);
	//madvise(buf, size, MADV_HUGEPAGE);
	memset(buf, 'A', size);

	shmdt(buf);
	shmctl(shmid, IPC_RMID, NULL);
}

static void test_khugepaged(void)
{
	int size = 1024 * 1024 * 1024; // 1G
	char *buf;

	buf = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	memset(buf, 'A', size);

	madvise(buf, size, MADV_HUGEPAGE);
	sleep(3);

	memset(buf, 'B', size);
	memset(buf, 'C', size);
	memset(buf, 'D', size);
	memset(buf, 'E', size);
	memset(buf, 'F', size);

	munmap(buf, size);
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();

	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_MMAP_ANON_PRIVATE_WRITE:
			test_mmap_syscall();
			break;
		case DEMO_SHM_ANON_WRITE:
			test_shm_anon();
			break;
		case DEMO_SHM_POSIX_WRITE:
			test_shm_posix();
			break;
		case DEMO_SHM_SYSTEMV_WRITE:
			test_shm_systemv();
			break;
		case DEMO_KHUGEPAGED:
			test_khugepaged();
			break;
	}

	dynamic_debug_end();

	return 0;
}
