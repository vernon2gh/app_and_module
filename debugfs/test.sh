#!/bin/bash

insmod mod.ko
cat /sys/kernel/debug/xxx/test
echo 123 > /sys/kernel/debug/xxx/test
cat /sys/kernel/debug/xxx/test
rmmod mod
