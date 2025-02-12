#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

struct dentry *xxx_debugfs_dir;
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
DEFINE_SHOW_STORE_ATTRIBUTE(test);

static int __init debugfs_init(void)
{
	if (debugfs_initialized()) {
		xxx_debugfs_dir = debugfs_create_dir("xxx", NULL);
		debugfs_create_file("test", 0666, xxx_debugfs_dir, NULL,
				    &test_fops);
	}

	return 0;
}

static void __exit debugfs_exit(void)
{
	debugfs_remove(xxx_debugfs_dir);
}

module_init(debugfs_init);
module_exit(debugfs_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("DEBUGFS MODULE");
MODULE_LICENSE("GPL");
