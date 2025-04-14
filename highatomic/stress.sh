#!/bin/bash

MEMSIZE=$(cat /proc/meminfo | grep "MemTotal" | awk '{ print $2 }')K

## memory reclaim watermark
echo 6758 > /proc/sys/vm/min_free_kbytes
echo 1    > /proc/sys/vm/watermark_scale_factor

## the all system memory are filled to anon page
stress-ng -vm 1 --vm-bytes $MEMSIZE &
