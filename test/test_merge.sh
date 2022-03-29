#!/bin/bash

# steps
# run merge for different testcases
# diff

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color

die() {
    echo -e "${RED}$1${NC}" 1>&3 2>&4
    echo
    exit 1
}

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

#redirect
verbose=0
exec 3>&1
exec 4>&2
if ((verbose)); then
  echo "verbose=1"
else
  echo "verbose=0"
  exec 1>/dev/null
  exec 2>/dev/null
fi
#echo "this should be seen if verbose"
#echo "this should always be seen" 1>&3 2>&4


# quick check if redundant here as we any way do a diff
slow5tools_quickcheck() {
    if $REL_PATH/../slow5tools quickcheck $1; then
        echo -e "${GREEN}SUCCESS: slow5tools_quickcheck passed!${NC}" 
    else
        echo -e "${RED}ERROR: slow5tools_quickcheck failed${NC}" 1>&3 2>&4
        exit 1
    fi
}

TESTCASE=0
info "-------------------testcase $TESTCASE: slow5tools version-------------------"
$SLOW5_EXEC --version || die "testcase TESTCASE: slow5tools version failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.1
TESTNAME="lossless merging of 4 different read groups"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg1.slow5 $RAW_DIR/rg2.slow5 $RAW_DIR/rg3.slow5"
OUTPUT_FILE=merged_different_rg.slow5
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/$OUTPUT_FILE || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/$OUTPUT_FILE $OUTPUT_DIR/$OUTPUT_FILE || die "testcase $TESTCASE: diff for $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.2
TESTNAME="lossless merging with -t 1 "
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/$OUTPUT_FILE -t1  || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/$OUTPUT_FILE $OUTPUT_DIR/$OUTPUT_FILE || die "testcase $TESTCASE: diff for $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.3
TESTNAME="lossless merging to stdout "
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
$SLOW5_EXEC merge $INPUT_FILES --to slow5 > $OUTPUT_DIR/$OUTPUT_FILE   || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/$OUTPUT_FILE $OUTPUT_DIR/$OUTPUT_FILE || die "testcase $TESTCASE: diff for $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.4
TESTNAME="lossy merging of 4 different read groups"
info "-------------------tetcase $TESTCASE: $TESTNAME-------------------"
OUTPUT_FILE=lossy_merged_different_rg.slow5
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/$OUTPUT_FILE --lossless false || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/$OUTPUT_FILE $OUTPUT_DIR/$OUTPUT_FILE || die "testcase $TESTCASE: diff for $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.4
TESTNAME="merging different slow5 input formats (slow5, blow5 with diff compression) and versions (0.1.0 and 0.2.0)"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/aux_no_enum.slow5 $RAW_DIR/none_v0.1.0.blow5 $RAW_DIR/zlib_svb-zd_v0.2.0.blow5 $RAW_DIR/zlib_v0.2.0.blow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_formats.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/merged_output_formats.slow5  $OUTPUT_DIR/merged_output_formats.slow5 || die "testcase $TESTCASE: diff for $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

#legacy space instead of "." in header vals and aux field vals

TESTCASE=1.5
# coverts BLOW5 output including EOF
TESTNAME="blow5 output"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg1.slow5 $RAW_DIR/rg2.slow5 $RAW_DIR/rg3.slow5"
$SLOW5_EXEC merge $INPUT_FILES -c zlib -s svb-zd -o $OUTPUT_DIR/merged_output.blow5 || die "testcase $TESTCASE: $TESTNAME failed"
slow5tools_quickcheck $OUTPUT_DIR/merged_output.blow5
diff -q $REL_PATH/data/exp/merge/merged_expected_zlib_svb.blow5  $OUTPUT_DIR/merged_output.blow5 || die "testcase $TESTCASE: diff for $TESTNAME"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.6
TESTNAME="merging files where one header attr is missing in one read group while it is there in the other"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0_asic_id_missing.slow5 $RAW_DIR/rg1.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
slow5tools_quickcheck $OUTPUT_DIR/merged_output.slow5
diff -q $REL_PATH/data/exp/merge/rg0_asic_id_missing_with_rg1.slow5  $OUTPUT_DIR/merged_output.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.7
TESTNAME="merging files where one header attr is missing in one read group while it is there in the other (reversed input order)"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg1.slow5 $RAW_DIR/rg0_asic_id_missing.slow5 "
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/rg1_with_rg0_asic_id_missing.slow5  $OUTPUT_DIR/merged_output.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.7
TESTNAME="merge same read group"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_1.slow5 "
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/same_rg.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/same_rg.slow5  $OUTPUT_DIR/same_rg.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.7
TESTNAME="merge same read group + diff readgroup"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg1.slow5 $RAW_DIR/rg0_1.slow5 "
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/same_rg_and_diff_rg.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/same_rg_and_diff_rg.slow5  $OUTPUT_DIR/same_rg_and_diff_rg.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.8
#this test also check what happens when header attributes in an input are not in sort order (heatsink_temp and version are moved not to be in alpha order)
TESTNAME="merge same read group with different aux field order"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_2_aux_order.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/same_rg_aux_order.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/same_rg_aux_order.slow5  $OUTPUT_DIR/same_rg_aux_order.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=1.9
#this test also check what happens when header attributes in an input are not in sort order (heatsink_temp and version are moved not to be in alpha order)
TESTNAME="merge different read group with different aux field order"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0_2_aux_order.slow5 $RAW_DIR/rg1.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/diff_rg_aux_order.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/diff_rg_aux_order.slow5  $OUTPUT_DIR/diff_rg_aux_order.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

