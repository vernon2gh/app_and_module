#!/bin/bash

CGROUP=/sys/fs/cgroup

mkdir $CGROUP/test
echo 10M > $CGROUP/test/memory.max
echo $$ > $CGROUP/test/cgroup.procs

./a.out
