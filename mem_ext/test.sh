#!/bin/bash

DIR=./build/bin
gcc app.c

## origin test
perf stat -- ./a.out

## thp adjust test
$DIR/mthp_adjust &
perf stat -- ./a.out
killall mthp_adjust
