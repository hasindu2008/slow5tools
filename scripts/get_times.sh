#!/bin/sh

n=1
while [ $n -le 32 ]; do
    grep "real time" "$DATA_DIR/bench$i.stderr"
    $n=$((n*2))
done
