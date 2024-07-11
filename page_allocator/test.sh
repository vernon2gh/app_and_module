#!/bin/bash

cat /proc/meminfo  > origin_meminfo.txt

insmod mod.ko demo_entry="$1"

cat /proc/meminfo  > access_meminfo.txt

rmmod mod.ko

../common/compare_values origin_meminfo.txt access_meminfo.txt 2 100000

rm -fr origin_*.txt access_*.txt
