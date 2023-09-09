#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

char enbuf[] = "func handle_pte_fault +p";
char debuf[] = "func handle_pte_fault -p";

int main(int argc, char *argv[])
{
	char tmp;

	printf("hello world.\n");

	int fd = open("/sys/kernel/debug/dynamic_debug/control", O_RDWR);
	char *buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	write(fd, enbuf, sizeof(enbuf));
	tmp = *buf;
	*buf = 0x12;
	write(fd, debuf, sizeof(debuf));

	close(fd);
	munmap(buf, 4096);

	return 0;
}
