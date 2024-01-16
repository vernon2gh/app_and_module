#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "trace.h"

static int fd_tracing_on;

void trace_configure(int pid, const char *function)
{
	int tmp_fd;
	char buf[10];

	/*
	 * echo 0 > /sys/kernel/tracing/tracing_on
	 */
	fd_tracing_on = open("/sys/kernel/tracing/tracing_on", O_RDWR);
	write(fd_tracing_on, "0", 1);

	/*
	 * echo $TRACE_PID > /sys/kernel/tracing/set_ftrace_pid
	 */
	tmp_fd = open("/sys/kernel/tracing/set_ftrace_pid", O_RDWR);
	sprintf(buf, "%d", pid);
	write(tmp_fd, buf, strlen(buf));
	close(tmp_fd);

	/*
	 * echo function_graph > /sys/kernel/tracing/current_tracer
	 */
	tmp_fd = open("/sys/kernel/tracing/current_tracer", O_RDWR);
	write(tmp_fd, "function_graph", strlen("function_graph"));
	close(tmp_fd);

	/*
	 * echo $TRACE_FUNC > /sys/kernel/tracing/set_graph_function
	 */
	tmp_fd = open("/sys/kernel/tracing/set_graph_function", O_RDWR);
	if (write(tmp_fd, function, strlen(function)) == -1)
		perror("Set graph function failed.\n");
	close(tmp_fd);

	/*
	 * echo > /sys/kernel/tracing/trace
	 */
	fclose(fopen("/sys/kernel/tracing/trace", "w"));
}

void trace_exit(void)
{
	close(fd_tracing_on);
}

void trace_on(void)
{
	/*
	 * echo 1 > /sys/kernel/tracing/tracing_on
	 */
	write(fd_tracing_on, "1", 1);
}

void trace_off(void)
{
	/*
	 * echo 0 > /sys/kernel/tracing/tracing_on
	 */
	write(fd_tracing_on, "0", 1);
}
