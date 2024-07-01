#!/bin/bash

fallocate -l 100M testfile

echo "==== origin meminfo ===="
cat /proc/meminfo | grep -E "file|anon"

echo "==== meminfo after access ===="
./a.out $1

echo "==== meminfo after access ===="
./a.out $1

rm -fr testfile
