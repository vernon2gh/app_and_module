#!/bin/bash

SWAPFILE=~/extswapfile

if [ ! -e $SWAPFILE ]; then
	fallocate -l 20M $SWAPFILE
	chmod 0600 $SWAPFILE
	mkswap $SWAPFILE
fi

swapon $SWAPFILE

cat /proc/vmstat   > origin_vmstat.txt
./a.out $1
cat /proc/vmstat   > access_vmstat.txt

swapoff $SWAPFILE
cat /sys/kernel/tracing/trace > trace.txt
../common/compare_values origin_vmstat.txt access_vmstat.txt 2 10000
