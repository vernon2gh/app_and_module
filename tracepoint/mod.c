#include <linux/module.h>
#include <linux/kthread.h>

#define CREATE_TRACE_POINTS
#include "testtracepoint.h"

static struct task_struct *test_task;

static int test_func(void *arg)
{
	int id = 0;

	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ);

		trace_test(id);
		id++;
	}

	return 0;
}

static int __init tracepoint_init(void)
{
	pr_info("%s: %d\n", __func__, __LINE__);

	test_task = kthread_run(test_func, NULL, "test_thread");
	if (IS_ERR(test_task))
		return -1;

	return 0;
}

static void __exit tracepoint_exit(void)
{
	pr_info("%s: %d\n", __func__, __LINE__);

	kthread_stop(test_task);
}

module_init(tracepoint_init);
module_exit(tracepoint_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("TRACEPOINT MODULE");
MODULE_LICENSE("GPL");
