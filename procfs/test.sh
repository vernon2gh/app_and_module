#!/bin/bash

insmod mod.ko
cat /proc/xxx/test
echo 123 > /proc/xxx/test
cat /proc/xxx/test
rmmod mod
