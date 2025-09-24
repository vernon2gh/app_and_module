#!/bin/bash

PERF='perf stat -e page-faults,cache-misses,cache-references --'

echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/shmem_enabled

function stats() {
	if [ ! -z "$PERF" ]; then
		return
	fi

	cd /sys/kernel/mm/transparent_hugepage/hugepages-64kB/stats
	grep . -rni
	cd - >> /dev/null

	cat /proc/vmstat | grep thp
}

function test_anon() {
	echo always > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/enabled
	$PERF ./a.out mmap_anon_private_write
	echo never > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/enabled

	stats
}

function test_shm_anon() {
	echo always > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/shmem_enabled
	$PERF ./a.out shm_anon_write
	echo never > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/shmem_enabled

	stats
}

function test_shm_tmpfs() {
	mkdir ~/foo
	mount -t tmpfs -o size=1G,huge=always tmpfs ~/foo

	$PERF dd if=/dev/urandom of=~/foo/testfile bs=1M count=1024

	umount ~/foo
	rm -fr ~/foo

	stats
}

function test_shm_tmpfs_posix() {
	mount -o remount,huge=always /dev/shm
	$PERF ./a.out shm_posix_write
	mount -o remount,huge=never /dev/shm

	stats
}

function test_shm_tmpfs_systemv() {
	echo always > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/shmem_enabled
	$PERF ./a.out shm_systemv_write
	echo never > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/shmem_enabled

	stats
}

function test_khugepaged() {
	echo madvise > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/enabled
	echo 0 > /sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs
	$PERF ./a.out khugepaged
	echo 10000 > /sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs
	echo never > /sys/kernel/mm/transparent_hugepage/hugepages-64kB/enabled

	stats
}

echo "================ test anon ================"
test_anon

echo "============== test shm anon =============="
test_shm_anon

echo "============= test shm tmpfs ============="
test_shm_tmpfs

echo "========== test shm tmpfs posix =========="
test_shm_tmpfs_posix

echo "====== test shm tmpfs systemv (bad) ======"
test_shm_tmpfs_systemv

echo "============ test khugepaged ============"
test_khugepaged

