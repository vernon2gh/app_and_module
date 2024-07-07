#!/bin/bash

CGROUP=/sys/fs/cgroup

mkdir -p $CGROUP/$1
echo $$ > $CGROUP/$1/cgroup.procs

case $1 in
	"memory_max")
		echo 10M > $CGROUP/$1/memory.max
		./a.out $1
		;;
	"memory_reclaim")
		fallocate -l 100M testfile
		vmtouch -t testfile >> /dev/null

		echo "==== origin meminfo ===="
		cat /proc/meminfo | grep "file"

		echo 100M > $CGROUP/$1/memory.reclaim

		echo "==== meminfo after reclaim ===="
		cat /proc/meminfo | grep "file"

		rm -fr testfile
		;;
	*)
		echo "Don't look for right demo entry."
		;;
esac
