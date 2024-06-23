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

# Run s2f with different file, input and output formats.
Usage="test_s2f.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color

# terminate script
die() {
    echo -e "${RED}$1${NC}" 1>&3 2>&4
    echo
    exit 1
}
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
RAW_DIR=$REL_PATH/data/raw/s2f
EXP_SLOW5_DIR=$REL_PATH/data/exp/f2s

OUTPUT_DIR="$REL_PATH/data/out/s2f"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Creating $OUTPUT_DIR failed"

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $REL_PATH/../slow5tools"
else
    SLOW5_EXEC=$REL_PATH/../slow5tools
fi

echo "-------------------slow5tools version-------------------"
$SLOW5_EXEC --version || die "slow5tools version failed"
echo

TESTCASE_NO=1
TESTNAME=".slow5 to .fast5"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
$SLOW5_EXEC s2f $RAW_DIR/a.slow5 -o $OUTPUT_DIR/a.fast5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=2
TESTNAME=".slow5 to .fast5 (output directory given)"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
$SLOW5_EXEC s2f $RAW_DIR/a.slow5 -d $OUTPUT_DIR/a || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=3
TESTNAME=".slow5 (in current directory) to .fast5"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
cd $RAW_DIR
CD_BACK=../../../..
$CD_BACK/slow5tools -v 7 s2f a.slow5 -o $CD_BACK/$OUTPUT_DIR/b.fast5 || die "testcase $TESTCASE_NO failed"
cd $CD_BACK
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=4
TESTNAME=".slow5 (in current directory) to .fast5 (output directory given)"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
cd $RAW_DIR
CD_BACK=../../../..
$CD_BACK/slow5tools s2f a.slow5 -d $CD_BACK/$OUTPUT_DIR/b || die "testcase $TESTCASE_NO failed"
cd $CD_BACK
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=5
TESTNAME="end_reason is an enum"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
$SLOW5_EXEC s2f $RAW_DIR/end_reason_is_enum.slow5 -o $OUTPUT_DIR/end_reason_is_enum.fast5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6
TESTNAME="end_reason is an unint8_t"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
$SLOW5_EXEC s2f $RAW_DIR/end_reason_is_unint8_t.slow5 -o $OUTPUT_DIR/end_reason_is_unint8_t.fast5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7
TESTNAME="end_reason is empty"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
$SLOW5_EXEC s2f $RAW_DIR/end_reason_empty.slow5 -o $OUTPUT_DIR/end_reason_empty.fast5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

if [ -z "$bigend" ]; then
TESTCASE_NO=8
TESTNAME="median_before is empty"
echo "-------------------testcase:$TESTCASE_NO: $TESTNAME-------------------"
$SLOW5_EXEC s2f $RAW_DIR/median_before_nan.blow5 -o $OUTPUT_DIR/median_before_nan.fast5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4
fi

#rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
