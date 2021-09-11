#!/bin/bash

set -e

FAST5_DIR=$1
TMP_FAST5=../tmp_fast5
TMP_BLOW5=../tmp_blow5
MERGED_BLOW5=../reads.blow5

cd $FAST5_DIR
mkdir $TMP_FAST5 $TMP_BLOW5

list=$(ls -1 | sed 's/_ch.*.fast5//g' | sort -u)
for each in $list; do
    echo $each; mkdir $TMP_FAST5/$each;
    mv $each* $TMP_FAST5/$each/
    slow5tools f2s $TMP_FAST5/$each/ -d $TMP_BLOW5/$each
done

slow5tools merge $TMP_BLOW5/ -o $MERGED_BLOW5
