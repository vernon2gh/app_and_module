#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

static int pagesize;
static int fd;

static inline void open_dynamic_debug(void)
{
	fd = open("/sys/kernel/debug/dynamic_debug/control", O_RDWR);
}

static inline void close_dynamic_debug(void)
{
	close(fd);
}

static void enable_dynamic_debug(char *debugfile)
{
	char buf[50];

	sprintf(buf, "file %s +p", debugfile);
	write(fd, buf, strlen(buf));
}

static void disable_dynamic_debug(char *debugfile)
{
	char buf[50];

	sprintf(buf, "file %s -p", debugfile);
	write(fd, buf, strlen(buf));
}

void pagefault_normal(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	enable_dynamic_debug("memory.c");
	tmp = *buf;
	*buf = 0x12;
	disable_dynamic_debug("memory.c");

	munmap(buf, pagesize);
}

static void __pagefault_thp(void *start_vaddr, int pagenum, char *debugfile)
{
	char *buf;
	int i;

	buf = mmap(start_vaddr, pagesize * pagenum,
		   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	enable_dynamic_debug(debugfile);
	for (i = 0; i < pagenum; i++)
		buf[pagesize * i] = 0x12;
	disable_dynamic_debug(debugfile);

	munmap(buf, pagesize * pagenum);
}

void pagefault_thp(void)
{
	__pagefault_thp((void *)0x7f987f200000, 512, "*memory.c");
}

void pagefault_small_size_thp(void)
{
	__pagefault_thp((void *)0x7f9877ff0000, 16, "memory.c");
}

int main(int argc, char *argv[])
{
	pagesize = getpagesize();
	open_dynamic_debug();

	pagefault_normal();
	pagefault_thp();
	pagefault_small_size_thp();

	close_dynamic_debug();

	return 0;
}
