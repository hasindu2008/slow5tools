#!/bin/bash
# Run f2s with different file, input and output formats.
Usage="get_test.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color
die() { echo -e "${RED}$1${NC}" 1>&3 2>&4 ; echo ; exit 1 ; } # terminate script
info() {  echo ; echo -e "${GREEN}$1${NC}" 1>&3 2>&4 ; }

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

slow5tools_quickcheck() {
    info "running slow5tools_quickcheck for files in $PWD/${1}"
    ls -1 $PWD/${1}/**.[sb]low5 | xargs -n1 $SLOW5_EXEC quickcheck &> /dev/null
    if [ $? -eq 0 ]; then
        info "SUCCESS: slow5tools_quickcheck passed!"
    elif [ $? -eq 1 ]; then
        info "FAILURE: $PWD/${1} files are corrupted"
        exit 1
    else
        info "ERROR: slow5tools_quickcheck failed for some weird reason"
        exit 1
    fi
}

OUTPUT_DIR="$REL_PATH/data/out/get"
test -d $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir $OUTPUT_DIR || die "Creating $OUTPUT_DIR failed"

EXP_DIR=$REL_PATH/data/exp/get
RAW_DIR=$REL_PATH/data/raw/get
SLOW5_EXEC_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $SLOW5_EXEC_WITHOUT_VALGRIND"
else
    SLOW5_EXEC=$SLOW5_EXEC_WITHOUT_VALGRIND
fi

info "-------------------slow5tools version-------------------"
$SLOW5_EXEC --version || die "slow5tools version failed"

TESTCASE=1
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5" r1 --to slow5 > "$OUTPUT_DIR/extracted_reads.slow5" || die "testcase $TESTCASE failed"
slow5tools_quickcheck $OUTPUT_DIR
diff -q "$EXP_DIR/expected_extracted_reads.slow5" "$OUTPUT_DIR/extracted_reads.slow5" &>/dev/null
if [ $? -ne 0 ]; then
    info "${RED}ERROR: diff failed for 'slow5tools get testcase $TESTCASE'${NC}"
    exit 1
fi
info "testcase $TESTCASE passed"

TESTCASE=2
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5" r1 r5 r3 --to slow5 > "$OUTPUT_DIR/extracted_reads2.slow5" || die "testcase $TESTCASE failed"
slow5tools_quickcheck $OUTPUT_DIR
diff -q "$EXP_DIR/expected_extracted_reads2.slow5" "$OUTPUT_DIR/extracted_reads2.slow5" &>/dev/null
if [ $? -ne 0 ]; then
    info "${RED}ERROR: diff failed for 'slow5tools get testcase $TESTCASE'${NC}"
    exit 1
fi
info "testcase $TESTCASE passed"

TESTCASE=3
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5" --list "$RAW_DIR/list.txt" --to slow5 > "$OUTPUT_DIR/extracted_reads3.slow5" || die "testcase $TESTCASE failed"
slow5tools_quickcheck $OUTPUT_DIR
diff -q "$EXP_DIR/expected_extracted_reads3.slow5" "$OUTPUT_DIR/extracted_reads3.slow5" &>/dev/null
if [ $? -ne 0 ]; then
    info "${RED}ERROR: diff failed for 'slow5tools get testcase $TESTCASE'${NC}"
    exit 1
fi
info "testcase $TESTCASE passed"

TESTCASE=4
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5" -t 2 r1 r5 r3 --to slow5 > "$OUTPUT_DIR/extracted_reads2.slow5" || die "testcase $TESTCASE failed"
slow5tools_quickcheck $OUTPUT_DIR
diff -q "$EXP_DIR/expected_extracted_reads2.slow5" "$OUTPUT_DIR/extracted_reads2.slow5" &>/dev/null
if [ $? -ne 0 ]; then
    info "${RED}ERROR: diff failed for 'slow5tools get testcase $TESTCASE'${NC}"
    exit 1
fi
info "testcase $TESTCASE passed"

TESTCASE=5
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5"  r1 r5 r3 --to blow5 -c zlib -s none > "$OUTPUT_DIR/extracted_reads2.blow5" || die "testcase $TESTCASE failed"
slow5tools_quickcheck $OUTPUT_DIR
diff -q "$EXP_DIR/expected_extracted_reads.blow5" "$OUTPUT_DIR/extracted_reads2.blow5" &>/dev/null
if [ $? -ne 0 ]; then
    info "${RED}ERROR: diff failed for 'slow5tools get testcase $TESTCASE'${NC}"
    exit 1
fi
info "testcase $TESTCASE passed"

TESTCASE=6
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5" r1 r5 r3 -o "$OUTPUT_DIR/extracted_reads2.blow5"  -c zlib -s none  || die "testcase $TESTCASE failed"
slow5tools_quickcheck $OUTPUT_DIR
diff -q "$EXP_DIR/expected_extracted_reads.blow5" "$OUTPUT_DIR/extracted_reads2.blow5" &>/dev/null
if [ $? -ne 0 ]; then
    info "${RED}ERROR: diff failed for 'slow5tools get testcase $TESTCASE'${NC}"
    exit 1
fi
info "testcase $TESTCASE passed"

TESTCASE=7
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5" --list "$RAW_DIR/list_with_invalid_reads.txt" --to slow5 > "$OUTPUT_DIR/extracted_reads7.slow5" && die "testcase $TESTCASE failed"
rm "$OUTPUT_DIR/extracted_reads7.slow5"
info "testcase $TESTCASE passed"

TESTCASE=8
info "------------------- slow5tools get testcase $TESTCASE -------------------"
$SLOW5_EXEC get "$RAW_DIR/example2.slow5" --skip --list "$RAW_DIR/list_with_invalid_reads.txt" --to slow5 > "$OUTPUT_DIR/extracted_reads8.slow5" || die "testcase $TESTCASE failed"
slow5tools_quickcheck $OUTPUT_DIR
info "testcase $TESTCASE passed"

rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed" 1>&3 2>&4
exit 0
