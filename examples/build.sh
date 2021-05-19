#!/bin/bash

gcc -Wall -O2 -I src/ examples/sequential_read.c build/libslow5.a  -o examples/sequential_read -lz
#gcc -Wall -O2 -I src/ -L build/ examples/sequential_read.c   -o examples/sequential_read -lslow5 -lz
