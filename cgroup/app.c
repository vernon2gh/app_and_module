#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char *buf;
	int i = 0;

	while (1) {
		buf = malloc(4096);
		*buf = 1;

		if (++i%256 == 0) {
			printf("%dMB\n", i/256);
			sleep(1);
		}
	}

	return 0;
}
