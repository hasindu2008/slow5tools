#!/bin/sh
#
# Create h5dump equivalents of fast5 files
# Usage: $0 fast5_folder out_folder

i=1
h5dump_file() {
    if [ -f "$1" ]; then
        file="$1"
        file_base=$(basename "$file")
        h5dump -A "$file" > "$2/$3_$file_base.h5dump"
        echo "$i"
        i=$((i + 1))
    elif [ -d "$1" ]; then
        dir="$1"
        for file in "$dir"/*; do
            h5dump_file "$file" "$2" "$3_$dir"
        done
    fi
}

if [ -n "$1" ] && [ -n "$2" ]; then

    for file in "$1"/*; do
        h5dump_file "$file" "$2" ""
    done

else
    echo "Usage $0 fast5_folder out_folder"
fi
