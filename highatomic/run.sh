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

function test() {
	ENABLED=$1

	for _ in {1..3}; do
		./stress.sh
		sleep 60
		./test_priority_allocate.sh $ENABLED
		killall stress-ng
		sleep 60
	done

	grep -w "pgsteal_direct" $OUTPUT/${ENABLED}_*.txt | awk '{sum += $2; count++} END {printf "%s %.0f\n", $1, sum/count}' >> $OUTPUT/${ENABLED}_total.txt
	grep -w "pgscan_direct"  $OUTPUT/${ENABLED}_*.txt | awk '{sum += $2; count++} END {printf "%s %.0f\n", $1, sum/count}' >> $OUTPUT/${ENABLED}_total.txt
	grep -w "highatomic_hit" $OUTPUT/${ENABLED}_*.txt | awk '{sum += $2; count++} END {printf "%s %.0f\n", $1, sum/count}' >> $OUTPUT/${ENABLED}_total.txt
}

test 0
test 1

swapoff $SWAPFILE
