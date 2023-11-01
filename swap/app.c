#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define SHM_NAME "sync_status"

static int pagesize;
static char *buf;

static inline void sync_start(void)
{
	int fd;

	fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0777);
	ftruncate(fd, pagesize);

	buf = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

static inline void sync_end(void)
{
	munmap(buf, pagesize);
	shm_unlink(SHM_NAME);
}

static void wait_shell_finish(void)
{
	memcpy(buf, "app2shell", 9);

	while (strncmp(buf, "shell2app", 9))
		sleep(1);
}

void swap_normal(void)
{
	char *buf;

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	buf[0] = 't';

	wait_shell_finish();
	madvise(buf, pagesize, MADV_PAGEOUT);
	wait_shell_finish();

	munmap(buf, pagesize);
}

static void __swap_thp(void *start_vaddr, int pagenum)
{
	char *buf;
	int i;

	buf = mmap(start_vaddr, pagesize * pagenum,
		   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	for (i = 0; i < pagenum; i++)
		buf[pagesize * i] = 0x12;

	wait_shell_finish();
	madvise(buf, pagesize * pagenum, MADV_PAGEOUT);
	wait_shell_finish();

	munmap(buf, pagesize * pagenum);
}

void swap_thp(void)
{
	__swap_thp((void *)0x7f987f200000, 512);
}

void swap_small_size_thp(void)
{
	__swap_thp((void *)0x7f9877ff0000, 16);
}

int main(int argc, char *argv[])
{
	pagesize = getpagesize();
	sync_start();

	swap_normal();
	swap_thp();
	swap_small_size_thp();

	sync_end();

	return 0;
}
