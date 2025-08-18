#include <linux/module.h>
#include <linux/swap.h>

static int __init demo_init(void)
{
	unsigned long nr_to_reclaim = (1024/4 * 1024 * 6);
	unsigned long nr_reclaimed;

	pr_info("%s: %d\n", __func__, __LINE__);

	nr_reclaimed = shrink_all_memory(nr_to_reclaim);
	pr_info("%s: want %lu(%luMB), real %lu(%luMB)\n",
		__func__, nr_to_reclaim, nr_to_reclaim*4/1024,
			nr_reclaimed, nr_reclaimed*4/1024);

	return 0;
}

static void __exit demo_exit(void)
{
	pr_info("%s: %d\n", __func__, __LINE__);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("DEMO MODULE");
MODULE_LICENSE("GPL");
