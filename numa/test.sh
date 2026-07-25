#!/bin/bash

nnodes=$(numactl --hardware | awk '/^available:/ {print $2}')
if [ "${nnodes:-0}" -lt 2 ]; then
	echo "warning: only ${nnodes:-0} NUMA node(s) online" >&2
fi

echo "A: CPU 绑 node1, 默认策略 -> 全部 N1"
numactl --cpunodebind=1 ./a.out 2048

echo "B: CPU 在 node0, mbind 到 node1 -> 全部 N1 (策略胜过 CPU 位置)"
numactl --cpunodebind=0 ./a.out 2048 bind 1

echo "C: interleave node0+node1 -> N0=1024 N1=1024"
./a.out 2048 interleave
