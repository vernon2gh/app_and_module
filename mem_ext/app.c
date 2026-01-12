#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#define PAGE_SIZE	4096
#define BATCH_SIZE	(128 * 1024 * 1024)	// 128MB
#define BATCH_PAGES	(BATCH_SIZE / PAGE_SIZE)

int main(int argc, char *argv[])
{
	char *buffer;
	int err;

	printf("Current pid: %d\n", getpid());
	printf("Batch size : %d MB\n", BATCH_SIZE / (1024 * 1024));
	printf("Batch pages: %d(4KB) or %d(2MB)\n", BATCH_PAGES,
				BATCH_SIZE / (2 * 1024 * 1024));
	sleep(1); /* wait ebpf to attach this task */

	buffer = mmap(NULL, BATCH_SIZE, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANON, -1, 0);
	if (buffer == MAP_FAILED)
		return -EINVAL;

	memset(buffer, 't', BATCH_SIZE);

unmap:
	munmap(buffer, BATCH_SIZE);

	return 0;
}
