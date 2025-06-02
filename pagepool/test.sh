#!/bin/bash

if [ $1 ]; then
	ENABLED=$1
else
	ENABLED=1
fi

PAGEPOOL=/proc/pagepool

OUTPUT=output
TESTFILE=$OUTPUT/testfile
ALLOC_STAT_OLD=$OUTPUT/alloc_stat_old
ALLOC_STAT_NEW=$OUTPUT/alloc_stat_new
FILE=$OUTPUT/${ENABLED}_$(date "+%Y-%m-%d-%H-%M-%S").txt
mkdir -p $OUTPUT
echo > $FILE

function compare_field() {
	FIELD=$3
	OLD=$(cat $1 | grep -w $FIELD | awk '{print $2}')
	NEW=$(cat $2 | grep -w $FIELD | awk '{print $2}')

	echo "$FIELD $((NEW  - OLD))"
}

function compare_total_direct_field() {
	compare_field $1 $2 success
	compare_field $1 $2 fail
}

cat $PAGEPOOL/alloc_stat > $ALLOC_STAT_OLD
dd if=/dev/random of=$TESTFILE bs=1M count=4096 status=none &
## bpftrace ./alloc_pages.bt -c 'dd if=/dev/random of=$TESTFILE bs=4M count=1024 status=none'
## perf record -g dd if=/dev/random of=$TESTFILE bs=4M count=1024 status=none
if [ $ENABLED == 1 ]; then
	echo $(pidof dd) > $PAGEPOOL/pid
fi
while pgrep -x "dd" >> /dev/null; do
	sleep 3
done
cat $PAGEPOOL/alloc_stat > $ALLOC_STAT_NEW

compare_total_direct_field $ALLOC_STAT_OLD $ALLOC_STAT_NEW >> $FILE

vmtouch -e $TESTFILE &>> /dev/null
rm -fr $TESTFILE $ALLOC_STAT_OLD $ALLOC_STAT_NEW
echo 0 > $PAGEPOOL/pid
