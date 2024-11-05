#!/bin/bash

WB=false

if [ -e "/sys/block/zram0/backing_dev" ]; then
	WB=true
	echo /dev/sdb > /sys/block/zram0/backing_dev
fi

if [ "$(cat /sys/block/zram0/disksize)" = "0" ]; then
	echo 20M > /sys/block/zram0/disksize
	mkswap /dev/zram0
fi

swapon /dev/zram0

cat /proc/vmstat   > origin_vmstat.txt
./a.out $1
cat /proc/vmstat   > access_vmstat.txt

if [ $WB = "true" ]; then
	## default block state
	cat /sys/kernel/debug/zram/zram0/block_state

	## mark all page to idle flags
	echo all > /sys/block/zram0/idle
	cat /sys/kernel/debug/zram/zram0/block_state

	## writeback idle page to backing device
	echo idle > /sys/block/zram0/writeback
	cat /sys/kernel/debug/zram/zram0/block_state
fi

swapoff /dev/zram0
cat /sys/kernel/tracing/trace > trace.txt
../common/compare_values origin_vmstat.txt access_vmstat.txt 2 10000
