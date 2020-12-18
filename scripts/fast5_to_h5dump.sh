#!/bin/sh
#
# Create h5dump equivalents of fast5 files
# Usage: $0 fast5_folder out_folder
#
# If exists subfolders in fast5_folder the program prefixes
# the fast5 files in them with the folder name followed by an underscore

i=1
# h5dump_file file/dir out_folder prefix
h5dump_file() {
    if [ -f "$1" ]; then
        file="$1"
        file_base=$(basename "$file")
        h5dump -A "$file" > "$2/$3_$file_base.h5dump"
        echo "$3_$file_base" # testing
        echo "$i"
        i=$((i + 1))
    elif [ -d "$1" ]; then
        dir="$1"
        dir_base="$(basename "$dir")"
        for file in "$dir"/*; do
            if [ -n "$3" ]; then
                h5dump_file "$file" "$2" "$3_$dir_base"
            else
                h5dump_file "$file" "$2" "$dir_base"
            fi
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
