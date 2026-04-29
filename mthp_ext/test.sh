#!/bin/bash

cpupower frequency-set -g performance &> /dev/null

eBPF=$(pwd)
CGROUP=/sys/fs/cgroup/mthp
rmdir    $CGROUP &> /dev/null
mkdir -p $CGROUP

function test_simply()
{
	make
	echo 64M > $CGROUP/memory.high
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$2" = "ebpf" ]; then
		$eBPF/mthp_ext -d -r $CGROUP &
	fi

	perf stat -e page-faults -- ./a.out

	if [ "$2" = "ebpf" ]; then
		killall mthp_ext
	fi
	make clean
}

function test_redis()
{
	sleep 60
	sysctl -q vm.overcommit_memory=1
	if [ "$2" = "BGSAVE" ]; then
		redis-server --daemonize yes
	else
		redis-server --save "" --daemonize yes
	fi
	echo 2G > $CGROUP/memory.high
	echo $(pidof redis-server) > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$3" = "ebpf" ]; then
		$eBPF/mthp_ext -r $CGROUP &
	fi

	echo 3 > /proc/sys/vm/drop_caches
	redis-benchmark --csv -r 3000000 -n 3000000 -d 1024 -c 16 -P 32 -t set

	if [ "$3" = "ebpf" ]; then
		killall mthp_ext
	fi
	redis-cli SHUTDOWN NOSAVE
}

function test_stream()
{
	sleep 60
	make
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$2" = "ebpf" ]; then
		$eBPF/mthp_ext -r $CGROUP &
	fi

	echo 3 > /proc/sys/vm/drop_caches
	export OMP_NUM_THREADS=8
	./stream | grep -E "Function|Copy|Scale|Add|Triad"

	if [ "$2" = "ebpf" ]; then
		killall mthp_ext
	fi
	make clean
}

function test_unixbench()
{
	sleep 60
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$2" = "ebpf" ]; then
		$eBPF/mthp_ext -r $CGROUP &
	fi

	cd ~/UnixBench
	echo 3 > /proc/sys/vm/drop_caches
	./Run -c 1 shell8 | grep "^System Benchmarks Index Score"
	cd - &> /dev/null

	if [ "$2" = "ebpf" ]; then
		killall mthp_ext
	fi
}


## test_simply always
## test_simply never
## test_simply always ebpf

test_redis always NONE
test_redis never  NONE
test_redis always NONE ebpf

test_redis always BGSAVE
test_redis never  BGSAVE
test_redis always BGSAVE ebpf

## test_stream always
## test_stream never
## test_stream always ebpf

## test_unixbench always
## test_unixbench never
## test_unixbench always ebpf
