#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static void test_posix_shmem(void)
{
	int fd;
	char *buf;
	char tmp;

	fd = shm_open("shmfile", O_RDWR | O_CREAT, 0666);
	ftruncate(fd, 4096);

	buf = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	tmp = *buf;
	*buf = 0x1;
	munmap(buf, 4096);

	shm_unlink("shmfile");
}

static void test_sysv_shmem(void)
{
        int shmid;
        char *buf;
	char tmp;

	shmid = shmget(0x22, 4096, IPC_CREAT | 0666);

	buf = shmat(shmid, NULL, 0);
	tmp = *buf;
	*buf = 0x2;
	shmdt(buf);

	shmctl(shmid, IPC_RMID, NULL);
}

int main(int argc, char *argv[])
{
	if (!argv[1])
		goto DEFAULT;

	if (!strncmp(argv[1], "sysv", strlen("sysv")))
		test_sysv_shmem();
	else
DEFAULT:	test_posix_shmem();

	return 0;
}
