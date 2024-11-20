#!/bin/bash

file=$1
addr=$2

if [ $# -ne 2 ]; then
	echo "Usage: $0 <file> <addr>"
	exit 1
fi

cat $file > local_file.txt
file=local_file.txt

addr=$(printf "%u\n" "$addr")

while read line; do
	range=$(echo $line | awk '{print $1}')

	if [[ $range =~ ([0-9a-fA-Fx]+)-([0-9a-fA-Fx]+) ]]; then
		start=$(printf "%u\n" "${BASH_REMATCH[1]}")
		end=$(printf "%u\n" "${BASH_REMATCH[2]}")

		if (( addr >= start && addr < end )); then
			echo "$line"
			break
		fi
	fi
done < $file

rm -fr *.txt
