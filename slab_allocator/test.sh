#!/bin/bash

cat /proc/meminfo  > origin_meminfo
cat /proc/slabinfo > origin_slabinfo

insmod mod.ko demo_entry="$1"

cat /proc/meminfo  > access_meminfo
cat /proc/slabinfo > access_slabinfo

rmmod mod.ko

../common/compare_values origin_meminfo access_meminfo
../common/compare_values origin_slabinfo access_slabinfo

rm -fr origin_* access_*
