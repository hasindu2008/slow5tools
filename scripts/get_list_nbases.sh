#!/bin/sh

READ_LIST="$1"
READ_FASTQ="$2"
OUT="$3"

nbases=0
i=0
while read -r readid; do
    read_bases=$(grep "$readid" "$READ_FASTQ" -m 1 -A 1 | tail -1 | wc -c)
    nbases=$((nbases + read_bases - 1))
    i=$((i + 1))
    echo "$i"
done < "$READ_LIST"

echo "$nbases" > "$OUT"
