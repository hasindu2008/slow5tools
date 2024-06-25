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
# run stats program
# diff ouput with the expected

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

#...directories files tools arguments commands clean
# Relative path to "slow5tools/tests/"
REL_PATH="$(dirname $0)/"

RAW_DIR=test/data/raw/skim
EXP_DIR=test/data/exp/skim
OUTPUT_DIR=test/data/out/skim
test -d "$OUTPUT_DIR" && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Failed creating $OUTPUT_DIR"

SLOW5TOOLS_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5TOOLS="valgrind --leak-check=full --error-exitcode=1 $SLOW5TOOLS_WITHOUT_VALGRIND"
else
    SLOW5TOOLS=$SLOW5TOOLS_WITHOUT_VALGRIND
fi

if [ -z "$bigend" ]; then

TESTCASE=1
info "testcase$TESTCASE"
$SLOW5TOOLS skim $RAW_DIR/sp1_dna.blow5 > $OUTPUT_DIR/sp1_dna.txt || die "testcase$TESTCASE: skim failed"
diff $OUTPUT_DIR/sp1_dna.txt "$EXP_DIR/sp1_dna.exp"  > /dev/null || die "testcase$TESTCASE: diff failed"

TESTCASE=2
info "testcase$TESTCASE"
$SLOW5TOOLS skim $RAW_DIR/sequin_rna.blow5 > $OUTPUT_DIR/sequin_rna.txt  || die "testcase$TESTCASE: skim failed"
diff $OUTPUT_DIR/sequin_rna.txt "$EXP_DIR/sequin_rna.exp"  > /dev/null || die "testcase$TESTCASE: diff failed"

fi

info "all $TESTCASE testcases passed"
exit 0
