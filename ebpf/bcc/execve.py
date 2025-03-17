#!/usr/bin/python

# use 'apt install python3-bpfcc',
# 'pip install bcc' not work
from bcc import BPF

kernel_prog = """
int handle_execve(void *ctx)
{
    bpf_trace_printk("Hello bcc!");
    return 0;
}
"""

b = BPF(text=kernel_prog)
b.attach_kprobe(event=b.get_syscall_fnname("execve"), fn_name="handle_execve")
b.trace_print()
