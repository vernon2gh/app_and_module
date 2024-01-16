#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static int fd;

static void test_private_anon(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	tmp = *buf;
	*buf = 0x1;
	munmap(buf, 4096);
}

static void test_share_anon(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	tmp = *buf;
	*buf = 0x2;
	munmap(buf, 4096);
}

static void test_private_file(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FILE, fd, 0);
	tmp = *buf;
	*buf = 'a';
	munmap(buf, 4096);
}

static void test_share_file(void)
{
	char *buf;
	char tmp;

	buf = mmap(0, 4096, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_FILE, fd, 0);
	tmp = *buf;
	*buf = 'b';
	munmap(buf, 4096);
}

int main(int argc, char *argv[])
{
	fd = open("testfile", O_RDWR | O_CREAT, 0666);
	write(fd, "testtext", 8);

	test_private_anon();
	test_share_anon();
	test_private_file();
	test_share_file();

	close(fd);

	return 0;
}
