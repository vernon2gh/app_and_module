#!/bin/bash

compare_values() {
 	if [ $# -ne 4 ]; then
		echo "Usage: compare_values file1 file2 column delta"
		return 1
	fi

	diff -y --suppress-common-lines $1 $2 | awk -v column=$3 -v delta=$4 '
	function abs(x) {
		return (x < 0 ? -x : x);
	}
	{
                split($0, array, "|");
                split(array[1], left, " ");
                val1 = left[column];
                split(array[2], right, " ");
                val2 = right[column];

		if (abs(val2 - val1) > delta)
			print $0;
	}
	'
}

cat /proc/meminfo  > origin_meminfo.txt

insmod mod.ko demo_entry="$1"

cat /proc/meminfo  > access_meminfo.txt

rmmod mod.ko

compare_values origin_meminfo.txt access_meminfo.txt 2 100000

rm -fr origin_*.txt access_*.txt
