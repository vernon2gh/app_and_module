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
	echo 100M > /sys/block/zram0/disksize
	mkswap $SWAPDEV &> /dev/null
fi

if [ -n "$SWAPDEV" ]; then
	swapon $SWAPDEV
fi

echo "swap-in 各阶段平均耗时"
echo
echo "基线: 不挂探针"
./a.out $DEMO
echo
echo "内核: 挂探针"
bpftrace -c "./a.out $DEMO" ./swapin.bt

if [ -n "$SWAPDEV" ]; then
	swapoff $SWAPDEV
fi

if [ $SWAP = "swapfile" ] || [ $SWAP = "zswap" ]; then
	rm -fr $SWAPDEV
	if [ $SWAP = "zswap" ]; then
		echo 0 > /sys/module/zswap/parameters/enabled
	fi
fi
