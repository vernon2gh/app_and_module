#!/bin/bash

nnodes=$(numactl --hardware | awk '/^available:/ {print $2}')
if [ "${nnodes:-0}" -lt 2 ]; then
	echo "warning: only ${nnodes:-0} NUMA node(s) online" >&2
fi

./a.out
