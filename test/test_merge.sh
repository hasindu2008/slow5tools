#!/bin/bash

# steps
# run merge for different testcases
# diff

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color
die() { echo -e "${RED}$1${NC}" >&2 ; echo ; exit 1 ; } # terminate script
info() {  echo ; echo -e "${GREEN}$1${NC}" >&2 ; }

#...directories files tools arguments commands clean
# Relative path to "slow5tools/tests/"
REL_PATH="$(dirname $0)/"

EXP_DIR="$REL_PATH/data/exp/merge"
RAW_DIR="$REL_PATH/data/raw/merge"
OUTPUT_DIR="$REL_PATH/data/out/merge"
test -d "$OUTPUT_DIR" && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Failed creating $OUTPUT_DIR"

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $REL_PATH/../slow5tools"
else
    SLOW5_EXEC=$REL_PATH/../slow5tools
fi
NUM_THREADS=4

TESTCASE=0
echo "-------------------tesetcase $TESTCASE: slow5tools version-------------------"
$SLOW5_EXEC --version || die "tesetcase TESTCASE: slow5tools version failed"

TESTCASE=1
echo "-------------------tesetcase $TESTCASE: lossless merging-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg1.slow5 $RAW_DIR/rg2.slow5 $RAW_DIR/rg3.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 --to slow5 || die "tesetcase $TESTCASE: lossless merging failed"
echo "comparing merged_output and merged_expected"
sort $REL_PATH/data/exp/merge/merged_expected.slow5 > $OUTPUT_DIR/merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/merged_output.slow5 > $OUTPUT_DIR/merged_output_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/merged_output.slow5  || die "remove $OUTPUT_DIR/merged_output.slow5 failed"
diff -q $OUTPUT_DIR/merged_expected_sorted.slow5 $OUTPUT_DIR/merged_output_sorted.slow5 || die "diff testcase $TESTCASE failed"

TESTCASE=2
echo "-------------------tetcase $TESTCASE: lossy merging-------------------"
$SLOW5_EXEC merge -l false $INPUT_FILES -o $OUTPUT_DIR/lossy_merged_output.slow5 --to slow5 || die "tetcase $TESTCASE: lossy merging failed"
echo "comparing lossy_merged_output and lossy_merged_expected"
sort $REL_PATH/data/exp/merge/lossy_merged_expected.slow5 > $OUTPUT_DIR/lossy_merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/lossy_merged_output.slow5 > $OUTPUT_DIR/lossy_merged_output_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/lossy_merged_output.slow5 || die "remove $OUTPUT_DIR/lossy_merged_output.slow5 failed"
diff -q $OUTPUT_DIR/lossy_merged_expected_sorted.slow5 $OUTPUT_DIR/lossy_merged_output_sorted.slow5 || die "diff testcase $TESTCASE failed"

TESTCASE=3
echo "-------------------tesetcase $TESTCASE: lossless merging with threads-------------------"
$SLOW5_EXEC merge -t $NUM_THREADS $INPUT_FILES -o $OUTPUT_DIR/merged_output_using_threads.slow5 --to slow5 || die "tesetcase $TESTCASE: lossless merging using threads failed"
echo "comparing merged_output_using_threads and merged_expected"
sort $REL_PATH/data/exp/merge/merged_expected.slow5 > $OUTPUT_DIR/merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/merged_output_using_threads.slow5 > $OUTPUT_DIR/merged_output_using_threads_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/merged_output_using_threads.slow5  || die "remove $OUTPUT_DIR/merged_output_using_threads.slow5 failed"
diff -q $OUTPUT_DIR/merged_expected_sorted.slow5 $OUTPUT_DIR/merged_output_using_threads_sorted.slow5 || die "diff testcase $TESTCASE failed"

# merging with and without enum data type
TESTCASE=4
echo "-------------------tesetcase $TESTCASE: merging with and without enum type-------------------"
INPUT_FILES="$RAW_DIR/aux_no_enum.slow5 $RAW_DIR/aux_enum.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_enum.slow5 || die "tesetcase $TESTCASE: merging with and without enum type failed"
diff -q $REL_PATH/data/exp/merge/merged_output_enum.slow5  $OUTPUT_DIR/merged_output_enum.slow5 || die "tesetcase $TESTCASE: diff for merging with and without enum type failed"

TESTCASE=5
echo "-------------------tesetcase $TESTCASE: merging without and with enum type-------------------"
INPUT_FILES="$RAW_DIR/aux_enum.slow5 $RAW_DIR/aux_no_enum.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_enum2.slow5 || die "tesetcase $TESTCASE: merging with and without enum type failed"
diff -q $REL_PATH/data/exp/merge/merged_output_enum2.slow5  $OUTPUT_DIR/merged_output_enum2.slow5 || die "tesetcase $TESTCASE: diff for merging with and without enum type failed"

# merging different slow5 formats and versions
TESTCASE=5
echo "-------------------tesetcase $TESTCASE: merging different slow5 formats and versions-------------------"
INPUT_FILES="$RAW_DIR/aux_no_enum.slow5 $RAW_DIR/none_v0.1.0.blow5 $RAW_DIR/zlib_svb-zd_v0.2.0.blow5 $RAW_DIR/zlib_v0.2.0.blow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_formats.slow5 || die "tesetcase $TESTCASE: merging different file types failed"
diff -q $REL_PATH/data/exp/merge/merged_output_formats.slow5  $OUTPUT_DIR/merged_output_formats.slow5 -q || die "tesetcase $TESTCASE: diff for merging different file types failed"

rm -r "$OUTPUT_DIR" || die "could not delete $OUTPUT_DIR"
info "done"
exit 0