#!/bin/bash
# WARNING: this file should be stored inside test directory
# WARNING: the executable should be found at ../
# WARNING: four slow5s should be found at ./data/exp/merge/slow5s
# WARNING: expected slow5 should be found at ./data/exp/merge

Usage="test_merge.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

# terminate script
die() {
    echo -e "${RED}$1${NC}" >&2
    echo
    exit 1
}

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $REL_PATH/../slow5tools"
else
    SLOW5_EXEC=$REL_PATH/../slow5tools
fi

OUTPUT_DIR="$REL_PATH/data/out/merge"
test -d $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir $OUTPUT_DIR || die "Creating $OUTPUT_DIR failed"

INPUT_FILE=$REL_PATH/data/exp/merge/slow5s

# merge four single group slow5s and diff with expected slow5 to check if slow5tools merge is working as expected

INPUT_FILES="$INPUT_FILE/rg0.slow5 $INPUT_FILE/rg1.slow5 $INPUT_FILE/rg2.slow5 $INPUT_FILE/rg3.slow5"

NUM_THREADS=2

echo "-------------------tesetcase 0: slow5tools version-------------------"
$SLOW5_EXEC --version || die "tesetcase 0: slow5tools version failed"

echo
echo "-------------------tesetcase 1: lossless merging-------------------"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 --to slow5 || die "tesetcase 1: lossless merging failed"
echo "comparing merged_output and merged_expected"
sort $REL_PATH/data/exp/merge/merged_expected.slow5 > $OUTPUT_DIR/merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/merged_output.slow5 > $OUTPUT_DIR/merged_output_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/merged_output.slow5  || die "remove $OUTPUT_DIR/merged_output.slow5 failed"
cmp -s $OUTPUT_DIR/merged_expected_sorted.slow5 $OUTPUT_DIR/merged_output_sorted.slow5
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: merged files are consistent!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: merged files are not consistent${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

echo
echo "-------------------tetcase 2: lossy merging-------------------"
$SLOW5_EXEC merge -l false $INPUT_FILES -o $OUTPUT_DIR/lossy_merged_output.slow5 --to slow5 || die "tetcase 2: lossy merging failed"
echo "comparing lossy_merged_output and lossy_merged_expected"
sort $REL_PATH/data/exp/merge/lossy_merged_expected.slow5 > $OUTPUT_DIR/lossy_merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/lossy_merged_output.slow5 > $OUTPUT_DIR/lossy_merged_output_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/lossy_merged_output.slow5 || die "remove $OUTPUT_DIR/lossy_merged_output.slow5 failed"
cmp -s $OUTPUT_DIR/lossy_merged_expected_sorted.slow5 $OUTPUT_DIR/lossy_merged_output_sorted.slow5
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: lossy merged files are consistent!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: lossy merged files are not consistent${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

echo
echo "-------------------tesetcase 3: lossless merging with threads-------------------"
$SLOW5_EXEC merge -t $NUM_THREADS $INPUT_FILES -o $OUTPUT_DIR/merged_output_using_threads.slow5 --to slow5 || die "tesetcase 3: lossless merging using threads failed"
echo "comparing merged_output_using_threads and merged_expected"
sort $REL_PATH/data/exp/merge/merged_expected.slow5 > $OUTPUT_DIR/merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/merged_output_using_threads.slow5 > $OUTPUT_DIR/merged_output_using_threads_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/merged_output_using_threads.slow5  || die "remove $OUTPUT_DIR/merged_output_using_threads.slow5 failed"
cmp -s $OUTPUT_DIR/merged_expected_sorted.slow5 $OUTPUT_DIR/merged_output_using_threads_sorted.slow5
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: merged files are consistent!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: merged files are not consistent${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

# merging with and without enum data type
echo
echo "-------------------tesetcase 4: merging with and without enum type-------------------"
INPUT_FILES="$INPUT_FILE/aux_no_enum.slow5 $INPUT_FILE/aux_enum.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_enum.slow5 || die "tesetcase 4: merging with and without enum type failed"

# merging different slow5 formats and versions
echo
echo "-------------------tesetcase 5: merging different slow5 formats and versions-------------------"
INPUT_FILES="$INPUT_FILE/aux_no_enum.slow5 $INPUT_FILE/none_v0.1.0.blow5 $INPUT_FILE/zlib_svb-zd_v0.2.0.blow5 $INPUT_FILE/zlib_v0.2.0.blow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 || die "tesetcase 4: merging with and without enum type failed"


#rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
