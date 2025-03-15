#include <stdio.h>
#include <stdlib.h>
#include <bpf/libbpf.h>

static struct bpf_link **glink;
static int nr_glink;

static inline int bpf_object__attach(struct bpf_object *obj)
{
	struct bpf_program *prog;
	int i = 0;

	bpf_object__for_each_program(prog, obj)
		nr_glink++;

	glink = malloc(nr_glink * sizeof(struct bpf_link *));
	memset(glink, 0, nr_glink * sizeof(struct bpf_link *));

	bpf_object__for_each_program(prog, obj) {
		glink[i] = bpf_program__attach(prog);
		if (libbpf_get_error(glink[i]))
			return -1;
		i++;
	}

	return 0;
}

static inline void bpf_object__destroy(struct bpf_object *obj)
{
	int i = 0;

	for (i = 0; i < nr_glink; i++)
		bpf_link__destroy(glink[i]);
	free(glink);

	bpf_object__close(obj);
}

int main(int ac, char **argv)
{
	struct bpf_object *obj;
	char filename[256];
	int ret;

	snprintf(filename, sizeof(filename), "%s.bpf.o", argv[0]);

	obj = bpf_object__open(filename);
	if (libbpf_get_error(obj)) {
		fprintf(stderr, "ERROR: opening BPF object file failed\n");
		return 0;
	}

	ret = bpf_object__load(obj);
	if (ret) {
		fprintf(stderr, "ERROR: loading BPF object file failed\n");
		goto cleanup;
	}

	ret = bpf_object__attach(obj);
	if (ret) {
		fprintf(stderr, "ERROR: attach BPF object file failed\n");
		goto cleanup;
	}

	ret = system("date");
	printf("Please run `sudo cat /sys/kernel/tracing/trace_pipe` "
	       "to see output of the BPF programs.\n");

cleanup:
	bpf_object__destroy(obj);
	return 0;
}
