#!/bin/bash

insmod mod.ko
dmesg | tail
ps -e | grep kxxxd
rmmod mod
