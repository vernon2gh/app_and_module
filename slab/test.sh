#!/bin/bash

display_slab_log() {
	cat /proc/meminfo | grep -E 'SReclaimable|SUnreclaim'
	cat /proc/slabinfo | grep '^kmalloc-4k'
}


echo "==== origin slab log ===="
display_slab_log

insmod mod.ko demo_entry="4KB"

echo "==== slab log after access ===="
display_slab_log

rmmod mod.ko
