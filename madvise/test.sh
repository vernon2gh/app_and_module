#!/bin/bash

dmesg -C

./a.out $1
dmesg
