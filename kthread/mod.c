#include <linux/module.h>
#include <linux/kthread.h>

struct task_struct *kxxxd;
static struct wait_queue_head wq;
static int flag;

static void test_wakeup(void)
{
	// Wake up kxxxd at the moment of necessity
	if (flag == 0) {
		flag = 1;
		wake_up_interruptible(&wq);
	}
}

static void test_working(void)
{
	// TODO
	pr_info("hello world\n");
}

static int kxxxd_func(void *arg)
{
	while (!kthread_should_stop()) {
		if (wait_event_interruptible(wq, flag == 1))
			continue;

		test_working();

		flag = 0;
	}

	return 0;
}

static int __init kernel_thread_init(void)
{
	init_waitqueue_head(&wq);
	kxxxd = kthread_run(kxxxd_func, NULL, "kxxxd");
	if (IS_ERR(kxxxd)) {
		pr_err("Failed to start kxxxd，ret=%ld\n", PTR_ERR(kxxxd));
		return -EINVAL;
	}

	test_wakeup();

	return 0;
}

static void __exit kernel_thread_exit(void)
{
	kthread_stop(kxxxd);
}

module_init(kernel_thread_init);
module_exit(kernel_thread_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("KTHREAD MODULE");
MODULE_LICENSE("GPL");
