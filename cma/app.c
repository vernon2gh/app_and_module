#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

enum demo_entry {
	DEMO_CMA,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"cma",
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

static void test_cma(void)
{
#define SZ_1G	0x40000000
	char *buf;
	int fd;

	fd = open("/dev/cma_test",O_RDWR);
	buf = mmap(NULL, SZ_1G, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        printf("%s\n", buf);
	strcpy(buf, "hello world");
        printf("%s\n", buf);

	munmap(buf, SZ_1G);
}

int main(int argc, char *argv[])
{
	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_CMA:
			test_cma();
			break;
	}

	return 0;
}
