#!/bin/bash

cat /proc/meminfo  > origin_meminfo.txt
cat /proc/slabinfo > origin_slabinfo.txt

insmod mod.ko demo_entry="$1"

cat /proc/meminfo  > access_meminfo.txt
cat /proc/slabinfo > access_slabinfo.txt

rmmod mod.ko

../common/compare_values origin_meminfo.txt access_meminfo.txt 2 100000
../common/compare_values origin_slabinfo.txt access_slabinfo.txt 2 25500

rm -fr origin_*.txt access_*.txt
