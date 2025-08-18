#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>

enum demo_entry {
	DEMO_HELLO,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"hello",
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

static void test_hello(void)
{
	printf("hello world.\n");

	unsigned long size = 1024 * 1024 * 1024 * 5UL;
	char *buf = mmap(0, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	unsigned long i;

	for (i = 0; i < size; i += 4096)
		buf[i] = i%256;

	while (1);
}

int main(int argc, char *argv[])
{
	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_HELLO:
			test_hello();
			break;
	}

	return 0;
}
