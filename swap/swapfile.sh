#!/bin/bash

if [ ! -e extswapfile ]; then
	fallocate -l 20M extswapfile
	chmod 0600 ./extswapfile
	mkswap ./extswapfile
fi

swapon ./extswapfile

gcc app.c
./a.out &

echo 0 > /sys/kernel/tracing/tracing_on
pidof a.out > /sys/kernel/tracing/set_ftrace_pid
echo function_graph > /sys/kernel/tracing/current_tracer
echo madvise_cold_or_pageout_pte_range > /sys/kernel/tracing/set_graph_function
echo > /sys/kernel/tracing/trace

function wait_app_ready()
{
	while [[ ! -e /dev/shm/sync_status || $(tr -d '\0' < /dev/shm/sync_status) == '' ]]; do
		sleep 1
	done
}

function wait_app_finish_and_enable_debug()
{
	while [ $(tr -d '\0' < /dev/shm/sync_status) != "app2shell" ]; do
		sleep 1
	done

	if [ $1 == true ]; then
		echo "file madvise.c +p" > /sys/kernel/debug/dynamic_debug/control
		echo 1 > /sys/kernel/tracing/tracing_on
	else
		echo 0 > /sys/kernel/tracing/tracing_on
		echo "file madvise.c -p" > /sys/kernel/debug/dynamic_debug/control
	fi

	echo "shell2app" > /dev/shm/sync_status
}

wait_app_ready
wait_app_finish_and_enable_debug true
wait_app_finish_and_enable_debug false

swapoff ./extswapfile
