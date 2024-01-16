#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "dynamic_debug.h"

static int pagesize;

void pagefault_normal(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	dynamic_debug_control("file memory.c +p");
	tmp = *buf;
	*buf = 0x12;
	dynamic_debug_control("file memory.c -p");

	munmap(buf, pagesize);
}

static void __pagefault_thp(void *start_vaddr, int pagenum)
{
	char *buf;
	int i;

	buf = mmap(start_vaddr, pagesize * pagenum,
		   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	dynamic_debug_control("file *memory.c +p");
	for (i = 0; i < pagenum; i++)
		buf[pagesize * i] = 0x12;
	dynamic_debug_control("file *memory.c -p");

	munmap(buf, pagesize * pagenum);
}

void pagefault_thp(void)
{
	__pagefault_thp((void *)0x7f987f200000, 512);
}

void pagefault_multi_size_thp(void)
{
	__pagefault_thp((void *)0x7f9877ff0000, 16);
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();
	pagesize = getpagesize();

	if (!argv[1])
		goto DEFAULT;

	if (!strncmp(argv[1], "thp", strlen("thp")))
		pagefault_thp();
	else if (!strncmp(argv[1], "mthp", strlen("mthp")))
		pagefault_multi_size_thp();
	else
DEFAULT:	pagefault_normal();

	dynamic_debug_end();

	return 0;
}
