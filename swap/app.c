#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "dynamic_debug.h"
#include "trace.h"

static int pagesize;

enum demo_entry {
	DEMO_NORMAL,
	DEMO_NORMAL_FORK,
	DEMO_NORMAL_PTHREAD,
	DEMO_NORMAL_SWPOUTIN,
	DEMO_SWAPIN_PTR,
	DEMO_SWAPIN_SAME,
	DEMO_SWAPIN_RAND,
	DEMO_SWAPIN_TEXT,
	DEMO_THP,
	DEMO_MTHP,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"normal",
	"normal_fork",
	"normal_pthread",
	"normal_swpoutin",
	"swapin_ptr",  /* 堆数据(零字段/小整数/指针混排) -> 可压缩，read_compressed_page + 解压 */
	"swapin_same", /* 整页同一个字节                 -> ZRAM_SAME，read_same_filled_page (只 memset) */
	"swapin_rand", /* 完全随机字节                   -> 不可压缩，read_incompressible_page (只拷贝)  */
	"swapin_text", /* 文本内容                       -> 可压缩，read_compressed_page + 解压 */
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

static void swap_normal_swpoutin(void)
{
	char *buf;
	char tmp;
	int i, nr = 10000;

	buf = mmap(0, pagesize * nr, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	for (i = 0; i < nr; i++) {
		buf[pagesize * i] = 't';
		madvise(buf + pagesize * i, pagesize, MADV_PAGEOUT);
		tmp = buf[pagesize * i];
	}

	munmap(buf, pagesize * nr);
}

static void fill_ptr(char *buf, unsigned seed)
{
	size_t off;

	for (off = 0; off + 8 <= pagesize; off += 8) {
		unsigned long long v;
		unsigned long long base = 0x00007f1000000000ULL +
			((seed & 0xff) << 12);

		switch ((off >> 3) & 3) {
		case 0:
			v = 0;
			break;
		case 1:
			v = base | ((base + off * 3) & 0xffff);
			break;
		case 2:
			v = (base + off) & 0x3fff;
			break;
		default:
			v = base | ((base >> 7) & 0xfffff);
			break;
		}
		memcpy(buf + off, &v, 8);
	}
}

static void fill_same(char *buf, unsigned seed)
{
	memset(buf, 0x41 + (seed & 0xf), pagesize);
}

static void fill_rand(char *buf, unsigned seed)
{
	size_t off;
	unsigned long long x = 0x243f6a8885a308d3ULL + seed;

	for (off = 0; off + 8 <= pagesize; off += 8) {
		x ^= x << 13;
		x ^= x >> 7;
		x ^= x << 17;
		memcpy(buf + off, &x, 8);
	}
}

static void fill_text(char *buf, unsigned seed)
{
	static const char txt[] =
		"the quick brown fox jumps over the lazy dog 0123456789 ";
	size_t n = sizeof(txt) - 1, off;

	for (off = 0; off < pagesize; off++)
		buf[off] = txt[(off + seed * 7) % n];
}

static void swapin_test(void (*fill)(char *buf, unsigned seed))
{
	volatile unsigned long tmp;
	const int iters = 1024;
	struct timespec t0, t1;
	struct rusage r1, r2;
	double sum = 0;
	char *buf;
	int i;

	buf = mmap(NULL, pagesize, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	madvise(buf, pagesize, MADV_NOHUGEPAGE);

	getrusage(RUSAGE_SELF, &r1);
	for (i = 0; i < iters; i++) {
		fill(buf, i);
		madvise(buf, pagesize, MADV_PAGEOUT);

		clock_gettime(CLOCK_MONOTONIC, &t0);
		tmp += ((volatile char *)buf)[0];
		clock_gettime(CLOCK_MONOTONIC, &t1);

		sum += (t1.tv_sec - t0.tv_sec) * 1000000000LL +
		       (t1.tv_nsec - t0.tv_nsec);
	}
	getrusage(RUSAGE_SELF, &r2);

	printf("avg=%.2f us, nr=%d, majflt=%ld\n", sum / iters / 1000.0, iters,
						r2.ru_majflt - r1.ru_majflt);
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
		case DEMO_NORMAL_SWPOUTIN:
			swap_normal_swpoutin();
			break;
		case DEMO_SWAPIN_PTR:
			swapin_test(fill_ptr);
			break;
		case DEMO_SWAPIN_SAME:
			swapin_test(fill_same);
			break;
		case DEMO_SWAPIN_RAND:
			swapin_test(fill_rand);
			break;
		case DEMO_SWAPIN_TEXT:
			swapin_test(fill_text);
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
