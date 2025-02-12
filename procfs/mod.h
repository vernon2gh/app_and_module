#ifndef _EXTRA_MOD_H
#define _EXTRA_MOD_H

#include <linux/proc_fs.h>

#define DEFINE_PROC_SHOW_STORE_ATTRIBUTE(__name)			\
static int __name ## _open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, __name ## _show, pde_data(inode));	\
}									\
									\
static const struct proc_ops __name ## _proc_ops = {			\
	.proc_open	= __name ## _open,				\
	.proc_read	= seq_read,					\
	.proc_write	= __name ## _write,				\
	.proc_lseek	= seq_lseek,					\
	.proc_release	= single_release,				\
}

#endif /* _EXTRA_MOD_H */
