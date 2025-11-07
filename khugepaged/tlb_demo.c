#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <x86intrin.h>

#define PAGE_4KB (4096)
#define PAGE_2MB (2 * 1024 * 1024)

// 确保超出TLB容量
// 典型x86 CPU: L1 DTLB ~64 entries, L2 TLB ~1536 entries
#define ARRAY_SIZE (128 * 1024 * 1024)  // 128MB
#define BATCH_SIZE (ARRAY_SIZE/PAGE_4KB)
#define ITERATIONS 10000

static inline int64_t access_memory_batch(volatile char *buffer, int count)
{
	uint64_t start, end;
	uint64_t seed;
	size_t *offsets;
	volatile char tmp;
	int i;

	srand(time(NULL));

	offsets = malloc(count * sizeof(size_t));
	if (!offsets) {
		perror("malloc failed");
		return -EINVAL;
	}

	// 随机访问大量不同的地址 - 保持访问范围一致，但确保跨越很多页
	// 效果分析：访问 ARRAY_SIZE 随机的4KB页
	// - 4KB页 → 高TLB miss
	// - 2MB页 很多在同一个2MB页内 → 低TLB miss
	for (i = 0; i < count; i++) {
		seed = rand();

		// 尽量分散到不同的4KB页上
		// 这样4KB页面会有更高的TLB miss率
		size_t max_4kb_pages = ARRAY_SIZE / PAGE_4KB;
		size_t page_4kb = seed % max_4kb_pages;

		// 在每个4KB页内随机选一个cache line对齐的位置
		size_t in_page_offset = (seed % (PAGE_4KB / 64)) * 64;
		size_t offset = page_4kb * PAGE_4KB + in_page_offset;

		offsets[i] = offset;
	}

	// 测量开始时间
	start = __rdtsc();

	// 批量访问内存 - 这样TLB压力更明显
	for (int i = 0; i < count; i++) {
		tmp = buffer[offsets[i]];
		asm volatile("" :: "r"(tmp) : "memory");
	}

	// 测量结束时间
	end = __rdtsc();

	free(offsets);

	return end - start;
}

double test_tlb_performance(char *buffer, size_t size, size_t page_size)
{
	struct timespec start, end;
	uint64_t total_cycles = 0;
	size_t access_count = 0;
	int64_t cycles;
	double elapsed;
	int i;

	printf("Test: %s page\n", page_size == PAGE_2MB ? "2MB" : "4KB");
	printf("  Total pages: %zu\n", size / page_size);

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < ITERATIONS; i++) {
		cycles = access_memory_batch(buffer, BATCH_SIZE);
		if (cycles == -EINVAL)
			continue;

		total_cycles += cycles;
		access_count += BATCH_SIZE;
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = (end.tv_sec - start.tv_sec) +
		  (end.tv_nsec - start.tv_nsec) / 1e9;

	printf("  Completed %zu accesses in %.2f seconds\n", access_count, elapsed);
	printf("  Average cycles per access: %.2f\n", (double)total_cycles / access_count);
	printf("  Throughput: %.2f M accesses/sec\n\n", access_count / elapsed / 1e6);

	return (double)total_cycles / access_count;
}

int main(int argc, char *argv[])
{
	char *buffer_4kb, *buffer_2mb;
	double cycles_4kb, cycles_2mb;

	printf("TLB Performance Test: 4KB vs 2MB pages\n\n");
	printf("Array size: %d MB\n", ARRAY_SIZE / (1024 * 1024));
	printf("Total iterations: %d\n\n", ITERATIONS);

	// 分配使用4KB页的内存
	buffer_4kb = mmap(NULL, ARRAY_SIZE, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
	if (buffer_4kb == MAP_FAILED) {
		perror("mmap 4KB failed");
		return -EINVAL;
	}

	// 分配使用2MB大页的内存
	// echo 1024 > /proc/sys/vm/nr_hugepages
	// cat /proc/meminfo | grep -i huge
	buffer_2mb = mmap(NULL, ARRAY_SIZE, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB, -1, 0);
	if (buffer_2mb == MAP_FAILED) {
		perror("mmap 2MB hugepage failed");
		printf("\nHint: Configure hugepages with:\n");
		printf("  sudo sh -c 'echo 4096 > /proc/sys/vm/nr_hugepages'\n");
		munmap(buffer_4kb, ARRAY_SIZE);
		return -EINVAL;
	}
	//madvise(buffer_2mb, ARRAY_SIZE, MADV_HUGEPAGE);
	//sleep(60);

	// 测试4KB页面性能 - 高TLB压力
	cycles_4kb = test_tlb_performance(buffer_4kb, ARRAY_SIZE, PAGE_4KB);

	// 测试2MB页面性能 - 低TLB压力
	cycles_2mb = test_tlb_performance(buffer_2mb, ARRAY_SIZE, PAGE_2MB);

	// 总结对比
	printf("PERFORMANCE SUMMARY:\n");
	printf("  4KB pages: %.2f cycles/access\n", cycles_4kb);
	printf("  2MB pages: %.2f cycles/access\n", cycles_2mb);
	printf("  Performance improvement: %.2f%%\n",
			(cycles_4kb - cycles_2mb) / cycles_4kb * 100.0);
	printf("  Speedup factor: %.2fx\n", cycles_4kb / cycles_2mb);

	// 清理
	munmap(buffer_4kb, ARRAY_SIZE);
	munmap(buffer_2mb, ARRAY_SIZE);

	return 0;
}
