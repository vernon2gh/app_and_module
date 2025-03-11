#!/bin/bash

VMSTAT_OLD=vmstat_old
VMSTAT_NEW=vmstat_new
MEM_STAT_OLD=memory_stat_old
MEM_STAT_NEW=memory_stat_new

CGROUP=/sys/fs/cgroup/test
mkdir -p $CGROUP

free -h
echo 4556 > /proc/sys/vm/min_free_kbytes
echo 1    > /proc/sys/vm/watermark_scale_factor

if [ $1 ]; then
	TEST=$1
else
	TEST=highatomic
fi

if [ $2 ]; then
	ENABLED=$2
else
	ENABLED=1
fi

if [[ $TEST = "pmempool_global" || $TEST = "pmempool_memcg" ]]; then
	dmesg | grep mempool
	cat /sys/kernel/debug/pmempool/stat
fi

if [[ $TEST = "pmempool_memcg" || $TEST = "highatomic" ]]; then
	echo $$ > $CGROUP/cgroup.procs
	#echo 2G > $CGROUP/memory.high
	#echo 4G > $CGROUP/memory.max
fi

if [ $TEST = "pmempool_global" ]; then
	ENABLED_FILE=/sys/kernel/debug/pmempool/enabled
elif [ $TEST = "pmempool_memcg" ]; then
	ENABLED_FILE=$CGROUP/memory.pmempool.enable
elif [ $TEST = "highatomic" ]; then
	ENABLED_FILE=$CGROUP/memory.highatomic
fi

echo $ENABLED > $ENABLED_FILE

function compare_field() {
	FIELD=$3
	OLD=$(cat $1 | grep -w $FIELD | awk '{print $2}')
	NEW=$(cat $2 | grep -w $FIELD | awk '{print $2}')

	echo "$FIELD $((NEW  - OLD))"
}

function compare_total_direct_field() {
	compare_field $1 $2 pgdemote_direct
	compare_field $1 $2 pgsteal_direct
	compare_field $1 $2 pgscan_direct
}

function compare_total_pmempool_field() {
	compare_total_direct_field $1 $2

	compare_field $1 $2 pgdemote_direct_kworker
	compare_field $1 $2 pgsteal_direct_kworker
	compare_field $1 $2 pgscan_direct_kworker
	compare_field $1 $2 pgpool_success
	compare_field $1 $2 pgpool_failed
	compare_field $1 $2 pgpool_refill_wakeup
}

for _ in {1..5}; do
	rm -fr testfile *_old *_new
	echo 3 > /proc/sys/vm/drop_caches

	cat /proc/vmstat             > $VMSTAT_OLD
	cat $CGROUP/memory.stat      > $MEM_STAT_OLD
	time dd if=/dev/random of=testfile bs=3M count=2048
	## bpftrace ./alloc_pages.bt -c 'dd if=/dev/random of=testfile bs=3M count=2048'
	## perf record -g dd if=/dev/random of=testfile bs=3M count=2048
	cat /proc/vmstat             > $VMSTAT_NEW
	cat $CGROUP/memory.stat      > $MEM_STAT_NEW

	if [ $TEST = "pmempool_global" ]; then
		echo "========= vmstat ========="
		compare_total_pmempool_field $VMSTAT_OLD   $VMSTAT_NEW
		compare_total_pmempool_field $VMSTAT_OLD   $VMSTAT_NEW pgpool_refill_failed
	elif [ $TEST = "pmempool_memcg" ]; then
		echo "========= vmstat ========="
		compare_total_pmempool_field $VMSTAT_OLD   $VMSTAT_NEW
		compare_field                $VMSTAT_OLD   $VMSTAT_NEW pgpool_refill_failed
		echo "========= memcg =========="
		compare_total_pmempool_field $MEM_STAT_OLD $MEM_STAT_NEW
	elif [ $TEST = "highatomic" ]; then
		echo "========= vmstat ========="
		compare_total_direct_field $VMSTAT_OLD   $VMSTAT_NEW
		echo "========= memcg =========="
		compare_total_direct_field $MEM_STAT_OLD $MEM_STAT_NEW
		echo "====== pagetypeinfo ======"
		cat /proc/pagetypeinfo
	fi
done

rm -fr testfile *_old *_new
echo 0 > $ENABLED_FILE
