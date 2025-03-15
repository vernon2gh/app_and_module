#include <stdio.h>
#include <bpf/libbpf.h>
#include "execve.skel.h"

int main(int argc, char **argv)
{
	struct execve_bpf *skel;
	int err;

	/* Open BPF application */
	skel = execve_bpf__open();
	if (!skel) {
		fprintf(stderr, "Failed to open BPF skeleton\n");
		return 1;
	}

	/* Load & verify BPF programs */
	err = execve_bpf__load(skel);
	if (err) {
		fprintf(stderr, "Failed to load and verify BPF skeleton\n");
		goto cleanup;
	}

	/* Attach tracepoint handler */
	err = execve_bpf__attach(skel);
	if (err) {
		fprintf(stderr, "Failed to attach BPF skeleton\n");
		goto cleanup;
	}

	err = system("date");
	printf("Please run `sudo cat /sys/kernel/tracing/trace_pipe` "
	       "to see output of the BPF programs.\n");

cleanup:
	execve_bpf__destroy(skel);
	return -err;
}
