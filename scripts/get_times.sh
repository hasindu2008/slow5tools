#!/bin/sh

DATA_DIR="$1"
SLOW5_PREFIX="$2"

n=1
while [ $n -le 32 ]; do
    grep "read time" "$DATA_DIR/$SLOW5_PREFIX.bench.stderr$n" | cut -f6 -d " "
    n=$((n*2))
done
