#include <linux/module.h>

static int __init demo_init(void)
{
	pr_info("%s: %d\n", __func__, __LINE__);

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
