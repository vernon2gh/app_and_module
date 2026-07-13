#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>

#define BUFSIZE		(128UL * 1024 * 1024)
#define ITERS		30000UL
#define PAGESIZE	4096

int main(int argc, char *argv[])
{
	unsigned long i;
	char *buffer;

	sleep(1); /* wait ebpf to attach this task */
	srandom(time(NULL));

	buffer = mmap(NULL, BUFSIZE, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buffer == MAP_FAILED)
		return -EINVAL;

	for (i = 0; i < ITERS; i++) {
		int page_nr = random() % (BUFSIZE / PAGESIZE);
		memset(buffer + page_nr * PAGESIZE, 't', PAGESIZE);
	}

	munmap(buffer, BUFSIZE);

	return 0;
}
