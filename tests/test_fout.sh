#!/bin/sh
# Usage $0
# Tests that f2s matches the expected output.

# Note: 
#
# - Don't move this file from "slow5/tests/". 
#   Requires subdirectory "data/" with testing dataset(s),
#   and "slow5tools" executable in parent directory.
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

# Path to slow5tools 
SLOW5TOOLS_PATH="$REL_PATH/../slow5tools"
# Ensure slow5tools exists
if [ ! -f $SLOW5TOOLS_PATH ]; then
    echo "ERROR: Missing slow5tools \"$SLOW5TOOLS_PATH\""

    echo "Exiting"
    exit 1
fi

CMD_FAST5_TO_SLOW5="fastt"


# Folder name in datasets containing FAST5 files
FAST5_FOLDER="fast5_files/"

# File name of expected SLOW5 output
SLOW5_EXPECTED="expected.slow5"
# File name of actual SLOW5 output
SLOW5_ACTUAL="actual.slow5"

# Iterate through each testset
for testset in $DATA_DIR/*; do
    "$SLOW5TOOLS_PATH" "$CMD_FAST5_TO_SLOW5" "$testset/$FAST5_FOLDER" > "$testset/$SLOW5_ACTUAL"
    diff "$testset/$SLOW5_EXPECTED" "$testset/$SLOW5_ACTUAL"
done
