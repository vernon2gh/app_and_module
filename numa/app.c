#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <numa.h>
#include <pthread.h>
#include <sys/mman.h>

#define NODE_SYS	"/sys/devices/system/node"
#define CMA_DEV		"/dev/cma_test"
#define MSZ		256		/* matrix dimension for matmul */
#define WSZ		(64UL << 20)	/* 64 MiB working set for bw/latency */

/* Read a whole sysfs/procfs text file into buf. */
static void readf(const char *path, char *buf, size_t sz)
{
	FILE *fp = fopen(path, "r");
	size_t n = 0;

	if (fp) {
		n = fread(buf, 1, sz - 1, fp);
		fclose(fp);
	}
	buf[n] = '\0';
}

/* Find "<key>...<value>" in a key/value blob, return the numeric value. */
static long long field(const char *txt, const char *key)
{
	const char *p = strstr(txt, key);

	if (!p)
		return 0;
	p += strlen(key);
	while (*p == ':' || *p == ' ' || *p == '\t')
		p++;
	return strtoll(p, NULL, 10);
}

/* Print NUMA node memory distribution, hugepage hit ratio and local
 * memory access ratio.
 */
static void numa_report(void)
{
	char buf[8192];
	int nid;
	long long ah, ap;

	printf("================ NUMA report ================\n");

	/* 1. NUMA node memory distribution */
	for (nid = 0; nid < 64; nid++) {
		char p[128];

		snprintf(p, sizeof(p), NODE_SYS "/node%d/meminfo", nid);
		readf(p, buf, sizeof(buf));
		if (!buf[0])
			continue;
		printf("node%d: total=%lld used=%lld free=%lld kB\n", nid,
		       field(buf, "MemTotal:"), field(buf, "MemUsed:"),
		       field(buf, "MemFree:"));
	}

	/* 2. Hugepage hit ratio */
	readf("/proc/meminfo", buf, sizeof(buf));
	ah = field(buf, "AnonHugePages:");
	ap = field(buf, "AnonPages:");
	printf("AnonHugePages=%lld AnonPages=%lld kB, Hugepage Hit Ratio=%.2f%%\n",
	       ah, ap, ah + ap ? 100.0 * ah / (ah + ap) : 0);

	/* 3. Local memory access ratio */
	for (nid = 0; nid < 64; nid++) {
		char p[128];
		long long l, o;

		snprintf(p, sizeof(p), NODE_SYS "/node%d/numastat", nid);
		readf(p, buf, sizeof(buf));
		if (!buf[0])
			continue;
		l = field(buf, "local_node");
		o = field(buf, "other_node");
		printf("node%d: local=%lld other=%lld ratio=%.2f%%\n", nid, l,
		       o, l + o ? 100.0 * l / (l + o) : 0);
	}
}

/* Pick the NUMA node with the most free memory. */
static int best_numa_node(void)
{
	int n, best = 0, max;
	long long maxfree = -1;

	max = numa_max_node();
	for (n = 0; n <= max; n++) {
		long long fr;

		if (numa_node_size64(n, &fr) < 0)
			continue;
		if (fr > maxfree) {
			maxfree = fr;
			best = n;
		}
	}
	return best;
}

/* Pin the current process to the best NUMA node (most free memory). */
static void numa_bind_best(void)
{
	struct bitmask *nodes;

	printf("============ bind to best node ===========\n");

	if (numa_available() < 0) {		/* init libnuma; <0 if NUMA unsupported */
		fprintf(stderr, "numa not available\n");
		return;
	}

	nodes = numa_allocate_nodemask();
	numa_bitmask_setbit(nodes, best_numa_node());
	numa_bind(nodes);
	numa_bitmask_free(nodes);
}

/* Current monotonic time in nanoseconds. */
static long long now_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/*
 * Allocate sz bytes of CMA-backed memory via /dev/cma_test.
 */
static void *cma_mmap(size_t sz)
{
	void *buf;

	int fd = open(CMA_DEV, O_RDWR);
	if (fd < 0) {
		perror(CMA_DEV);
		return NULL;
	}

	buf = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED) {
		perror("mmap cma_test failed.\n");
		close(fd);
		return NULL;
	}

	close(fd);
	return buf;
}

/*
 * Memory-intensive workload: naive NxN matrix multiply. The column-major
 * access to b (stride n) misses cache on nearly every load, so the
 * dominant traffic is ~N^3 reads of b. Effective bandwidth is reported as
 * N^3 * sizeof(double) / time. Matrices are backed by CMA memory from
 * /dev/cma_test (on the bound node).
 */
