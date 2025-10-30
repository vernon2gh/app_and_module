#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#define LEN (16 * 1024 * 1024) /* 16MB */

int main(int argc, char *argv[])
{
	char *buf;
	int err, i;

	sleep(1); /* wait ebpf to attach this task */

	buf = mmap(NULL, LEN, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (buf == MAP_FAILED)
		return -EINVAL;
	// printf("pid %d, buf %lx\n", getpid(), (unsigned long)buf);

	err = madvise(buf, LEN, MADV_HUGEPAGE);
	if (err == -1)
		goto unmap;

	// printf("wait trigger.\n");
	// getchar();

	for (i = 0; i < LEN; i += 4096)
		buf[i] = 1;

	// getchar();
unmap:
	munmap(buf, LEN);
	return 0;
}
