#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <x86intrin.h>

#define PAGE_SIZE	4096
#define BATCH_SIZE	(128 * 1024 * 1024)	// 128MB
#define BATCH_PAGES	(BATCH_SIZE / PAGE_SIZE)
#define ITERATIONS	10000

#define KHUGEPAGED_SCAN	"/sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs"

static inline int64_t access_memory_batch(volatile char *buffer, int count)
{
	static size_t offsets[BATCH_PAGES];
	uint64_t start, end;
	volatile char tmp;
	int i;

	srand(time(NULL));

	/*
	 * Access a large number of different addresses randomly, keep the
	 * scope of access consistent, but make sure to span many pages.
	 * Effect analysis:
	 *   4KB page -> high TLB miss
	 *   2MB page -> low TLB miss
	 */
	for (i = 0; i < count; i++) {
		uint64_t seed = rand();

		/*
		 * as far as possible to spread it over different 4KB pages so
		 * that the 4KB page will have a higher TLB miss rate.
		 */
		size_t max_pages = BATCH_SIZE / PAGE_SIZE;
		size_t page = seed % max_pages;

		/*
		 * select a cache line alignment location within
		 * each 4KB page randomly, reduce cache hit.
		 */
		size_t in_page_offset = (seed % (PAGE_SIZE / 64)) * 64;
		size_t offset = page * PAGE_SIZE + in_page_offset;

		offsets[i] = offset;
	}

	start = __rdtsc();

	for (int i = 0; i < count; i++) {
		tmp = buffer[offsets[i]];
		asm volatile("" :: "r"(tmp) : "memory");
	}

	end = __rdtsc();

	return end - start;
}

static inline void test_tlb_performance(char *buffer, const char *caller)
{
	struct timespec start, end;
	uint64_t total_cycles = 0;
	size_t total_access = 0;
	int64_t cycles;
	double elapsed;
	int i;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < ITERATIONS; i++) {
		cycles = access_memory_batch(buffer, BATCH_PAGES);
		if (cycles == -EINVAL)
			continue;

		total_cycles += cycles;
		total_access += BATCH_PAGES;
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = (end.tv_sec - start.tv_sec) +
		  (end.tv_nsec - start.tv_nsec) / 1e9;

	printf("%s\n", caller);
	printf("  Completed %zu accesses in %.2f seconds\n", total_access, elapsed);
	printf("  Average cycles per access: %.2f cycles/access\n", (double)total_cycles / total_access);
	printf("  Throughput: %.2f M accesses/sec\n\n", total_access / elapsed / 1e6);
}

static void process_cold(void)
{
	char *buffer;

	buffer = mmap(NULL, BATCH_SIZE, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
	if (buffer == MAP_FAILED) {
		perror("Allocate memory failed.\n");
		return;
	}

	/* Make khugepaged to collapse buffer */
	madvise(buffer, BATCH_SIZE, MADV_HUGEPAGE);
	/* Make buffer to inactive list for lru */
	madvise(buffer, BATCH_SIZE, MADV_FREE);

	test_tlb_performance(buffer, __func__);
	while(1)
		sleep(1);

	munmap(buffer, BATCH_SIZE);
}

static void process_hot(void)
{
	char val[10] = { 0 };
	int len;
	int fd;
	char *buffer;

	fd = open(KHUGEPAGED_SCAN, O_RDWR);
	if (fd == -1) {
		printf("Open %s file failed.\n", KHUGEPAGED_SCAN);
		return;
	}
	len = read(fd, val, sizeof(val));
	lseek(fd, 0, SEEK_SET);

	buffer = mmap(NULL, BATCH_SIZE, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
	if (buffer == MAP_FAILED) {
		perror("Allocate memory failed.\n");
		return;
	}

	/* Make khugepaged to collapse buffer */
	madvise(buffer, BATCH_SIZE, MADV_HUGEPAGE);
	/* khuegepaged start working. */
	len = write(fd, val, len);
	/* doing something, e.g. access buffer frequently */
#ifdef ACCESS_BUFFER
	for (int i = 0; i < 10000; i++)
		memset(buffer, i, BATCH_SIZE);
#else
	sleep(60);
#endif

	test_tlb_performance(buffer, __func__);

	munmap(buffer, BATCH_SIZE);
}

int main(int argc, char *argv[])
{
        pid_t pid;

	printf("Batch size : %d MB\n", BATCH_SIZE / (1024 * 1024));
	printf("Batch pages: %d(4KB) or %d(2MB)\n", BATCH_PAGES,
				BATCH_SIZE / (2 * 1024 * 1024));
	printf("Total iterations: %d\n\n", ITERATIONS);

	pid = fork();
	switch (pid) {
	case -1:
		perror("Execute fork() failed.");
		break;
	case 0:
		process_cold();
		break;
	default:
		sleep(1); /* wait process_cold to allocate memory first */
		process_hot();
		kill(pid, SIGTERM);
		break;
	}

	return 0;
}
