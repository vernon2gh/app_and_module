#include <linux/module.h>
#include <linux/seq_file.h>
#include "mod.h"

static struct proc_dir_entry *xxx_procfs_dir;
static int test;

static int test_show(struct seq_file *s, void *data)
{
	seq_printf(s, "test %d\n", test);

	return 0;
}

static ssize_t test_write(struct file *f, const char __user *ubuf, size_t len,
			loff_t *offset)
{
	int ret;

	ret = kstrtoint_from_user(ubuf, len, 0, &test);
	if (ret)
		pr_err("strtoint failed. ret = %d\n", ret);

	return len;
}
DEFINE_PROC_SHOW_STORE_ATTRIBUTE(test);

static int __init procfs_init(void)
{
	xxx_procfs_dir = proc_mkdir("xxx", NULL);
	proc_create("test", 0666, xxx_procfs_dir, &test_proc_ops);

	return 0;
}

static void __exit procfs_exit(void)
{
	proc_remove(xxx_procfs_dir);
}

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("PROCFS MODULE");
MODULE_LICENSE("GPL");
