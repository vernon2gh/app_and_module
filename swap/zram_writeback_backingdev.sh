#!/bin/bash

echo /dev/sdb > /sys/block/zram0/backing_dev
# fallocate -l 20M extswapfile
# echo ./extswapfile > /sys/block/zram0/backing_dev

echo 20M > /sys/block/zram0/disksize
mkswap /dev/zram0
swapon /dev/zram0

cat /proc/swaps
# Filename				Type		Size		Used		Priority
# /dev/zram0                              partition	20476		0		-2
gcc app.c
./a.out &
cat /proc/swaps
# Filename				Type		Size		Used		Priority
# /dev/zram0                              partition	20476		256		-2

cat /sys/kernel/debug/zram/zram0/block_state
#            0           15.823322 ......
#          256           17.017736 ......
echo all > /sys/block/zram0/idle
cat /sys/kernel/debug/zram/zram0/block_state
#            0           15.823322 ...i..
#          256           17.017736 ...i..
echo idle > /sys/block/zram0/writeback
cat /sys/kernel/debug/zram/zram0/block_state
#            0            0.000000 .w....
#          256            0.000000 .w....
