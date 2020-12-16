# Given a folder of fast5 files
# Copies them to tmp
# Creates files with the output of h5dump on each fast5 file
# Runs check to see which attributes change and are static over the experiment
#
# Usage: $0 [fast5_folder] [out_folder]

TMP_FOLDER='/tmp/run_check_fast5_static_copy'

if [ -n "$1" ] && [ -n "$2" ]; then
    # Copies them to tmp
    mkdir -p "$TMP_FOLDER"
    cp "$1"/* "$TMP_FOLDER"

    # Creates files with the output of h5dump on each fast5 file
    ../scripts/fast5_to_h5dump.sh "$TMP_FOLDER" "$2"

    # Runs check to see which attributes change and are static over the experiment
    python3 check_fast5_static.py "$2"/*.h5dump > "$2/fast5_var_const.txt"
    python3 check_fast5_static_nice.py "$2"/*.h5dump > "$2/fast5_var_const_nice.txt"

else
    echo "Usage $0 fast5_folder out_folder"
fi
