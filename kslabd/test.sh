#!/bin/bash

VMSTAT_OLD=vmstat_old
VMSTAT_NEW=vmstat_new

CGROUP=/sys/fs/cgroup/test
mkdir -p $CGROUP
echo $$ > $CGROUP/cgroup.procs

function compare_field() {
	FIELD=$3
	OLD=$(cat $1 | grep -w $FIELD | awk '{print $2}')
	NEW=$(cat $2 | grep -w $FIELD | awk '{print $2}')

	echo "$FIELD $((NEW  - OLD))"
}

free -h

dd if=/dev/random of=testfile bs=1M count=200
perf record -g echo 100M > $CGROUP/memory.reclaim
mv perf.data perf.data.direct

perf record -g -t $(pidof kswapd0) &
dd if=/dev/random of=testfile bs=3M count=2048
kill $(pidof perf)
mv perf.data perf.data.kswapd

rm -fr testfile; sync; sleep 3
bpftrace ./alloc_pages.bt -c 'dd if=/dev/random of=testfile bs=3M count=2048'

rm -fr testfile; sync; sleep 3
cat /proc/vmstat > $VMSTAT_OLD
time dd if=/dev/random of=testfile bs=3M count=2048
cat /proc/vmstat > $VMSTAT_NEW
compare_field $VMSTAT_OLD $VMSTAT_NEW pgsteal_kswapd
compare_field $VMSTAT_OLD $VMSTAT_NEW pgsteal_direct

rm -fr testfile $VMSTAT_OLD $VMSTAT_NEW