## bloody enum

# merging with and without enum data type
TESTCASE=2.1
TESTNAME="merging with and without enum type"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/aux_no_enum.slow5 $RAW_DIR/aux_enum.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_enum.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/merged_output_enum.slow5  $OUTPUT_DIR/merged_output_enum.slow5 || die "testcase $TESTCASE: diff for $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=2.2
TESTNAME="merging without and with enum type reverse input file order"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/aux_enum.slow5 $RAW_DIR/aux_no_enum.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output_enum2.slow5 || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/merged_output_enum2.slow5  $OUTPUT_DIR/merged_output_enum2.slow5 || die "testcase $TESTCASE: diff for $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

# enum different labels in same read group
# enum labels order different
# enum different labels in different read groups

# enum data type different

# data type mismatch in aux fields



## weird cases where merging should fail unless --allow is specified
TESTCASE=3.1
TESTNAME="merging files where one header attr is missing in one file in same read group"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0_asic_id_missing.slow5 $RAW_DIR/rg0.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 2> $OUTPUT_DIR/err.log  && die "testcase $TESTCASE: $TESTNAME failed"
grep -q "ERROR.* Attributes are different for the same run_id.*" $OUTPUT_DIR/err.log || die "ERROR: testcase $TESTCASE: $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=3.2
TESTNAME="merging files where one header attr is missing in one file in same read group with --allow "
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0_asic_id_missing.slow5 $RAW_DIR/rg0.slow5"
$SLOW5_EXEC merge -a $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 2> $OUTPUT_DIR/err.log || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/asic_id_missing_expected.slow5  $OUTPUT_DIR/merged_output.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
grep -q "WARNING" $OUTPUT_DIR/err.log || die "WARNING: testcase $TESTCASE: $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=3.3
TESTNAME="merging files where one header attr is missing in one file in same read group (reversed input file order)"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_asic_id_missing.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE: $TESTNAME failed"
grep -q "ERROR.* Attributes are different for the same run_id.*" $OUTPUT_DIR/err.log || die "ERROR: testcase $TESTCASE: $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=3.4
TESTNAME="merging files where one header attr is missing in one file in same read group (reversed input file order) with --allow "
#asic ID is set to . in merged output
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_asic_id_missing.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 -a 2> $OUTPUT_DIR/err.log || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/asic_id_missing_expected.slow5  $OUTPUT_DIR/merged_output.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
grep -q "WARNING" $OUTPUT_DIR/err.log || die "WARNING: testcase $TESTCASE: $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=3.5
TESTNAME=" different attribute values in same read group"
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_diff_attr.slow5"
$SLOW5_EXEC merge $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE: $TESTNAME failed"
grep -q "ERROR.* Attributes are different for the same run_id.*" $OUTPUT_DIR/err.log || die "ERROR: testcase $TESTCASE: $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

TESTCASE=3.6
TESTNAME="different attribute values in same read group with --allow "
info "-------------------testcase $TESTCASE: $TESTNAME-------------------"
#attribute value for those which differ are set to . in merged output
INPUT_FILES="$RAW_DIR/rg0.slow5 $RAW_DIR/rg0_diff_attr.slow5"
$SLOW5_EXEC merge --allow  $INPUT_FILES -o $OUTPUT_DIR/merged_output.slow5 2> $OUTPUT_DIR/err.log || die "testcase $TESTCASE: $TESTNAME failed"
diff -q $REL_PATH/data/exp/merge/same_run_id_different_attribute_values.slow5  $OUTPUT_DIR/merged_output.slow5 || die "testcase $TESTCASE: diff for $TESTNAME"
grep -q "WARNING" $OUTPUT_DIR/err.log || die "ERROR: testcase $TESTCASE: $TESTNAME failed"
echo -e "${GREEN}testcase $TESTCASE passed${NC}" 1>&3 2>&4

# different aux fields in same read group

rm -r "$OUTPUT_DIR" || die "could not delete $OUTPUT_DIR"
info "done"
exit 0

