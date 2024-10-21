#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include "dynamic_debug.h"
#include "trace.h"

static int pagesize;

void swap_normal(void)
{
	char *buf;

	trace_configure(getpid(), "madvise_cold_or_pageout_pte_range");

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	buf[0] = 't';

	dynamic_debug_control("file madvise.c +p");
	trace_on();
	madvise(buf, pagesize, MADV_PAGEOUT);
	trace_off();
	dynamic_debug_control("file madvise.c -p");

	printf("%p: %c\n", buf, buf[0]);

	munmap(buf, pagesize);
	trace_exit();
}

void swap_normal_fork(void)
{
	char *buf;
	pid_t pid;

	trace_configure(0, "do_swap_page");

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	buf[0] = 't';
	madvise(buf, pagesize, MADV_PAGEOUT);

	dynamic_debug_control("file memory.c +p");
	trace_on();

	pid = fork();
	if (pid < 0)
		return;

	printf("%p: %c\n", buf, buf[0]);

	if (pid == 0)  // child task
		return;
	else
		wait(NULL);

	trace_off();
	dynamic_debug_control("file memory.c -p");

	munmap(buf, pagesize);
	trace_exit();
}

static void *thread_fun(void *param)
{
	char *buf = param;

	printf("%p: %c\n", buf, buf[0]);

 	return NULL;
}

void swap_normal_pthread(void)
{
	char *buf;
	pthread_t tid1, tid2;

	trace_configure(0, "do_swap_page");

	buf = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	buf[0] = 't';
	madvise(buf, pagesize, MADV_PAGEOUT);

	dynamic_debug_control("file memory.c +p");
	trace_on();
	pthread_create(&tid1, NULL, thread_fun, buf);
	pthread_create(&tid2, NULL, thread_fun, buf);
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	trace_off();
	dynamic_debug_control("file memory.c -p");

	munmap(buf, pagesize);
	trace_exit();
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

	pagesize = getpagesize();

	if (!argv[1])
		goto DEFAULT;

	if (!strncmp(argv[1], "normal", strlen(argv[1])))
DEFAULT:	swap_normal();
	else if (!strncmp(argv[1], "normal_fork", strlen(argv[1])))
		swap_normal_fork();
	else if (!strncmp(argv[1], "normal_pthread", strlen(argv[1])))
		swap_normal_pthread();
	else if (!strncmp(argv[1], "thp", strlen(argv[1])))
		swap_thp();
	else if (!strncmp(argv[1], "mthp", strlen(argv[1])))
		swap_multi_size_thp();
	else
		goto DEFAULT;

	dynamic_debug_end();

	return 0;
}
