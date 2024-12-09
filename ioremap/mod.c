#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <asm/set_memory.h>
#include <asm/io.h>

#define NR_TESTS	8
static void *buf[NR_TESTS];

static char *demo_entry = "ioremap_reserve_mem";
module_param(demo_entry, charp, 0444);
MODULE_PARM_DESC(demo_entry, "A demo entry string");

enum demo_entry {
	DEMO_IOREMAP_RESERVE_MEM,
	DEMO_IOREMAP_NORMAL_MEM,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"ioremap_reserve_mem",
	"ioremap_normal_mem",
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

static void test_ioremap_from_normal_memory(unsigned int order, gfp_t flags)
{
	int i = 0;
	int tmp;
	void *addr;

	buf[i] = alloc_pages(flags, order);
	i++;

#ifdef CONFIG_X86_64
	buf[i] = alloc_pages(flags, order);
	set_pages_uc(buf[i], 1 << order);
	i++;

	buf[i] = alloc_pages(flags, order);
	set_pages_wb(buf[i], 1 << order);
	i++;
#endif

	buf[i] = alloc_pages(flags, order);
	addr = ioremap(page_to_phys((struct page *)buf[i]), PAGE_SIZE);
	if (addr == NULL)
		pr_info("ioremap failed.\n");
	i++;

	buf[i] = alloc_pages(flags, order);
	addr = ioremap_cache(page_to_phys((struct page *)buf[i]), PAGE_SIZE);
	if (addr == NULL)
		pr_info("ioremap_cache failed.\n");
	i++;

	buf[i] = alloc_pages(flags, order);
	addr = ioremap_uc(page_to_phys((struct page *)buf[i]), PAGE_SIZE);
	if (addr == NULL)
		pr_info("ioremap_uc failed.\n");
	i++;

	buf[i] = alloc_pages(flags, order);
	addr = ioremap_wc(page_to_phys((struct page *)buf[i]), PAGE_SIZE);
	if (addr == NULL)
		pr_info("ioremap_wc failed.\n");
	i++;

	buf[i] = alloc_pages(flags, order);
	addr = ioremap_wt(page_to_phys((struct page *)buf[i]), PAGE_SIZE);
	if (addr == NULL)
		pr_info("ioremap_wt failed.\n");
	i++;

	WARN_ON(i > NR_TESTS);

	for (tmp = i, i = 0; i < tmp; i++)
		pr_info("buf[%d] 0x%lx\n", i, (unsigned long)page_address(buf[i]));
}

static void test_iounmap_from_normal_memory(unsigned int order)
{
	int i;

	for (i = 0; i < NR_TESTS; i++) {
		if (buf[i])
			__free_pages(buf[i], order);
	}
}

/*
 * reserve memory range: [2G, 2G+100M]
 *
 * 1. arm64 device tree, e.g.
 *
 * reserved-memory {
 * 	#size-cells = <0x02>;
 * 	#address-cells = <0x02>;
 * 	ranges;
 *
 * 	buf1: buf@80000000 {
 * 		reg = <0x00 0x80000000 0x00 0x6400000>;
 * 		no-map;
 * 	};
 * };
 *
 * 2. x86_64 bootargs, e.g.
 *
 * memmap=100M$2G or memmap=0x6400000$0x80000000
 */
static void test_ioremap_from_reserve_memory(void)
{
	int i = 0;
	int tmp;

	buf[i] = alloc_pages(GFP_KERNEL, 0);
	pr_info("buf[%d] 0x%lx\n", i, (unsigned long)page_address(buf[i]));
	i++;

	buf[i++] = ioremap(0x80000000, PAGE_SIZE);
	buf[i++] = ioremap_cache(0x80001000, PAGE_SIZE);
	buf[i++] = ioremap_uc(0x80002000, PAGE_SIZE);
	buf[i++] = ioremap_wc(0x80003000, PAGE_SIZE);
	buf[i++] = ioremap_wt(0x80004000, PAGE_SIZE);

	WARN_ON(i > NR_TESTS);

	for (tmp = i, i = 1; i < tmp; i++)
		pr_info("buf[%d] 0x%lx\n", i, (unsigned long)buf[i]);
}

static void test_iounmap_from_reserve_memory(void)
{
	int i;

	__free_pages(buf[0], 0);

	for (i = 1; i < NR_TESTS; i++) {
		if (buf[i])
			iounmap(buf[i]);
	}
}

static int __init ioremap_init(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_IOREMAP_RESERVE_MEM:
			test_ioremap_from_reserve_memory();
			break;
		case DEMO_IOREMAP_NORMAL_MEM:
			test_ioremap_from_normal_memory(0, GFP_KERNEL);
			break;
	}

	return 0;
}

static void __exit ioremap_exit(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_IOREMAP_RESERVE_MEM:
			test_iounmap_from_reserve_memory();
			break;
		case DEMO_IOREMAP_NORMAL_MEM:
			test_iounmap_from_normal_memory(0);
			break;
	}
}

module_init(ioremap_init);
module_exit(ioremap_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("IOREMAP MODULE");
MODULE_LICENSE("GPL");
