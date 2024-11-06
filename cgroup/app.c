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
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"memory_reclaim_swappiness",
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

int main(int argc, char *argv[])
{
	pagesize = getpagesize();

	switch (test_demo_entry(argv[1])) {
		default:
			break;
		case DEMO_MEMORY_RECLAIM_SWAPPINESS:
			memory_reclaim_swappiness();
			break;
	}

	return 0;
}
