#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

static pid_t gettid(void)
{
    return syscall(__NR_gettid);
}

static void *thread_fun(void *param)
{
    printf("pid: %d, ppid: %d, tid: %d\n", getpid(), getppid(), gettid());
    sleep(10);
    return NULL;
}

int main(void)
{
    pthread_t tid;
    int ret;

    printf("pid: %d, ppid: %d, tid: %d\n", getpid(), getppid(), gettid());

    ret = pthread_create(&tid, NULL, thread_fun, NULL);
    if (ret == -1) {
        perror("can't create new thread");
        return -1;
    }

    if (pthread_join(tid, NULL) != 0) {
        perror("call pthread_join function fail");
        return -1;
    }

    return 0;
}