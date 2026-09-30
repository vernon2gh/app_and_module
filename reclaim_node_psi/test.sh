#!/bin/bash

CG=/sys/fs/cgroup

mkdir -p $CG/test
echo $$ > $CG/test/cgroup.procs

echo "psi init:"
cat $CG/test/memory.pressure

./a.out

echo "psi test:"
cat $CG/test/memory.pressure
