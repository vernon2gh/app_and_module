#!/bin/bash

TESTFILE=~/testfile

function color_echo() {
	echo -e "\e[32m$1\e[0m"
}

case $1 in
	"vmstat_pgpgin_pgpgout")
		color_echo "0. create test file:"
		cat /proc/vmstat  > origin_vmstat.txt
		dd if=/dev/random of=$TESTFILE bs=1M count=40 &> /dev/null
		cat /proc/vmstat  > access_vmstat.txt
		../common/compare_values origin_vmstat.txt access_vmstat.txt 2 10000

		color_echo "1.1 evict pages from memory:"
		cat /proc/vmstat  > origin_vmstat.txt
		vmtouch -e $TESTFILE >> /dev/null
		cat /proc/vmstat  > access_vmstat.txt
		../common/compare_values origin_vmstat.txt access_vmstat.txt 2 10000

		color_echo "1.2 touch pages into memory:"
		cat /proc/vmstat  > origin_vmstat.txt
		vmtouch -t $TESTFILE >> /dev/null
		cat /proc/vmstat  > access_vmstat.txt
		../common/compare_values origin_vmstat.txt access_vmstat.txt 2 10000

		color_echo "2.1 evict pages from memory:"
		cat /proc/vmstat  > origin_vmstat.txt
		vmtouch -e $TESTFILE >> /dev/null
		cat /proc/vmstat  > access_vmstat.txt
		../common/compare_values origin_vmstat.txt access_vmstat.txt 2 10000

		color_echo "2.2 touch pages into memory:"
		cat /proc/vmstat  > origin_vmstat.txt
		vmtouch -t $TESTFILE >> /dev/null
		cat /proc/vmstat  > access_vmstat.txt
		../common/compare_values origin_vmstat.txt access_vmstat.txt 2 10000
		;;
	*)

		color_echo "Don't look for right demo entry."
		;;
esac

rm -fr $TESTFILE *.txt
