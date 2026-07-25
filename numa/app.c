// SPDX-License-Identifier: GPL-2.0
/*
 * app.c - 演示 NUMA 节点选择
 *
 * 用法:
 *   ./a.out <页数>              默认策略: 大页落在 first-touch CPU 的本地节点
 *   ./a.out <页数> bind <node>  mbind(MPOL_BIND) 强制到 <node>
 *   ./a.out <页数> interleave   mbind(MPOL_INTERLEAVE) 在 node0/node1 间轮转
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <numaif.h>
#include <errno.h>
#include <sys/mman.h>

#define PAGE_SIZE	4096

/* 打印 numa_maps 中地址落在 [start, start+len) 的行 */
static void show_numa_maps(void *start, size_t len)
{
	unsigned long addr, lo = (unsigned long)start, hi = lo + len;
	char line[512];
	FILE *f = fopen("/proc/self/numa_maps", "r");

	if (!f) {
		perror("fopen numa_maps");
		return;
	}
	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "%lx", &addr) == 1 && addr >= lo && addr < hi)
			printf("  %s", line);
	}

	fclose(f);
}

int main(int argc, char **argv)
{
	char *p;
	long i, nr;

	if (argc < 2) {
		fprintf(stderr, "用法: %s <nr_pages> [bind <node> | interleave]\n", argv[0]);
		return 1;
	}
	nr = atol(argv[1]);

	p = mmap(NULL, nr * PAGE_SIZE, PROT_READ | PROT_WRITE,
		 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	if (argc > 2) {
		int mode;
		unsigned long mask;

		if (!strcmp(argv[2], "bind") && argc > 3) {
			mode = MPOL_BIND;
			mask = 1UL << atoi(argv[3]);
		} else if (!strcmp(argv[2], "interleave")) {
			mode = MPOL_INTERLEAVE;
			mask = 0x3; /* node0 + node1 */
		} else
			return -EINVAL;

		mbind(p, nr * PAGE_SIZE, mode, &mask, 65, 0);
	}

	for (i = 0; i < nr; i++)
		p[i * PAGE_SIZE] = 1;

	show_numa_maps(p, nr * PAGE_SIZE);

	return 0;
}
