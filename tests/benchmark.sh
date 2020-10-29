#!/bin/sh
# Extract read entries given read ids
# Benchmark over 1,2,4,8,16,32 threads

DATA_DIR='/data/slow5-testdata/GZFN211103/'
OUT_DIR="$DATA_DIR/slow5/take"

# Create new directory for this test
i=1
while [ -e "$OUT_DIR$i" ]; do
    i=$((i+1))
done
OUT_DIR="$OUT_DIR$i/"

mkdir "$OUT_DIR"
cd "$OUT_DIR"

SLOW5TOOLS_PATH='/home/sasjen/slow5/slow5tools'
READID_FILE="$DATA_DIR/reads.list"
SLOW5_FILE="$DATA_DIR/slow5/bench.blow5.gz"

# Do the test
i=1
while [ "$i" -le "32" ]; do
    clean_fscache
    echo "Extracting with $i threads"
    time -v "$SLOW5TOOLS_PATH" extract "-@$i" "$SLOW5_FILE" < "$READID_FILE" > "bench.out$i" 2> "bench.stderr$i"
    i=$((i*2))
done

cd -
