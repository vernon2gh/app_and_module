#!/bin/bash

if [ ! -e extswapfile ]; then
	fallocate -l 20M extswapfile
	chmod 0600 ./extswapfile
	mkswap ./extswapfile
fi

swapon ./extswapfile

./a.out

swapoff ./extswapfile
