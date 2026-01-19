#!/bin/bash

BPF=$(pwd)/build/bin
UB=~/UnixBench

echo always > /sys/kernel/mm/transparent_hugepage/enabled

function test_a.out()
{
	## origin test
	perf stat -- ./a.out

	## thp adjust test
	$BPF/mthp_adjust &
	sleep 3
	perf stat -- ./a.out
	killall mthp_adjust
}

function test_stream()
{
	export OMP_NUM_THREADS=8

	## origin test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	./stream

	## thp adjust test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	$BPF/mthp_adjust &
	sleep 3
	./stream
	killall mthp_adjust
}

function test_unixbench()
{
	cd $UB

	## origin test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	./Run -c 1 shell8

	## thp adjust test
	sleep 60
	echo 3 > /proc/sys/vm/drop_caches
	$BPF/mthp_adjust &
	sleep 3
	./Run -c 1 shell8
	killall mthp_adjust

	cd -
}

make

test_a.out
## test_stream
## test_unixbench

make clean
