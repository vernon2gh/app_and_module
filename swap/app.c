#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include "dynamic_debug.h"
#include "trace.h"

static int pagesize;

enum demo_entry {
	DEMO_NORMAL,
	DEMO_NORMAL_FORK,
	DEMO_NORMAL_PTHREAD,
	DEMO_THP,
	DEMO_MTHP,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"normal",
	"normal_fork",
	"normal_pthread",
	"thp",
	"mthp",
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

static void swap_normal(void)
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

static void swap_normal_fork(void)
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

static void swap_normal_pthread(void)
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

static void swap_thp(void)
{
	__swap_thp((void *)0x7f987f200000, 512);
}

static void swap_multi_size_thp(void)
{
	__swap_thp((void *)0x7f9877ff0000, 16);
}

int main(int argc, char *argv[])
{
	dynamic_debug_start();

	pagesize = getpagesize();

	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_NORMAL:
			swap_normal();
			break;
		case DEMO_NORMAL_FORK:
			swap_normal_fork();
			break;
		case DEMO_NORMAL_PTHREAD:
			swap_normal_pthread();
			break;
		case DEMO_THP:
			swap_thp();
			break;
		case DEMO_MTHP:
			swap_multi_size_thp();
			break;
	}

	dynamic_debug_end();

	return 0;
}
