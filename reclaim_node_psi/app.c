#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
	unsigned long i;
	int fd;

	fd = open("/sys/devices/system/node/node0/reclaim", O_WRONLY);
	for (i = 0; i < 10000; i++)
		write(fd, "16M", 3);

	close(fd);

	return 0;
}
