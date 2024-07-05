#!/bin/bash

echo "==== origin slab log ===="
cat /proc/meminfo | grep -E 'SReclaimable|SUnreclaim'
cat /proc/slabinfo | grep '^kmalloc.*4k'

insmod mod.ko demo_entry="$1"

echo "==== slab log after access ===="
cat /proc/meminfo | grep -E 'SReclaimable|SUnreclaim'
cat /proc/slabinfo | grep '^kmalloc.*4k'

rmmod mod.ko
