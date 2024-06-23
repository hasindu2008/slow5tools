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

# test slow5 index

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

OUTPUT_DIR="$REL_PATH/data/out/slow5tools_index"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir $OUTPUT_DIR || die "Creating $OUTPUT_DIR failed"

SLOW5_DIR=$REL_PATH/data/exp/index
SLOW5_EXEC_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $SLOW5_EXEC_WITHOUT_VALGRIND"
else
    SLOW5_EXEC=$SLOW5_EXEC_WITHOUT_VALGRIND
fi

info "-------------------slow5tools version-------------------"
$SLOW5_EXEC --version || die "slow5tools version failed"


if [ -z "$bigend" ]; then
echo
TESTCASE_NO=1
echo "------------------- slow5tools index testcase ${TESTCASE_NO} -------------------"
$SLOW5_EXEC index $SLOW5_DIR/example_multi_rg_v0.1.0.blow5 || die "testcase ${TESTCASE_NO} failed"
diff -q $SLOW5_DIR/example_multi_rg_v0.1.0.blow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.1.0.blow5.idx || die "ERROR: diff failed for testcase ${TESTCASE_NO}"
echo -e "${GREEN}testcase ${TESTCASE_NO} passed${NC}"  1>&3 2>&4
fi

echo
TESTCASE_NO=2
echo "------------------- slow5tools index testcase ${TESTCASE_NO} -------------------"
$SLOW5_EXEC index $SLOW5_DIR/example_multi_rg_v0.1.0.slow5 || die "testcase ${TESTCASE_NO} failed"
diff -q $SLOW5_DIR/example_multi_rg_v0.1.0.slow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.1.0.slow5.idx || die "ERROR: diff failed for testcase ${TESTCASE_NO}"
echo -e "${GREEN}testcase ${TESTCASE_NO} passed${NC}"  1>&3 2>&4

if [ -z "$bigend" ]; then
echo
TESTCASE_NO=3
echo "------------------- slow5tools index testcase ${TESTCASE_NO} -------------------"
$SLOW5_EXEC index $SLOW5_DIR/example_multi_rg_v0.2.0.blow5 || die "testcase ${TESTCASE_NO} failed"
diff -q $SLOW5_DIR/example_multi_rg_v0.2.0.blow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.2.0.blow5.idx || die "ERROR: diff failed for testcase ${TESTCASE_NO}"
echo -e "${GREEN}testcase ${TESTCASE_NO} passed${NC}"  1>&3 2>&4

echo
TESTCASE_NO=4
echo "------------------- slow5tools index testcase ${TESTCASE_NO} -------------------"
$SLOW5_EXEC view $SLOW5_DIR/example_multi_rg_v0.2.0.blow5 -c none -s none -o $SLOW5_DIR/example_multi_rg_v0.2.0_none_none.blow5 || die "testcase ${TESTCASE_NO} failed"
$SLOW5_EXEC index $o $SLOW5_DIR/example_multi_rg_v0.2.0_none_none.blow5 || die "testcase ${TESTCASE_NO} failed"
diff -q $SLOW5_DIR/example_multi_rg_v0.2.0_none_none.blow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.2.0_none_none.blow5.idx || die "ERROR: diff failed for testcase ${TESTCASE_NO}"
echo -e "${GREEN}testcase ${TESTCASE_NO} passed${NC}"  1>&3 2>&4
fi

if [ "$zstd" = "1" ]; then
  echo
  TESTCASE_NO=5
  echo "------------------- slow5tools index testcase ${TESTCASE_NO} -------------------"
  # file included as the file may differ based on zstd version
  $SLOW5_EXEC index $o $SLOW5_DIR/example_multi_rg_v0.2.0_zstd_svb-zd.blow5  || die "testcase ${TESTCASE_NO} failed"
  diff -q $SLOW5_DIR/example_multi_rg_v0.2.0_zstd_svb-zd.blow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.2.0_zstd_svb-zd.blow5.idx || die "ERROR: diff failed for testcase ${TESTCASE_NO}"
  echo -e "${GREEN}testcase ${TESTCASE_NO} passed${NC}"  1>&3 2>&4
fi

echo
TESTCASE_NO=6
echo "------------------- slow5tools index testcase ${TESTCASE_NO} -------------------"
$SLOW5_EXEC index $SLOW5_DIR/duplicate_read.blow5 && die "testcase ${TESTCASE_NO} failed"
echo -e "${GREEN}testcase ${TESTCASE_NO} passed${NC}"  1>&3 2>&4


rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
