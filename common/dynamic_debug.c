#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

static int fd_control;

void dynamic_debug_start(void)
{
	fd_control = open("/sys/kernel/debug/dynamic_debug/control", O_RDWR);
}

void dynamic_debug_end(void)
{
	close(fd_control);
}

void dynamic_debug_control(const char *buf)
{
	write(fd_control, buf, strlen(buf));
}
