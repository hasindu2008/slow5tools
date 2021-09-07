#!/bin/bash

# steps
# run quickcheck program
# diff ouput with the expected

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color
die() { echo -e "${RED}$1${NC}" >&2 ; echo ; exit 1 ; } # terminate script
info() {  echo ; echo -e "${GREEN}$1${NC}" >&2 ; }
#...directories files tools arguments commands clean
# Relative path to "slow5tools/tests/"
REL_PATH="$(dirname $0)/"
RAW_DIR=$REL_PATH/data/raw/quickcheck
SLOW5TOOLS_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5TOOLS="valgrind --leak-check=full --error-exitcode=1 $SLOW5TOOLS_WITHOUT_VALGRIND"
else
    SLOW5TOOLS=$SLOW5TOOLS_WITHOUT_VALGRIND
fi

TESTCASE=1
$SLOW5TOOLS --version || die "testcase$TESTCASE:slow5tools --version failed"

GOOD_LIST="exp_1_lossless_good.slow5 exp_1_lossy_good.blow5 zlib_svb-zd_multi_rg_v0.2.0_good.blow5"
BAD_LIST="exp_1_lossless_bad_rg_missing.slow5  exp_1_lossy_bad_eof.blow5  zlib_svb-zd_multi_rg_v0.2.0_bad_hdr_len.blow5"

TESTCASE=2
for each in $GOOD_LIST
do
    info "testcase$TESTCASE"
    $SLOW5TOOLS quickcheck $RAW_DIR/${each} 2> /dev/null || die "testcase$TESTCASE: stats failed"
    TESTCASE=$((TESTCASE + 1))
done

for each in $BAD_LIST
do
    info "testcase$TESTCASE (should error out)"
    $SLOW5TOOLS quickcheck $RAW_DIR/${each} 2> /dev/null && die "testcase$TESTCASE: stats failed"
    TESTCASE=$((TESTCASE + 1))
done

info "all $TESTCASE testcases pased"
exit 0
