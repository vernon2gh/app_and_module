#!/bin/bash

echo "==== create testfile ===="
fallocate -l 100M testfile
cat /proc/meminfo | grep "file"

echo "==== first call demo, $1 ===="
./a.out $1

echo "==== second call demo, $1 ===="
./a.out $1

rm -fr testfile
