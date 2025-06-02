#!/bin/bash

OUTPUT=output
SWAPFILE=$OUTPUT/swapfile

mkdir -p $OUTPUT

if [ ! -e $SWAPFILE ]; then
	fallocate -l 10G $SWAPFILE
	chmod 0600 $SWAPFILE
	mkswap $SWAPFILE
fi

swapon $SWAPFILE
./stress.sh
sleep 60

function test() {
	ENABLED=$1

	for _ in {1..3}; do
		./test.sh $ENABLED
	done

	grep -w "success" $OUTPUT/${ENABLED}_*.txt | awk '{sum += $2; count++} END {printf "%s %.0f\n", $1, sum/count}' >> $OUTPUT/${ENABLED}_total.txt
	grep -w "fail"  $OUTPUT/${ENABLED}_*.txt | awk '{sum += $2; count++} END {printf "%s %.0f\n", $1, sum/count}' >> $OUTPUT/${ENABLED}_total.txt
}

test 0
sleep 3
test 1

pkill stress-ng
swapoff $SWAPFILE
