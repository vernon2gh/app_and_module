#!/bin/bash

if [ $1 ]; then
	ENABLED=$1
else
	ENABLED=1
fi

OUTPUT=output
TESTFILE=$OUTPUT/testfile
VMSTAT_OLD=$OUTPUT/vmstat_old
VMSTAT_NEW=$OUTPUT/vmstat_new
MEM_STAT_OLD=$OUTPUT/memory_stat_old
MEM_STAT_NEW=$OUTPUT/memory_stat_new
FILE=$OUTPUT/${ENABLED}_$(date "+%Y-%m-%d-%H-%M-%S").txt
mkdir -p $OUTPUT
echo > $FILE

## create cgroup environment
CGROUP=/sys/fs/cgroup/test
ENABLED_FILE=$CGROUP/memory.highatomic
mkdir -p $CGROUP
echo $$ > $CGROUP/cgroup.procs

function compare_field() {
	FIELD=$3
	OLD=$(cat $1 | grep -w $FIELD | awk '{print $2}')
	NEW=$(cat $2 | grep -w $FIELD | awk '{print $2}')

	echo "$FIELD $((NEW  - OLD))"
}

function compare_total_direct_field() {
	compare_field $1 $2 pgsteal_direct
	compare_field $1 $2 pgscan_direct
	compare_field $1 $2 highatomic_hit
}

echo $ENABLED > $ENABLED_FILE

cat /proc/vmstat             > $VMSTAT_OLD
cat $CGROUP/memory.stat      > $MEM_STAT_OLD
dd if=/dev/random of=$TESTFILE bs=1M count=1024 status=none
## bpftrace ./alloc_pages.bt -c 'dd if=/dev/random of=$TESTFILE bs=4M count=1024 status=none'
## perf record -g dd if=/dev/random of=$TESTFILE bs=4M count=1024 status=none
cat /proc/vmstat             > $VMSTAT_NEW
cat $CGROUP/memory.stat      > $MEM_STAT_NEW

## echo "========= vmstat ========="			>> $FILE
## compare_total_direct_field $VMSTAT_OLD $VMSTAT_NEW	>> $FILE
echo "========= memcg =========="			>> $FILE
compare_total_direct_field $MEM_STAT_OLD $MEM_STAT_NEW	>> $FILE
echo "====== pagetypeinfo ======"			>> $FILE
cat /proc/pagetypeinfo					>> $FILE
echo "======== zoneinfo ========"			>> $FILE
cat /proc/zoneinfo | grep -E "zone |highatomic"		>> $FILE

vmtouch -e $TESTFILE &>> /dev/null
rm -fr $TESTFILE $VMSTAT_OLD $MEM_STAT_OLD $VMSTAT_NEW $MEM_STAT_NEW

echo 0 > $ENABLED_FILE
