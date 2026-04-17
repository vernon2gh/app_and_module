#!/bin/bash

cpupower frequency-set -g performance &> /dev/null

eBPF=$(pwd)
CGROUP=/sys/fs/cgroup/mthp
mkdir -p $CGROUP

function test_a.out()
{
	make
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e always

	## origin test
	perf stat -e page-faults -- ./a.out

	## mthp test
	$eBPF/mthp_ext -d -o 0 &
	perf stat -e page-faults -- ./a.out
	sleep 60
	killall mthp_ext

	make clean
}

function test_redis()
{
	sleep 60
	./mthp_set_show.sh -e $1
	if [ "$2" = "ebpf" ]; then
		$eBPF/mthp_ext -o 0 -r $CGROUP &
	fi

	sysctl -q vm.overcommit_memory=1
	redis-server --save "" --daemonize yes
	echo 2G > $CGROUP/memory.high
	echo $(pidof redis-server) > $CGROUP/cgroup.procs

	echo 3 > /proc/sys/vm/drop_caches
	redis-benchmark --csv -r 3000000 -n 3000000 -d 1024 -c 16 -P 32 -t set

	killall redis-server
	if [ "$2" = "ebpf" ]; then
		killall mthp_ext
	fi
}

function test_stream()
{
	make
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e always
	export OMP_NUM_THREADS=8

	## origin test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	sleep 60
	./stream

	## thp test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	$eBPF/mthp_ext &
	sleep 60
	./stream
	killall mthp_ext

	make clean
}

function test_unixbench()
{
	echo $$ > $CGROUP/cgroup.procs
	./mthp_set_show.sh -e always
	cd ~/UnixBench

	## origin test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	sleep 60
	./Run -c 1 shell8

	## thp test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	$eBPF/mthp_ext &
	sleep 60
	./Run -c 1 shell8
	killall mthp_ext

	cd -
}


## test_a.out
test_redis always
test_redis never
test_redis always ebpf
## test_stream
## test_unixbench
