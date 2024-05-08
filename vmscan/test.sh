#!/bin/bash

if [[ $1 == "read" || $1 == "write" || $1 == "mmap_read" || $1 == "mmap_write" ]]; then
	echo "==== create testfile ===="
	fallocate -l 100M testfile
	cat /proc/meminfo | grep "file"

	echo "==== first $1 ===="
	./a.out $1
	cat /proc/meminfo | grep "file"

	echo "==== second $1 ===="
	./a.out $1
	cat /proc/meminfo | grep "file"

	rm -fr testfile
fi
