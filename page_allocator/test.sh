#!/bin/bash

cat /proc/meminfo  > origin_meminfo.txt
insmod mod.ko demo_entry="$1"
cat /proc/meminfo  > access_meminfo.txt

../common/compare_values origin_meminfo.txt access_meminfo.txt 2 100000

rmmod mod.ko
rm -fr *.txt
