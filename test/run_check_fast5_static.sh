#!/bin/sh

# Given a folder of fast5 files
# Copies them to tmp
# Creates files with the output of h5dump on each fast5 file
# Runs check to see which attributes change and are static over the experiment
#
# Usage: $0 [fast5_folder] [out_folder]

TMP_FOLDER='/tmp/run_check_fast5_static_copy'

dirname="$(dirname "$0")"

if [ -n "$1" ] && [ -n "$2" ]; then
    # Copies them to tmp
    mkdir "$TMP_FOLDER" || rm "$TMP_FOLDER"/*
    cp "$1"/* "$TMP_FOLDER"

    # Creates files with the output of h5dump on each fast5 file
    "$dirname/../scripts/fast5_to_h5dump.sh" "$TMP_FOLDER" "$2"

    # Runs check to see which attributes change and are static over the experiment
    python3 "$dirname/check_fast5_static.py" "$2"/*.h5dump > "$2/fast5_var_const.txt"
    python3 "$dirname/check_fast5_static_nice.py" "$2"/*.h5dump > "$2/fast5_var_const_nice.txt"
    python3 "$dirname/check_fast5_static.py" -c "$2"/*.h5dump > "$2/fast5_var_const_label.txt"
    python3 "$dirname/check_fast5_static_nice.py" -c "$2"/*.h5dump > "$2/fast5_var_const_nice_label.txt"

else
    echo "Usage $0 fast5_folder out_folder"
fi
