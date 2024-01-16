#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "dynamic_debug.h"
#include "trace.h"

static int pagesize;

void swap_normal(void)
{
	char *buf;

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	buf[0] = 't';

	dynamic_debug_control("file madvise.c +p");
	trace_on();
	madvise(buf, pagesize, MADV_PAGEOUT);
	trace_off();
	dynamic_debug_control("file madvise.c -p");

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

	dynamic_debug_control("file madvise.c +p");
	trace_on();
	madvise(buf, pagesize * pagenum, MADV_PAGEOUT);
	trace_off();
	dynamic_debug_control("file madvise.c -p");

	munmap(buf, pagesize * pagenum);
}

void swap_thp(void)
{
	__swap_thp((void *)0x7f987f200000, 512);
}

void swap_multi_size_thp(void)
{
	__swap_thp((void *)0x7f9877ff0000, 16);
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();
	trace_configure(getpid(), "madvise_cold_or_pageout_pte_range");
	pagesize = getpagesize();

	if (!argv[1])
		goto DEFAULT;

	if (!strncmp(argv[1], "thp", strlen("thp")))
		swap_thp();
	else if (!strncmp(argv[1], "mthp", strlen("mthp")))
		swap_multi_size_thp();
	else
DEFAULT:	swap_normal();

	trace_exit();
	dynamic_debug_end();

	return 0;
}
