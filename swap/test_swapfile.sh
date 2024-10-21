#!/bin/bash

SWAPFILE=~/extswapfile

if [ ! -e $SWAPFILE ]; then
	fallocate -l 20M $SWAPFILE
	chmod 0600 $SWAPFILE
	mkswap $SWAPFILE
fi

swapon $SWAPFILE

./a.out $1

swapoff $SWAPFILE
cat /sys/kernel/tracing/trace > trace.txt
cat /proc/vmstat | grep swp
