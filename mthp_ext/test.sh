#!/bin/bash

cpupower frequency-set -g performance &> /dev/null

eBPF=$(pwd)
CGROUP=/sys/fs/cgroup/mthp
mkdir -p $CGROUP

function test_simply()
{
	make
	echo 64M > $CGROUP/memory.high
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$2" = "ebpf" ]; then
		$eBPF/mthp_ext -d &
	fi

	perf stat -e page-faults -- ./a.out

	if [ "$2" = "ebpf" ]; then
		killall mthp_ext
	fi
	echo max > $CGROUP/memory.high
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
	echo $3 > $CGROUP/memory.high
	echo $(pidof redis-server) > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$4" = "ebpf" ]; then
		$eBPF/mthp_ext &
	fi

	echo 3 > /proc/sys/vm/drop_caches
	redis-benchmark --csv -r 3000000 -n 3000000 -d 1024 -c 16 -P 32 -t set

	if [ "$4" = "ebpf" ]; then
		killall mthp_ext
	fi
	echo max > $CGROUP/memory.high
	redis-cli SHUTDOWN NOSAVE
}

function test_stream()
{
	sleep 60
	make
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$2" = "ebpf" ]; then
		$eBPF/mthp_ext &
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
	UB=~/UnixBench

	sleep 60
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e $1
	if [ "$2" = "ebpf" ]; then
		$eBPF/mthp_ext &
	fi

	cd $UB
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

test_redis always noBGSAVE max
test_redis never  noBGSAVE max
test_redis always noBGSAVE max ebpf

test_redis always BGSAVE max
test_redis never  BGSAVE max
test_redis always BGSAVE max ebpf

test_redis always noBGSAVE 2G
test_redis never  noBGSAVE 2G
test_redis always noBGSAVE 2G ebpf

test_redis always BGSAVE 2G
test_redis never  BGSAVE 2G
test_redis always BGSAVE 2G ebpf

## test_stream always
## test_stream never
## test_stream always ebpf

## test_unixbench always
## test_unixbench never
## test_unixbench always ebpf
