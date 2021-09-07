#!/bin/bash

# steps
# run stats program
# diff ouput with the expected

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color
die() { echo -e "${RED}$1${NC}" >&2 ; echo ; exit 1 ; } # terminate script
info() {  echo ; echo -e "${GREEN}$1${NC}" >&2 ; }
#...directories files tools arguments commands clean
# Relative path to "slow5tools/tests/"
REL_PATH="$(dirname $0)/"

RAW_DIR=test/data/raw/stats
EXP_DIR=test/data/exp/stats
OUTPUT_DIR=test/data/out/stats
test -d "$OUTPUT_DIR" && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Failed creating $OUTPUT_DIR"

SLOW5TOOLS_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5TOOLS="valgrind --leak-check=full --error-exitcode=1 $SLOW5TOOLS_WITHOUT_VALGRIND"
else
    SLOW5TOOLS=$SLOW5TOOLS_WITHOUT_VALGRIND
fi

TESTCASE=1
info "testcase$TESTCASE"
$SLOW5TOOLS --version || die "testcase$TESTCASE:slow5tools --version failed"

TESTCASE=2
info "testcase$TESTCASE"
$SLOW5TOOLS stats > $OUTPUT_DIR/output.log || die "testcase$TESTCASE: stats failed"

TESTCASE=3
info "testcase$TESTCASE"
$SLOW5TOOLS stats $RAW_DIR/exp_1_lossless.slow5 > $OUTPUT_DIR/output.log || die "testcase$TESTCASE: stats failed"
diff $OUTPUT_DIR/output.log "$EXP_DIR/exp_1_lossless.stdout"  > /dev/null || die "testcase$TESTCASE: diff failed"

TESTCASE=4
info "testcase$TESTCASE"
$SLOW5TOOLS stats $RAW_DIR/exp_1_lossy.blow5 > $OUTPUT_DIR/output.log || die "testcase$TESTCASE: stats failed"
diff $OUTPUT_DIR/output.log "$EXP_DIR/exp_1_lossy.stdout"  > /dev/null || die "testcase$TESTCASE: diff failed"

TESTCASE=5
info "testcase$TESTCASE"
$SLOW5TOOLS stats $RAW_DIR/zlib_svb-zd_multi_rg_v0.2.0.blow5> $OUTPUT_DIR/output.log || die "testcase$TESTCASE: stats failed"
diff $OUTPUT_DIR/output.log "$EXP_DIR/zlib_svb-zd_multi_rg_v0.2.0.stdout"  > /dev/null || die "testcase$TESTCASE: diff failed"


rm -r "$OUTPUT_DIR" || die "could not delete $OUTPUT_DIR"
info "all $TESTCASE testcases pased"
exit 0
