#!/bin/bash

cat /proc/meminfo  > origin_meminfo
insmod mod.ko demo_entry="$1"
cat /proc/meminfo  > access_meminfo

../common/compare_values origin_meminfo access_meminfo

rmmod mod.ko
rm -fr origin_* access_*
