#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

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

enum {
	CMA_ALLOC = _IOW('T', 0, unsigned long),
	CMA_FREE  = _IOW('T', 1, unsigned long),
	CMA_PFN   = _IOWR('T', 0, unsigned long),
};

#define SZ_1G	0x40000000

static void test_cma(void)
{
	unsigned long data;
	char *buf;
	int fd;
	int i;
	int err;

	fd = open("/dev/cma_test", O_RDWR);
	if (fd < 0) {
		printf("open failed: %s\n", strerror(errno));
		return;
	}

	err = ioctl(fd, CMA_ALLOC, SZ_1G);
	if (err < 0) {
		printf("ioctl CMA_ALLOC failed: %s\n", strerror(errno));
		goto ioctl_fail;
	}

	buf = mmap(NULL, SZ_1G, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED) {
		printf("mmap failed: %s\n", strerror(errno));
		goto mmap_fail;
	}

	printf("old buf: %s\n", buf);
	strcpy(buf, "hello world");
	printf("new buf: %s\n", buf);

	for (i = 0; i < 10; i++) {
		data = (unsigned long)buf + 4096 * i;
		err = ioctl(fd, CMA_PFN, &data);
		if (err < 0) {
			printf("ioctl CMA_PFN failed: %s\n", strerror(errno));
			goto out;
		}

		printf("vaddr 0x%lx, pfn 0x%lx\n", (unsigned long)buf + 4096 * i,
						   data);
	}

out:
	munmap(buf, SZ_1G);
mmap_fail:
	ioctl(fd, CMA_FREE, SZ_1G);
ioctl_fail:
	close(fd);
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
