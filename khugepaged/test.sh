#!/bin/bash

echo madvise > /sys/kernel/mm/transparent_hugepage/enabled
echo 1 > /sys/kernel/mm/transparent_hugepage/khugepaged/max_ptes_none
make
sleep 60

./a.out hot1 &
sleep 1
./a.out cold &
sleep 1
./a.out hot2
## perf stat -d -d -d -- ./a.out hot2

make clean
