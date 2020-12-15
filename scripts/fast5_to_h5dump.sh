#!/bin/sh
#
# Create h5dump equivalents of fast5 files
# Usage: $0 fast5_folder out_folder

if [ -n "$1" ] && [ -n "$2" ]; then

    i=1
    for file in "$1"/*; do
        file_base=$(basename "$file")
        if [ -f "$file" ]; then
            h5dump "$file" > "$2/$file_base.h5dump"
        fi
        echo "$i"
        i=$((i + 1))
    done

else
    echo "Usage $0 fast5_folder out_folder"
fi
