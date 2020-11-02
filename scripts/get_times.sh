#!/bin/sh

DATA_DIR="$1"

n=1
while [ $n -le 32 ]; do
    grep "real time" "$DATA_DIR/bench.stderr$n" | cut -f5 -d " "
    n=$((n*2))
done
