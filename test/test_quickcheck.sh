#!/bin/bash

# MIT License

# Copyright (c) 2020 Hiruna Samarakoon
# Copyright (c) 2020 Sasha Jenner
# Copyright (c) 2020,2023 Hasindu Gamaarachchi

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

###############################################################################

# steps
# run quickcheck program
# diff ouput with the expected

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color
die() { echo -e "${RED}$1${NC}" >&2 ; echo ; exit 1 ; } # terminate script
info() {  echo -e "${GREEN}$1${NC}" >&2 ; }
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
$SLOW5TOOLS --version > /dev/null 2> /dev/null || die "testcase$TESTCASE:slow5tools --version failed"

if [ -z "$bigend" ]; then
    GOOD_LIST="exp_1_lossless_good.slow5 exp_1_lossy_good.blow5 zlib_svb-zd_multi_rg_v0.2.0_good.blow5"
    BAD_LIST="exp_1_lossless_bad_rg_missing.slow5  exp_1_lossy_bad_eof.blow5  zlib_svb-zd_multi_rg_v0.2.0_bad_hdr_len.blow5"
else
    GOOD_LIST="exp_1_lossless_good.slow5 exp_1_lossy_good.blow5"
    BAD_LIST="exp_1_lossless_bad_rg_missing.slow5  exp_1_lossy_bad_eof.blow5"
fi

TESTCASE=2
for each in $GOOD_LIST
do
    info "testcase$TESTCASE"
    $SLOW5TOOLS quickcheck $RAW_DIR/${each} 2> /dev/null || die "testcase$TESTCASE: stats failed"
    TESTCASE=$((TESTCASE + 1))
done

for each in $BAD_LIST
do
    info "testcase$TESTCASE"
    $SLOW5TOOLS quickcheck $RAW_DIR/${each} 2> /dev/null && die "testcase$TESTCASE: stats failed"
    TESTCASE=$((TESTCASE + 1))
done

info "all $TESTCASE testcases passed"
exit 0
