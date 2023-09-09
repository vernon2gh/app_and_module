#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

char enbuf[] = "func get_unmapped_area +p";
char debuf[] = "func get_unmapped_area -p";

char enbuf2[] = "func shmem_get_unmapped_area +p";
char debuf2[] = "func shmem_get_unmapped_area -p";

int main(int argc, char *argv[])
{
	char *buf;
	int fd, fd_test;

	printf("hello world.\n");

	fd = open("/sys/kernel/debug/dynamic_debug/control", O_RDWR);
	write(fd, enbuf, sizeof(enbuf));
	write(fd, enbuf2, sizeof(enbuf2));

	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); // PRIVATE ANONYMOUS
	munmap(buf, 4096);
	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);  // SHARE ANONYMOUS
	munmap(buf, 4096);

	fd_test = open("./hello.c", O_RDWR);
	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fd_test, 0);   // PRIVATE FILE
	munmap(buf, 4096);
	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd_test, 0);    // SHARE FILE
	munmap(buf, 4096);
	close(fd_test);

	write(fd, debuf, sizeof(debuf));
	write(fd, debuf2, sizeof(debuf2));
	close(fd);

	return 0;
}
