#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static int pagesize;

static int fd;
static char enbuf[] = "file memory.c +p";
static char debuf[] = "file memory.c -p";

void pagefault_normal(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	write(fd, enbuf, sizeof(enbuf));
	tmp = *buf;
	*buf = 0x12;
	write(fd, debuf, sizeof(debuf));

	munmap(buf, pagesize);
}

void pagefault_small_size_thp(void)
{
	char *buf;
	int i;
#define PAGE_NUM 16

	buf = mmap((void *)0x7f9877ff0000, pagesize * PAGE_NUM,
		   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	write(fd, enbuf, sizeof(enbuf));
	for (i = 0; i < PAGE_NUM; i++)
		buf[pagesize * i] = 0x12;
	write(fd, debuf, sizeof(debuf));

	munmap(buf, pagesize * PAGE_NUM);
}

int main(int argc, char *argv[])
{
	pagesize = getpagesize();

	fd = open("/sys/kernel/debug/dynamic_debug/control", O_RDWR);

	pagefault_normal();
	pagefault_small_size_thp();

	close(fd);

	return 0;
}
