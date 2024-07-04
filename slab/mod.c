#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#define TATOL_SIZE	(100 * 1024 * 1024)
#define SIZE_4KB	(PAGE_SIZE)
#define SIZE_4MB	(PAGE_SIZE * MAX_ORDER_NR_PAGES)

void **buf;

static void test_kmalloc(unsigned int per_size)
{
	int nr = TATOL_SIZE / per_size;
	int i;

	buf = kmalloc(nr * sizeof(void *), GFP_KERNEL);

	for (i = 0; i < nr; i++)
		buf[i] = kmalloc(per_size, GFP_KERNEL);
}

static void test_kfree(unsigned int per_size)
{
	int nr = TATOL_SIZE / per_size;
	int i;

	for (i = 0; i < nr; i++)
		kfree(buf[i]);

	kfree(buf);
}

enum demo_entry {
	DEMO_4KB,
	DEMO_4MB,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"4KB",
	"4MB",
};

static enum demo_entry test_demo_entry(char *entry)
{
	int i;

	if (!entry)
		return DEMO_4KB;

	for (i = 0; i < DEMO_ENTRY_MAX; i++) {
		if (!strncmp(entry, string[i], strlen(string[i])))
			return i;
	}

	pr_info("Don't look for right demo entry.\n");
	return -EINVAL;
}

static char *demo_entry = "4KB";
module_param(demo_entry, charp, 0444);
MODULE_PARM_DESC(demo_entry, "A demo entry string");

static int __init slab_init(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_4KB:
			test_kmalloc(SIZE_4KB);
			break;
		case DEMO_4MB:
			test_kmalloc(SIZE_4MB);
			break;
	}

	return 0;
}

static void __exit slab_exit(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_4KB:
			test_kfree(SIZE_4KB);
			break;
		case DEMO_4MB:
			test_kfree(SIZE_4MB);
			break;
	}
}

module_init(slab_init);
module_exit(slab_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("SLAB MODULE");
MODULE_LICENSE("GPL");
