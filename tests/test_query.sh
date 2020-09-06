#!/bin/bash
# Usage $0
# Tests that querying slow5 index files matches the expected output.

# Note: 
#
# - Don't move this file from "slow5/tests/". 
#   Requires subdirectory "data/" with testing dataset(s),
#   and "slow5tools" executable in parent directory.
#
# - Expects testing datasets with the following structure:
#     - dataset/
#         |- expected.slow5
#         |- expected.slow5.s5i



# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 

# Folder containing testing datasets
DATA_DIR="$REL_PATH/data"
# Ensure data directory exists
if [ ! -d $DATA_DIR ]; then
    echo "ERROR: Missing data directory \""$REL_PATH"/"$DATA_DIR"\""

    echo "Exiting"
    exit 1
fi

# Path to slow5tools 
SLOW5TOOLS_PATH="$REL_PATH/../slow5tools"
# Ensure slow5tools exists
if [ ! -f $SLOW5TOOLS_PATH ]; then
    echo "ERROR: Missing slow5tools \""$REL_PATH"/"$SLOW5TOOLS_PATH"\""

    echo "Exiting"
    exit 1
fi

CMD_SLOW5_IDX_QUERY="extract"


# File name of expected SLOW5 output
SLOW5_EXPECTED="expected.slow5"

declare -i ret=0
declare -i total_tests=0
declare -i total_passed=0

# Iterate through each testset
for testset in $DATA_DIR/*; do
    echo "$testset"

    declare -i n_lines=$(wc -l < "$testset/$SLOW5_EXPECTED")
    declare -i n_tests=$(( $n_lines / 10 ))
    if [ $n_tests -lt 5 ]; then
        n_tests=5
    fi
    total_tests+=$n_tests

    echo Testing $n_tests reads
    echo ""

    for i in $(seq 1 $n_tests); do
        declare -i rand_line=$(( 3 + RANDOM % ($n_lines - 3) )) # TODO change to check for #
        rand_read="$(sed "${rand_line}q;d" "$testset/$SLOW5_EXPECTED")"
        rand_readid="$(echo "$rand_read" | cut -f1)"
        query_read="$("$SLOW5TOOLS_PATH" $CMD_SLOW5_IDX_QUERY "$testset/$SLOW5_EXPECTED" "$rand_readid" 2>/dev/null)"

        if diff <(echo "$query_read") <(echo "$rand_read") 2>&1 >/dev/null; then
            total_passed+=1
        else
            ret=1
            echo -e "failure
--READ-ID------------------------
$rand_readid
---------------------------------
"
        fi
    done
done

if [ $ret -eq 0 ]; then
    echo "PASSED $total_passed/$total_tests"
else
    echo "FAILED $total_passed/$total_tests"
fi
exit $ret
