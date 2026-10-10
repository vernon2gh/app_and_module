#!/bin/bash

SWAP=$1
DEMO=$2

if [ $SWAP = "swapfile" ] || [ $SWAP = "zswap" ]; then
	SWAPDEV=~/swapfile
	fallocate -l 100M $SWAPDEV
	chmod 0600 $SWAPDEV
	mkswap $SWAPDEV &> /dev/null
	if [ $SWAP = "zswap" ]; then
		echo 1 > /sys/module/zswap/parameters/enabled
	fi
elif [ $SWAP = "zram" ]; then
	SWAPDEV=/dev/zram0
	## echo /dev/sdb > /sys/block/zram0/backing_dev
	echo 100M > /sys/block/zram0/disksize
	mkswap $SWAPDEV &> /dev/null
fi

swapon $SWAPDEV

cat /proc/meminfo  > origin_meminfo
cat /proc/vmstat   > origin_vmstat
./a.out $DEMO
cat /proc/meminfo  > access_meminfo
cat /proc/vmstat   > access_vmstat

if [ $SWAP = "zram" ] && [ "$(cat /sys/block/zram0/backing_dev)" != "none" ]; then
	cat /sys/kernel/debug/zram/zram0/block_state	## default block state
	echo all > /sys/block/zram0/idle		## mark all page to idle flags
	cat /sys/kernel/debug/zram/zram0/block_state
	echo idle > /sys/block/zram0/writeback		## writeback idle page to backing device
	cat /sys/kernel/debug/zram/zram0/block_state
fi

swapoff $SWAPDEV

../common/compare_values origin_meminfo access_meminfo -a
../common/compare_values origin_vmstat  access_vmstat  -a

if [ $SWAP = "swapfile" ] || [ $SWAP = "zswap" ]; then
	rm -fr $SWAPDEV
	if [ $SWAP = "zswap" ]; then
		echo 0 > /sys/module/zswap/parameters/enabled
	fi
fi
rm -fr origin* access*
