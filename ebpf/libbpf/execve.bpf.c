#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int handle_execve(struct pt_regs *ctx)
{
	bpf_printk("Hello libbpf.");

	return 0;
}

char LICENSE[] SEC("license") = "GPL";
