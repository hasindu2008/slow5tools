#!/bin/sh
# Extract read entries given read ids
# Benchmark over 1,2,4,8,16,32 threads

REL_DIR="$(dirname $0)/"
OUT_DIR="$REL_DIR/output/GZFN211103/"

cd "$OUT_DIR"

SLOW5TOOLS_PATH="../../../slow5tools"
READID_FILE=reads.list

i=1
while [ "$i" -le "32" ]; do
    echo "Extracting with $i threads"
    time -v "$SLOW5TOOLS_PATH" extract "-@$i" bench.blow5.gz < "$READID_FILE" > "bench.out$i" 2> "bench.stderr$i"
    i=$((i*2))
done

cd -
