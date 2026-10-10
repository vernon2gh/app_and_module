#!/bin/bash

## watch -n 1 'cat /proc/meminfo | grep -iE "active|Free"'     ## system memory info
## watch -n 1 'cat /sys/kernel/debug/lru_gen | grep tmux -A 6' ## aging/evict info

SWAPFILE=~/extswapfile

if [ ! -e $SWAPFILE ]; then
	fallocate -l 4G $SWAPFILE
	chmod 0600 $SWAPFILE
	mkswap $SWAPFILE
fi

swapon $SWAPFILE
dmesg -C

# stress-ng --vm 2 --vm-bytes 5G --vm-hang 600 &
./a.out &
dd if=/dev/urandom of=~/testfile bs=1M count=3000
## echo "+ 44 0 4" > /sys/kernel/debug/lru_gen ## aging
## echo "- 44 0 1" > /sys/kernel/debug/lru_gen ## evict

cat /proc/vmstat | grep -E "pgskip|pgrefill|pgscan|pgsteal" > origin_vmstat
insmod mod.ko
cat /proc/vmstat | grep -E "pgskip|pgrefill|pgscan|pgsteal" > access_vmstat

../common/compare_values origin_vmstat access_vmstat
dmesg
rmmod mod
rm -fr origin_* access_*
killall a.out
# killall stress-ng
swapoff $SWAPFILE
