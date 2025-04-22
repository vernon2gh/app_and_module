#!/bin/bash


function info() {
	grep anon $1 | awk '{ sum+=$2; } END { printf "anon: %.0f MB\n", sum/1024 }'
	grep file $1 | awk '{ sum+=$2; } END { printf "file: %.0f MB\n", sum/1024 }'
	grep pgfault $2
}

fallocate -l 100M testfile

./a.out $1

echo "======= origin info ======="
info origin_meminfo.txt origin_vmstat.txt
echo "==== info after access ===="
info access_meminfo.txt access_vmstat.txt
echo "==== info after munmap ===="
info munmap_meminfo.txt munmap_vmstat.txt

rm -fr testfile *.txt
