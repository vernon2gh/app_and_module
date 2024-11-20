#!/bin/bash

if [ $1 == "alloc_page_4KB_cache" ]; then
	x86_64_ptdump=/sys/kernel/debug/page_tables/kernel
	arm64_ptdump=/sys/kernel/debug/kernel_page_tables
	if [ -f $x86_64_ptdump ]; then
		file=$x86_64_ptdump
	elif [ -f $arm64_ptdump ]; then
		file=$arm64_ptdump
	else
		echo "Can't dump page table."
		exit
	fi

	insmod mod.ko demo_entry="$1"
	dmesg | grep "buf\[.*\]" | awk -F' ' '{print $4}' > buf_addr.txt

	while read addr; do
		./check_addr.sh $file $addr
	done < buf_addr.txt
else
	cat /proc/meminfo  > origin_meminfo.txt
	insmod mod.ko demo_entry="$1"
	cat /proc/meminfo  > access_meminfo.txt

	../common/compare_values origin_meminfo.txt access_meminfo.txt 2 100000
fi

rmmod mod.ko
rm -fr *.txt
