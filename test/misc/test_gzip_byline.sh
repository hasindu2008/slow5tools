#!/bin/bash

while IFS="" read -r line || [ -n "$line" ]; do
    gzip <(echo "$line") -c >> "$1"_line.gz
done < "$1"
echo ""
