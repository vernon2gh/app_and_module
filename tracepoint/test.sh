#!/bin/bash

insmod mod.ko

echo 1 > /sys/kernel/tracing/events/testtracepoint/test/enable
echo 1 > /sys/kernel/tracing/tracing_on
sleep 3

cat /sys/kernel/tracing/trace

rmmod mod
