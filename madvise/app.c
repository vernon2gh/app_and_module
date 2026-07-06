#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>

#define BUFSIZE		(64UL * 1024 * 1024)
#define ITERATIONS	10000
#define WRITERS_DEF	16

static volatile bool cancel;

static void touch_mapping(char *addr)
{
	long page_size = sysconf(_SC_PAGESIZE);
	size_t i;

	for (i = 0; i < BUFSIZE; i += (size_t)page_size)
		addr[i] = 1;
}

static void *munmap_fn(void *arg)
{
	void *buf;

	while (!cancel) {
		buf = mmap(NULL, BUFSIZE, PROT_READ | PROT_WRITE,
			   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (buf == MAP_FAILED)
			break;

		touch_mapping(buf);
		munmap(buf, BUFSIZE);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t writers[WRITERS_DEF];
	struct timespec start, end;
	double elapsed;
	int advice;
	void *buf;
	int i;

	if (argc != 2) {
		printf("uasge: %s <cold/pageout>\n", argv[0]);
		return -EINVAL;
	}

	if (!strcmp(argv[1], "cold"))
		advice = MADV_COLD;
	else
		advice = MADV_PAGEOUT;

	buf = mmap(NULL, BUFSIZE, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buf == MAP_FAILED)
		return -EINVAL;
	touch_mapping(buf);

	for (i = 0; i < WRITERS_DEF; i++)
		pthread_create(&writers[i], NULL, munmap_fn, NULL);

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < ITERATIONS; i++)
		madvise(buf, BUFSIZE, advice);
	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = (end.tv_sec - start.tv_sec) +
		  (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("Completed in %.2f seconds\n", elapsed);

	cancel = true;
	munmap(buf, BUFSIZE);
	for (i = 0; i < WRITERS_DEF; i++)
		pthread_join(writers[i], NULL);

	return 0;
}