static void bench_matmul(void)
{
	int n = MSZ, i, j, k;
	size_t bytes = sizeof(double) * n * n;
	double *a, *b, *c, gbs;
	volatile double sink = 0;
	long long t0, t1;

	a = cma_mmap(bytes);
	b = cma_mmap(bytes);
	c = cma_mmap(bytes);
	if (!a || !b || !c) {
		fprintf(stderr, "matmul: cma alloc failed\n");
		if (a) munmap(a, bytes);
		if (b) munmap(b, bytes);
		if (c) munmap(c, bytes);
		return;
	}

	for (i = 0; i < n * n; i++) {
		a[i] = (double)i * 0.001;
		b[i] = (double)i * 0.002;
	}

	t0 = now_ns();
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++) {
			double s = 0;

			for (k = 0; k < n; k++)
				s += a[i * n + k] * b[k * n + j];
			c[i * n + j] = s;
		}
	t1 = now_ns();

	for (i = 0; i < n * n; i++)
		sink += c[i];

	gbs = (double)n * n * n * sizeof(double) / ((t1 - t0) / 1e9) / 1e9;
	printf("bandwidth %.2f GB/s\n", gbs);

	munmap(a, bytes); munmap(b, bytes); munmap(c, bytes);
}

/*
 * Random-access latency via pointer chasing: a single random cycle is
 * built over a working set larger than the LLC, defeating prefetching so
 * each step hits memory. The working set is CMA memory from /dev/cma_test.
 */
static void bench_latency(void)
{
	size_t n = WSZ / sizeof(size_t), i;
	size_t *arr, *perm;
	volatile size_t idx;
	unsigned int seed = 1;
	long long t0, t1;

	arr = cma_mmap(WSZ);
	perm = cma_mmap(WSZ);
	if (!arr || !perm) {
		fprintf(stderr, "latency: cma alloc failed\n");
		goto out;
	}

	for (i = 0; i < n; i++)
		perm[i] = i;
	for (i = n - 1; i > 0; i--) {	/* Fisher-Yates shuffle */
		size_t j = rand_r(&seed) % (i + 1);
		size_t t = perm[i];

		perm[i] = perm[j];
		perm[j] = t;
	}
	for (i = 0; i < n; i++)		/* build one random cycle */
		arr[perm[i]] = perm[(i + 1) % n];

	idx = perm[0];
	t0 = now_ns();
	for (i = 0; i < n; i++)
		idx = arr[idx];
	t1 = now_ns();

	printf("latency %.1f ns\n", (double)(t1 - t0) / n);

out:
	if (arr) munmap(arr, WSZ);
	if (perm) munmap(perm, WSZ);
}

/* Run a memory-intensive workload and measure latency/bw. */
static void mem_bench(void)
{
	printf("============= Memory Benchmark =============\n");
	bench_matmul();
	bench_latency();
}

/* Bytes referenced (accessed) by the process, from smaps_rollup. KiB. */
static long long referenced_kb(void)
{
	char buf[16384];

	readf("/proc/self/smaps_rollup", buf, sizeof(buf));
	return field(buf, "Referenced:");
}

/* Clear the page-referenced bits of all pages owned by the process. */
static int clear_refs(void)
{
	FILE *fp = fopen("/proc/self/clear_refs", "w");

	if (!fp)
		return -1;
	fputs("1", fp);
	fclose(fp);
	return 0;
}

#define MON_INTERVAL_MS	200

/*
 * Background thread: sample memory-access hotness as the referenced-byte
 * rate (MB/sec).
 */
static void *monitor_thread(void *arg)
{
	long long ref;

	if (clear_refs() < 0) {
		fprintf(stderr, "cannot open /proc/self/clear_refs\n");
		return NULL;
	}

	while (1) {
		clear_refs();
		usleep(MON_INTERVAL_MS * 1000);
		ref = referenced_kb();
		printf("[hotness] %.2f MiB/s referenced\n",
		       (double)ref * 1000 / 1024 / MON_INTERVAL_MS);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t mon;

	numa_bind_best();

	pthread_create(&mon, NULL, monitor_thread, NULL);
	mem_bench();
	pthread_cancel(mon);
	pthread_join(mon, NULL);

	numa_report();

	return 0;
}
