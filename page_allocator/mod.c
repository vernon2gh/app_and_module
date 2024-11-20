#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <asm/set_memory.h>

#define TATOL_SIZE	(100 * 1024 * 1024)
#define ORDER_4KB	0
#define ORDER_4MB	MAX_PAGE_ORDER

static void **buf;

static char *demo_entry = "alloc_page_4KB";
module_param(demo_entry, charp, 0444);
MODULE_PARM_DESC(demo_entry, "A demo entry string");

enum demo_entry {
	DEMO_ALLOC_PAGE_4KB,
	DEMO_ALLOC_PAGE_4KB_RECLAIM,
	DEMO_ALLOC_PAGE_4KB_CACHE,
	DEMO_ALLOC_PAGE_4MB,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"alloc_page_4KB",
	"alloc_page_4KB_reclaim",
	"alloc_page_4KB_cache",
	"alloc_page_4MB",
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

	pr_info("Don't look for right demo entry.\n");
	return -EINVAL;
}

static void test_alloc_page(unsigned int order, gfp_t flags)
{
	int size = PAGE_SIZE * (1 << order);
	int nr = TATOL_SIZE / size;
	int i;

	buf = kmalloc(nr * sizeof(void *), GFP_KERNEL);

	for (i = 0; i < nr; i++)
		buf[i] = alloc_pages(flags, order);
}

static void test_alloc_page_cache(unsigned int order, gfp_t flags)
{
	int size = PAGE_SIZE * (1 << order);
	int nr = TATOL_SIZE / size;
	int i = 0;
	int tmp;

	buf = kmalloc(nr * sizeof(void *), GFP_KERNEL|__GFP_ZERO);

	buf[i] = alloc_pages(flags, order);
	i++;

	buf[i] = alloc_pages(flags, order);
	set_pages_uc(buf[i], 1 << order);
	i++;

	buf[i] = alloc_pages(flags, order);
	set_pages_wb(buf[i], 1 << order);
	i++;

	for (tmp = i, i = 0; i < tmp; i++)
		pr_info("buf[%d] 0x%lx\n", i, (unsigned long)page_address(buf[i]));
}

static void test_free_page(unsigned int order)
{
	int size = PAGE_SIZE * (1 << order);
	int nr = TATOL_SIZE / size;
	int i;

	for (i = 0; i < nr; i++) {
		if (buf[i])
			__free_pages(buf[i], order);
	}

	kfree(buf);
}

static int __init page_init(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_ALLOC_PAGE_4KB:
			test_alloc_page(ORDER_4KB, GFP_KERNEL);
			break;
		case DEMO_ALLOC_PAGE_4KB_RECLAIM:
			pr_warn("__GFP_RECLAIMABLE is invaild for page allocator, "
				"only vaild for slab allocator kmalloc(size <= 8KB)\n");
			test_alloc_page(ORDER_4KB, GFP_KERNEL | __GFP_RECLAIMABLE);
			break;
		case DEMO_ALLOC_PAGE_4KB_CACHE:
			test_alloc_page_cache(ORDER_4KB, GFP_KERNEL);
			break;
		case DEMO_ALLOC_PAGE_4MB:
			test_alloc_page(ORDER_4MB, GFP_KERNEL);
			break;
	}

	return 0;
}

static void __exit page_exit(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_ALLOC_PAGE_4KB:
		case DEMO_ALLOC_PAGE_4KB_RECLAIM:
		case DEMO_ALLOC_PAGE_4KB_CACHE:
			test_free_page(ORDER_4KB);
			break;
		case DEMO_ALLOC_PAGE_4MB:
			test_free_page(ORDER_4MB);
			break;
	}
}

module_init(page_init);
module_exit(page_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("PAGE ALLOCATOR MODULE");
MODULE_LICENSE("GPL");
