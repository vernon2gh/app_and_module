#include <stdio.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	char *buf;

	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	buf[0] = 't';
	madvise(buf, 4096, MADV_PAGEOUT);
	while(1);

	munmap(buf, 4096);

	return 0;
}
