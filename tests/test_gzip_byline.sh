#!/bin/bash

while IFS="" read -r line || [ -n "$line" ]; do
    gzip <(echo "$line") -c >> $1_line.gz
done < $1
echo ""
