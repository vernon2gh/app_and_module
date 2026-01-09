#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

static void *thread_fun(void *param)
{
	bool parent = param ? true : false;
	pid_t pid;

	usleep(1000);

	if (parent)
		printf("parent task in the thread:\n");
	else
		printf("child task in the thread:\n");

	pid = gettid();
	printf("  task_struct pid  %d\n", pid);
	printf("  task_struct tgid %d\n", getpid());
	printf("  task_struct pgid %d\n", getpgid(pid));
	printf("  task_struct sid  %d\n", getsid(pid));

 	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t tid;
	pid_t pid;

	pid = fork();
	switch (pid) {
	case -1:
		perror("Execute fork() failed.");
		return -1;
	case 0:
		printf("child task:\n");
		break;
	default:
		usleep(2000);
		printf("parent task:\n");
		break;
	}

	pthread_create(&tid, NULL, thread_fun, (void *)(unsigned long)pid);

	pid = gettid();
	printf("  task_struct pid  %d\n", pid);
	printf("  task_struct tgid %d\n", getpid());
	printf("  task_struct pgid %d\n", getpgid(pid));
	printf("  task_struct sid  %d\n", getsid(pid));

	pthread_join(tid, NULL);

	return 0;
}

