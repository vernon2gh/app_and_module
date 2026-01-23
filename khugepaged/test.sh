#!/bin/bash

echo madvise > /sys/kernel/mm/transparent_hugepage/enabled
make
sleep 60

./a.out hot1 &
sleep 1
./a.out cold &
sleep 1
./a.out hot2
## perf stat -d -d -d -- ./a.out hot2

make clean
