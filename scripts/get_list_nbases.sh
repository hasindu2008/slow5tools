#!/bin/sh

READ_LIST="$1"
READ_FASTQ="$2"

nbases=0
for readid in $(cat "$READ_LIST"); do
    read_bases=$(grep "$readid" "$READ_FASTQ" -m 1 -A 1 | tail -1 | wc -c)
    nbases=$((nbases + read_bases - 1))
done

echo "$nbases"
