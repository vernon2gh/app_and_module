#!/bin/bash

CGROUP=/sys/fs/cgroup

mkdir -p $CGROUP/$1
echo $$ > $CGROUP/$1/cgroup.procs

case $1 in
	"memory_max")
		echo 10M > $CGROUP/$1/memory.max
		dd if=/dev/random of=testfile bs=1M count=11 &> /dev/null
		;;
	"memory_reclaim")
		fallocate -l 100M testfile
		vmtouch -t testfile >> /dev/null

		cat /proc/meminfo  > origin_meminfo.txt
		echo 100M > $CGROUP/$1/memory.reclaim
		cat /proc/meminfo  > access_meminfo.txt

		../common/compare_values origin_meminfo.txt access_meminfo.txt 2 100000
		;;
	"dirty_memory_reclaim")
		dd if=/dev/random of=testfile bs=1M count=100 &> /dev/null

		cat /proc/meminfo  > origin_meminfo.txt
		cat /proc/vmstat   > origin_vmstat.txt
		echo 100M > $CGROUP/$1/memory.reclaim
		cat /proc/meminfo  > access_meminfo.txt
		cat /proc/vmstat   > access_vmstat.txt

		../common/compare_values origin_meminfo.txt access_meminfo.txt 2 10000
		../common/compare_values origin_vmstat.txt  access_vmstat.txt  2 2560
		;;
	"kswapd")
		echo 10M > $CGROUP/$1/memory.high

		cat /proc/meminfo  > origin_meminfo.txt
		dd if=/dev/random of=testfile bs=1M count=100 &> /dev/null
		cat /proc/meminfo  > access_meminfo.txt

		../common/compare_values origin_meminfo.txt access_meminfo.txt 2 10000
		cat /proc/vmstat | grep -E 'vmscan'
		cat /proc/`pidof kswapd0`/stat
		;;
	*)

		echo "Don't look for right demo entry."
		;;
esac

rm -fr *.txt testfile
