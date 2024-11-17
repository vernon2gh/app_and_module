#!/bin/bash

DIR=$1

if [ ! $DIR ]; then
	DIR=time_real
fi

mkdir $DIR
echo > $DIR/total.txt

function demo_three() {
	echo "=========== $1 $2 ===========" >> $DIR/total.txt
	for i in {1..3}; do
		./test.sh memory_reclaim_swappiness $2 &>> $DIR/total.txt

		mkdir -p $DIR/$1/$i
		mv *.txt $DIR/$1/$i
		sync
		sleep 5
	done
}

function demo_run() {
	if [ $1 != "default" ]; then
		echo always > /sys/kernel/mm/transparent_hugepage/hugepages-$1kB/enabled
	fi

	demo_three $1
	## demo_three $1 no_cache_trim

	if [ $1 != "default" ]; then
		echo never > /sys/kernel/mm/transparent_hugepage/hugepages-$1kB/enabled
	fi
}

demo_run default
demo_run 2048
demo_run 1024
demo_run 512
demo_run 256
demo_run 128
demo_run 64
demo_run 32
demo_run 16
## demo_run 8
