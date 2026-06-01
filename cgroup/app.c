#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "dynamic_debug.h"
#include "trace.h"

static int pagesize;

enum demo_entry {
	DEMO_MEMORY_RECLAIM_SWAPPINESS,
	DEMO_MMAP_TOUCH_FREE,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"memory_reclaim_swappiness",
	"mmap_touch_free",
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

static void memory_reclaim_swappiness(void)
{
	int i, nr = 25600;
	char *buf;

	buf = mmap(0, pagesize * nr, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	for (i = 0; i < nr; i++)
		buf[pagesize * i] = 't';

	while(1); // don't free anon page.
	munmap(buf, pagesize * nr);
}

static void mmap_touch_free(void)
{
#define MMAP_SIZE	(120UL * 1024 * 1024)
#define STEP_SIZE	(10UL * 1024 * 1024)
	unsigned long offset;
	char *buf;

	buf = mmap(0, MMAP_SIZE, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	for (offset = 0; offset < MMAP_SIZE; offset += STEP_SIZE) {
		memset(buf + offset, 't', STEP_SIZE);
		sleep(1);
	}

	for (offset = 0; offset < MMAP_SIZE; offset += STEP_SIZE) {
		madvise(buf + offset, STEP_SIZE, MADV_DONTNEED);
		sleep(1);
	}

	munmap(buf, MMAP_SIZE);
}

int main(int argc, char *argv[])
{
	pagesize = getpagesize();

	switch (test_demo_entry(argv[1])) {
		default:
			break;
		case DEMO_MEMORY_RECLAIM_SWAPPINESS:
			memory_reclaim_swappiness();
			break;
		case DEMO_MMAP_TOUCH_FREE:
			mmap_touch_free();
			break;
	}

	return 0;
}
