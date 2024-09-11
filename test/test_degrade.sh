#!/bin/bash

# MIT License

# Copyright (c) 2020 Hiruna Samarakoon
# Copyright (c) 2020,2024 Sasha Jenner
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
# run degrade program
# diff output with the expected

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
REL_PATH=$(dirname "$0")/

RAW_DIR="$REL_PATH/data/raw/degrade"
EXP_DIR="$REL_PATH/data/exp/degrade"
OUT_DIR="$REL_PATH/data/out/degrade"
test -d "$OUT_DIR" && rm -r "$OUT_DIR"
mkdir "$OUT_DIR" || die "Failed creating $OUT_DIR"

SLOW5TOOLS_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5TOOLS="valgrind --leak-check=full --error-exitcode=1 $SLOW5TOOLS_WITHOUT_VALGRIND"
else
    SLOW5TOOLS=$SLOW5TOOLS_WITHOUT_VALGRIND
fi

i=1
name="testcase $i: 1 bit"
$SLOW5TOOLS degrade "$RAW_DIR/example2.slow5" -o "$OUT_DIR/example2_b1.slow5" --bits=1 || die "$name: slow5tools failed"
diff "$OUT_DIR/example2_b1.slow5" "$EXP_DIR/example2_b1.slow5" > /dev/null || die "$name: diff failed"
info "$name"

if [ -z "$bigend" ]; then
    i=$((i + 1))
    name="testcase $i: 4 bits"
    $SLOW5TOOLS degrade -b 4 "$RAW_DIR/example2.slow5" -o "$OUT_DIR/example2_b4.blow5" || die "$name: slow5tools failed"
    diff "$OUT_DIR/example2_b4.blow5" "$EXP_DIR/example2_b4.blow5" > /dev/null || die "$name: diff failed"
    info "$name"

    i=$((i + 1))
    name="testcase $i: minion r10 dna"
    $SLOW5TOOLS degrade "$RAW_DIR/minir10dna.blow5" -o "$OUT_DIR/minir10dna_auto.blow5" || die "$name: slow5tools failed"
    diff "$OUT_DIR/minir10dna_auto.blow5" "$EXP_DIR/minir10dna_b3.blow5" > /dev/null || die "$name: diff failed"
    info "$name"

    i=$((i + 1))
    name="testcase $i: promethion r10 dna 4khz"
    $SLOW5TOOLS degrade "$RAW_DIR/promr10dna4khz.blow5" -s svb-zd -o "$OUT_DIR/promr10dna4khz_auto.blow5" || die "$name: slow5tools failed"
    diff "$OUT_DIR/promr10dna4khz_auto.blow5" "$EXP_DIR/promr10dna4khz_b3.blow5" > /dev/null || die "$name: diff failed"
    info "$name"

    i=$((i + 1))
    name="testcase $i: promethion r10 dna 5khz"
    $SLOW5TOOLS degrade "$RAW_DIR/promr10dna5khz.blow5" -o "$OUT_DIR/promr10dna5khz_auto.blow5" || die "$name: slow5tools failed"
    diff "$OUT_DIR/promr10dna5khz_auto.blow5" "$EXP_DIR/promr10dna5khz_b3.blow5" > /dev/null || die "$name: diff failed"
    info "$name"
fi

i=$((i + 1))
name="testcase $i: promethion r10 dna: bad header"
! $SLOW5TOOLS degrade "$RAW_DIR/promr10dna_badhdr.slow5" -o "$OUT_DIR/promr10dna_badhdr_auto.slow5" || die "$name: slow5tools failed"
info "$name"

i=$((i + 1))
name="testcase $i: promethion r10 dna: bad header sample frequency"
! $SLOW5TOOLS degrade "$RAW_DIR/promr10dna_badhdr_sample_freq.slow5" > /dev/null || die "$name: slow5tools failed"
info "$name"

i=$((i + 1))
name="testcase $i: promethion r10 dna: bad header sample rate"
! $SLOW5TOOLS degrade "$RAW_DIR/promr10dna_badhdr_sample_rate.slow5" > /dev/null || die "$name: slow5tools failed"
info "$name"

i=$((i + 1))
name="testcase $i: promethion r10 dna: bad header sample rate 2"
! $SLOW5TOOLS degrade "$RAW_DIR/promr10dna_badhdr_sample_rate2.slow5" > /dev/null || die "$name: slow5tools failed"
info "$name"

i=$((i + 1))
name="testcase $i: promethion r10 dna: bad record"
! $SLOW5TOOLS degrade "$RAW_DIR/promr10dna_badrec.slow5" > /dev/null || die "$name: slow5tools failed"
info "$name"

exit 0
