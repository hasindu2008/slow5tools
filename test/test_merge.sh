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
info "-------------------tesetcase $TESTCASE: slow5tools version-------------------"
$SLOW5_EXEC --version || die "tesetcase TESTCASE: slow5tools version failed"

TESTCASE=1
TESTNAME="lossless merging"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg1.slow5 $RAW_DIR/rg2.slow5 $RAW_DIR/rg3.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 --to slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
sort $REL_PATH/data/exp/merge/merged_expected.slow5 > $OUTPUT_DIR/merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/merged_output.slow5 > $OUTPUT_DIR/merged_output_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/merged_output.slow5  || die "remove $OUTPUT_DIR/merged_output.slow5 failed"
diff -q $OUTPUT_DIR/merged_expected_sorted.slow5 $OUTPUT_DIR/merged_output_sorted.slow5 || die "diff testcase $TESTCASE: $TESTNAME failed"

TESTCASE=2
TESTNAME="lossy merging"
info "-------------------tetcase $TESTCASE: $TESTNAME-------------------"
$SLOW5_EXEC merge -l false $INPUT_FILES -o $OUTPUT_DIR/lossy_merged_output.slow5 --to slow5 || die "tetcase $TESTCASE: $TESTNAME failed"
sort $REL_PATH/data/exp/merge/lossy_merged_expected.slow5 > $OUTPUT_DIR/lossy_merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/lossy_merged_output.slow5 > $OUTPUT_DIR/lossy_merged_output_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/lossy_merged_output.slow5 || die "remove $OUTPUT_DIR/lossy_merged_output.slow5 failed"
diff -q $OUTPUT_DIR/lossy_merged_expected_sorted.slow5 $OUTPUT_DIR/lossy_merged_output_sorted.slow5 || die "diff testcase $TESTCASE: $TESTNAME failed"

TESTCASE=3
TESTNAME="lossless merging with threads"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
$SLOW5_EXEC merge -t $NUM_THREADS $INPUT_FILES -o $OUTPUT_DIR/merged_output_using_threads.slow5 --to slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
sort $REL_PATH/data/exp/merge/merged_expected.slow5 > $OUTPUT_DIR/merged_expected_sorted.slow5 || die "sort failed"
sort $OUTPUT_DIR/merged_output_using_threads.slow5 > $OUTPUT_DIR/merged_output_using_threads_sorted.slow5 || die "sort failed"
rm $OUTPUT_DIR/merged_output_using_threads.slow5  || die "remove $OUTPUT_DIR/merged_output_using_threads.slow5 failed"
diff -q $OUTPUT_DIR/merged_expected_sorted.slow5 $OUTPUT_DIR/merged_output_using_threads_sorted.slow5 || die "diff testcase $TESTCASE: $TESTNAME failed"

# merging with and without enum data type
TESTCASE=4
TESTNAME="merging with and without enum type"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/aux_no_enum.slow5 $RAW_DIR/aux_enum.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_enum.slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/merged_output_enum.slow5  $OUTPUT_DIR/merged_output_enum.slow5 || die "tesetcase $TESTCASE: diff for $TESTNAME failed"

TESTCASE=5
TESTNAME="merging without and with enum type reverse input file order"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/aux_enum.slow5 $RAW_DIR/aux_no_enum.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_enum2.slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/merged_output_enum2.slow5  $OUTPUT_DIR/merged_output_enum2.slow5 || die "tesetcase $TESTCASE: diff for $TESTNAME failed"

# merging different slow5 formats and versions
TESTCASE=6
TESTNAME="merging different slow5 formats and versions"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/aux_no_enum.slow5 $RAW_DIR/none_v0.1.0.blow5 $RAW_DIR/zlib_svb-zd_v0.2.0.blow5 $RAW_DIR/zlib_v0.2.0.blow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_formats.slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/merged_output_formats.slow5  $OUTPUT_DIR/merged_output_formats.slow5 || die "tesetcase $TESTCASE: diff for $TESTNAME failed"

TESTCASE=7
TESTNAME="merging files where one header attr is missing in one file"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0_asic_id_missing.slow5 $RAW_DIR/rg1.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/asic_id_missing_expected_rg1.slow5  $OUTPUT_DIR/merged_output.slow5 || die "tesetcase $TESTCASE: diff for $TESTNAME"

TESTCASE=8
TESTNAME="blow5 output"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg1.slow5 $RAW_DIR/rg2.slow5 $RAW_DIR/rg3.slow5"
$SLOW5_EXEC merge $INPUT_FILES -c zlib -s svb-zd -o $OUTPUT_DIR/merged_output.blow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/merged_expected_zlib_svb.blow5  $OUTPUT_DIR/merged_output.blow5 || die "tesetcase $TESTCASE: diff for $TESTNAME"

arg_true="true"
arg_false="false"
TESTCASE=9
TESTNAME="merging files where one header attr is missing in one file"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0_asic_id_missing.slow5 $RAW_DIR/rg0.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 && die "tesetcase $TESTCASE: $TESTNAME failed"

TESTCASE=10
TESTNAME="merging files where one header attr is missing in one file with --allow true "
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0_asic_id_missing.slow5 $RAW_DIR/rg0.slow5"
$SLOW5_EXEC merge --allow $arg_true $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/asic_id_missing_expected.slow5  $OUTPUT_DIR/merged_output.slow5 || die "tesetcase $TESTCASE: diff for $TESTNAME"

TESTCASE=11
TESTNAME="merging files where one header attr is missing in one file (reversed input file order)"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_asic_id_missing.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 && die "tesetcase $TESTCASE: $TESTNAME failed"

TESTCASE=12
TESTNAME="merging files where one header attr is missing in one file (reversed input file order) with --allow true "
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_asic_id_missing.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 --allow $arg_true || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/asic_id_missing_expected.slow5  $OUTPUT_DIR/merged_output.slow5 || die "tesetcase $TESTCASE: diff for $TESTNAME"

TESTCASE=13
TESTNAME="same run_id different attribute values"
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_1.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 && die "tesetcase $TESTCASE: $TESTNAME failed"

TESTCASE=14
TESTNAME="same run_id different attribute values with --allow true "
info "-------------------tesetcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_1.slow5"
$SLOW5_EXEC merge --allow $arg_true $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 || die "tesetcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/same_run_id_different_attribute_values.slow5  $OUTPUT_DIR/merged_output.slow5 || die "tesetcase $TESTCASE: diff for $TESTNAME"

rm -r "$OUTPUT_DIR" || die "could not delete $OUTPUT_DIR"
info "done"
exit 0
