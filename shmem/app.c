#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

enum demo_entry {
	DEMO_POSIX,
	DEMO_SYSV,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"posix",
	"sysv",
};

static enum demo_entry test_demo_entry(char *entry)
{
	int i;

	if (!entry)
		return 0;

	for (i = 0; i < DEMO_ENTRY_MAX; i++) {
		if (!strncmp(entry, string[i], strlen(entry)))
			return i;
	}

	printf("Don't look for right demo entry.\n");
	return -EINVAL;
}

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
	switch (test_demo_entry(argv[1])) {
		default:
		case DEMO_POSIX:
			test_posix_shmem();
			break;
		case DEMO_SYSV:
			test_sysv_shmem();
			break;
	}

	return 0;
}
