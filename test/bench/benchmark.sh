#!/bin/sh
# Extract read entries given read ids
# Benchmark over 1,2,4,8,16,32 threads

SLOW5TOOLS_PATH='/home/hasindu/hasindu2008.git/slow5/slow5tools'

DATA_DIR='/data/slow5-testdata/GZFN211103/'
READID_FILE="$DATA_DIR/reads.list"

FAST5_DIR="$DATA_DIR/fast5"
SLOW5_FILE="$DATA_DIR/slow5/bench.slow5"
BLOW5_FILE="$DATA_DIR/slow5/bench.blow5"
CLOW5_FILE="$DATA_DIR/slow5/bench_gz.blow5"
SLOW5_IDX="$SLOW5_FILE.idx"
BLOW5_IDX="$BLOW5_FILE.idx"
CLOW5_IDX="$CLOW5_FILE.idx"

OUT_DIR="$DATA_DIR/slow5/take"

# Create new directory for this test
i=1
while [ -e "$OUT_DIR$i" ]; do
    i=$((i+1))
done
OUT_DIR="$OUT_DIR$i/"

mkdir "$OUT_DIR"
cd "$OUT_DIR" || exit

# Create slow5 files if not already there
if [ ! -e "$SLOW5_FILE" ]; then
    echo 'Creating slow5' 
	test -d tmp/ && rm -rf tmp/
	mkdir tmp/
    command time -v "$SLOW5TOOLS_PATH" f2s "$FAST5_DIR" --lossy -d tmp/ --iop 64  2> f2s.slow5.stderr
    command time -v "$SLOW5TOOLS_PATH"  merge tmp/ > "$SLOW5_FILE" 2>> f2s.slow5.stderr
	rm -rf tmp/
fi
if [ ! -e "$BLOW5_FILE" ]; then
    echo 'Creating blow5'
    command time -v "$SLOW5TOOLS_PATH" view "$SLOW5_FILE" --to blow5 -c none > "$BLOW5_FILE" 2> f2s.blow5.stderr
fi
if [ ! -e "$CLOW5_FILE" ]; then
    echo 'Creating clow5'
    command time -v "$SLOW5TOOLS_PATH" view "$SLOW5_FILE" --to blow5 -c gzip > "$CLOW5_FILE" 2> f2s.clow5.stderr
fi

# Create index files if not already there
if [ ! -e "$SLOW5_IDX" ]; then
    echo 'Creating slow5 index'
    command time -v "$SLOW5TOOLS_PATH" index "$SLOW5_FILE" 2> index.slow5.stderr
fi
if [ ! -e "$BLOW5_IDX" ]; then
    echo 'Creating blow5 index'
    command time -v "$SLOW5TOOLS_PATH" index "$BLOW5_FILE" 2> index.blow5.stderr
fi
if [ ! -e "$CLOW5_IDX" ]; then
    echo 'Creating clow5 index'
    command time -v "$SLOW5TOOLS_PATH" index "$CLOW5_FILE" 2> index.clow5.stderr
fi

# Do the test

# slow5
echo 'Starting slow5 test'
i=1
while [ "$i" -le "32" ]; do
   clean_fscache
   echo "Extracting with $i threads"
   command time -v "$SLOW5TOOLS_PATH" extract "-@$i" "$SLOW5_FILE" < "$READID_FILE" > /dev/null 2> "slow5.bench.stderr$i"
   i=$((i*2))
done

## blow5
echo 'Starting blow5 test'
i=1
while [ "$i" -le "32" ]; do
   clean_fscache
   echo "Extracting with $i threads"
   command time -v "$SLOW5TOOLS_PATH" extract "-@$i" "$BLOW5_FILE" < "$READID_FILE" > /dev/null 2> "blow5.bench.stderr$i"
   i=$((i*2))
done

# blow5.gz
echo 'Starting clow5 test'
i=1
while [ "$i" -le "32" ]; do
    clean_fscache
    echo "Extracting with $i threads"
    command time -v "$SLOW5TOOLS_PATH" extract "-@$i" "$CLOW5_FILE" < "$READID_FILE" > /dev/null 2> "clow5.bench.stderr$i"
    i=$((i*2))
done

cd - || exit
