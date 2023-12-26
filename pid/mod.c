#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched/signal.h>

static int app_pid;
module_param(app_pid, int, 0644);

static int print_ppid_tid(int input_pid)
{
	struct pid *pid, *ppid;
	struct task_struct *task, *tmp;

	pid = find_get_pid(input_pid);
	task = pid_task(pid, PIDTYPE_PID);
	ppid = get_task_pid(task->parent, PIDTYPE_PID);

	pr_info("parent pid: %d\n", pid_nr(ppid));

	tmp = task;
	while((tmp = next_thread(tmp)) != task) {
		pid = get_task_pid(tmp, PIDTYPE_PID);
		pr_info("thread pid: %d\n", pid_nr(pid));
	}

	return 0;
}

static int __init pid_init(void)
{
	pr_info("%s: %d\n", __func__, __LINE__);
	print_ppid_tid(app_pid);

	return 0;
}

static void __exit pid_exit(void)
{
	pr_info("%s: %d\n", __func__, __LINE__);
}

module_init(pid_init);
module_exit(pid_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("PID MODULE");
MODULE_LICENSE("GPL");
