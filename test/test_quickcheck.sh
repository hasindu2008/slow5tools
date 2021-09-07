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
info "testcase$TESTCASE"
$SLOW5TOOLS --version || die "testcase$TESTCASE:slow5tools --version failed"

TESTCASE=2
info "testcase$TESTCASE"
$SLOW5TOOLS quickcheck $RAW_DIR/good.blow5 || die "testcase$TESTCASE: stats failed"

TESTCASE=3
info "testcase$TESTCASE (should error out)"
$SLOW5TOOLS stats $RAW_DIR/bad.blow5 && die "testcase$TESTCASE: stats failed"

info "all $TESTCASE testcases pased"
exit 0
# If you want to log to the same file: command1 >> log_file 2>&1
# If you want different files: command1 >> log_file 2>> err_file
# use ANSI syntax format to view stdout/stderr on SublimeText
# use bash -n [script] and shellcheck [script] to check syntax