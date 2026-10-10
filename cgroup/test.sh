#!/bin/bash

TESTFILE=~/testfile
CGROUP=/sys/fs/cgroup

mkdir -p $CGROUP/$1
echo $$ > $CGROUP/$1/cgroup.procs

function color_echo() {
	echo -e "\e[32m$1\e[0m"
}

case $1 in
	"memory_max")
		echo 10M > $CGROUP/$1/memory.max
		dd if=/dev/random of=$TESTFILE bs=1M count=11 &> /dev/null
		;;
	"memory_reclaim")
		fallocate -l 100M $TESTFILE
		vmtouch -t $TESTFILE >> /dev/null

		cat /proc/meminfo  > origin_meminfo
		echo 100M > $CGROUP/$1/memory.reclaim
		cat /proc/meminfo  > access_meminfo

		../common/compare_values origin_meminfo access_meminfo
		;;
	"memory_reclaim_dirty")
		dd if=/dev/random of=$TESTFILE bs=1M count=100 &> /dev/null

		cat /proc/meminfo  > origin_meminfo
		cat /proc/vmstat   > origin_vmstat
		echo 100M > $CGROUP/$1/memory.reclaim
		cat /proc/meminfo  > access_meminfo
		cat /proc/vmstat   > access_vmstat

		color_echo "meminfo:"
		../common/compare_values origin_meminfo access_meminfo
		color_echo "vmstat:"
		../common/compare_values origin_vmstat  access_vmstat
		;;
	"memory_reclaim_swappiness")
		## allocate 100MB anon page
		./a.out $1 &

		if [ "$(cat /sys/block/zram0/disksize)" = "0" ]; then
			echo 100M > /sys/block/zram0/disksize
			mkswap /dev/zram0
		fi
		swapon /dev/zram0

		## allocate 100MB file page
		dd if=/dev/random of=$TESTFILE bs=1M count=100 &> /dev/null

		cat /proc/meminfo           > origin_meminfo
		cat /proc/vmstat            > origin_vmstat
		cat $CGROUP/$1/memory.stat  > origin_memory_stat
		echo "100M swappiness=200"  > $CGROUP/$1/memory.reclaim
		cat /proc/meminfo           > access_meminfo
		cat /proc/vmstat            > access_vmstat
		cat $CGROUP/$1/memory.stat  > access_memory_stat

		color_echo "meminfo:"
		../common/compare_values origin_meminfo access_meminfo
		color_echo "vmstat:"
		../common/compare_values origin_vmstat  access_vmstat
		color_echo "cgroup memory stat:"
		../common/compare_values origin_memory_stat access_memory_stat

		killall a.out
		swapoff /dev/zram0
		;;
	"kswapd")
		echo 10M > $CGROUP/$1/memory.high

		cat /proc/meminfo  > origin_meminfo
		cat /proc/vmstat   > origin_vmstat
		dd if=/dev/random of=$TESTFILE bs=1M count=100 &> /dev/null
		cat /proc/meminfo  > access_meminfo
		cat /proc/vmstat   > access_vmstat

		color_echo "meminfo:"
		../common/compare_values origin_meminfo access_meminfo
		color_echo "vmstat:"
		../common/compare_values origin_vmstat  access_vmstat
		color_echo "kswapd stat:"
		cat /proc/`pidof kswapd0`/stat
		;;
	*)

		echo "Don't look for right demo entry."
		;;
esac

rm -fr $TESTFILE
