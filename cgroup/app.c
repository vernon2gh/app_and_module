#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define TATOL_SIZE	(100 * 1024 * 1024)
#define PAGE_SIZE	4096

enum demo_entry {
	DEMO_MEMORY_MAX,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"memory_max",
};

static enum demo_entry test_demo_entry(char *entry)
{
	int i;

	if (!entry)
		return DEMO_MEMORY_MAX;

	for (i = 0; i < DEMO_ENTRY_MAX; i++) {
		if (!strncmp(entry, string[i], strlen(entry)))
			return i;
	}

	printf("Don't look for right demo entry.\n");
	return -EINVAL;
}

static void test_memory_max(void)
{
	int nr_pages = TATOL_SIZE / PAGE_SIZE;
	int i = 0;
	char *buf;

	buf = malloc(TATOL_SIZE);

	for (i = 0; i < nr_pages; i++)
		buf[i * PAGE_SIZE] = 'a';
}

int main(int argc, char *argv[])
{
	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_MEMORY_MAX:
			test_memory_max();
			break;
	}

	return 0;
}
