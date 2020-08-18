#!/bin/sh
# Usage ./test.sh
# Tests that fast5toslow5 matches the expected output.

# Note: 
#
# - Don't move this file from "slow5/tests/". 
#   Requires subdirectory "data/" with testing dataset(s),
#   and "fast5toslow5" executable in parent directory.
#
# - Expects testing datasets with the following structure:
#     - dataset/
#         |- expected.slow5
#         |- expected.slow5.fti
#         |- expected_bin.slow5
#         |- fast5_files/
#             |- expected_1.fast5
#             |- ...



# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 

# Folder containing testing datasets
DATA_DIR="$REL_PATH/data/"
# Ensure data directory exists
if [ ! -d $DATA_DIR ]; then
    echo "ERROR: Missing data directory \"$DATA_DIR\""

    echo "Exiting"
    exit 1
fi

# Path to fast5toslow5
F5_TO_S5_PATH="$REL_PATH/../fast5toslow5"
# Ensure fast5toslow5 exists
if [ ! -f $DATA_DIR ]; then
    echo "ERROR: Missing fast5toslow5 \"$F5_TO_S5_PATH\""

    echo "Exiting"
    exit 1
fi


# Folder name in datasets containing FAST5 files
FAST5_FOLDER="fast5_files/"

# File name of expected SLOW5 output
SLOW5_EXPECTED="expected.slow5"
# File name of actual SLOW5 output
SLOW5_ACTUAL="actual.slow5"

# Iterate through each testset
for testset in "$DATA_DIR/*"; do
    "$F5_TO_S5_PATH" "$testset/$FAST5_FOLDER" -o "$testset/$SLOW5_ACTUAL"
    diff "$testset/$SLOW5_EXPECTED" "$testset/$SLOW5_ACTUAL"
done
