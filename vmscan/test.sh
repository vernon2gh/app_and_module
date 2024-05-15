#!/bin/bash

echo "==== create testfile ===="
fallocate -l 100M testfile
cat /proc/meminfo | grep -E "file|anon"

echo "==== access testfile ===="
./a.out $1

echo "==== access testfile ===="
./a.out $1

rm -fr testfile
