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

# Run f2s, s2f, and again f2s and check if first produced slow5s are same as the last set.
Usage1="f2s_s2f_integrity_test.sh"
Usage2="f2s_s2f_integrity_test.sh [path to fast5 directory] [path to create a temporary directory][-c or --to (optional) [clean_fscache -f (optional)]"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

# terminate script
die() {
    echo -e "${RED}$1${NC}" >&2
    echo
    exit 1
}

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $REL_PATH/../slow5tools"
else
    SLOW5_EXEC=$REL_PATH/../slow5tools
fi

FAST5_DIR="$REL_PATH/data/raw/f2s_s2f_integrity/"
TEMP_DIR="$REL_PATH/data/out/f2s_s2f_integrity/"
if [[ "$#" -ge 2 ]]; then
  FAST5_DIR=$1
  TEMP_DIR="$2/f2s_s2f_integrity_test"
fi

F2S_atm1_OUTPUT="$TEMP_DIR/f2s_attempt1"
S2F_OUTPUT="$TEMP_DIR/s2f"
F2S_atm2_OUTPUT="$TEMP_DIR/f2s_attempt2"

SLOW5_FORMAT="--to slow5"
if [[ "$#" -eq 4 ]]; then
    SLOW5_FORMAT=$4
fi

test -d "$TEMP_DIR" && rm -r "$TEMP_DIR"
mkdir "$TEMP_DIR" || die "Creating $TEMP_DIR failed"
mkdir "$F2S_atm1_OUTPUT" || die "Creating $F2S_atm1_OUTPUT failed"
mkdir "$S2F_OUTPUT" || die "Creating $S2F_OUTPUT failed"
mkdir "$F2S_atm2_OUTPUT" || die "Creating $F2S_atm2_OUTPUT failed"


if [[ $* == *-f* ]];then
  clean_fscache
fi
echo "-------------------f2s attempt 1-------------------"
echo
$SLOW5_EXEC f2s "$FAST5_DIR" -d "$F2S_atm1_OUTPUT" --iop 64 $SLOW5_FORMAT 2>/dev/null || die "f2s attempt 1 failed"
if [[ $* == *-f* ]];then
  clean_fscache
fi
echo
echo "-------------------s2f attempt-------------------"
echo
$SLOW5_EXEC s2f "$F2S_atm1_OUTPUT" -d "$S2F_OUTPUT" --iop 64 2>/dev/null || die "s2f failed"
if [[ $* == *-f* ]];then
  clean_fscache
fi
echo
echo "-------------------f2s attempt 2-------------------"
echo
$SLOW5_EXEC f2s "$S2F_OUTPUT" -d "$F2S_atm2_OUTPUT" --iop 64 $SLOW5_FORMAT 2>/dev/null || die "f2s attempt 2 failed"
echo "running diff on f2s attempt 1 and f2s attempt 2"
echo "du -hs $F2S_atm1_OUTPUT"
du -hs "$F2S_atm1_OUTPUT"
echo "du -hs $F2S_atm2_OUTPUT"
du -hs "$F2S_atm2_OUTPUT"
echo "ls $F2S_atm1_OUTPUT | wc"
ls "$F2S_atm1_OUTPUT" | wc
echo "ls $F2S_atm2_OUTPUT | wc"
ls "$F2S_atm2_OUTPUT" | wc
echo "f2s might not create the same exact header lines (starting with '@') from after s2f fast5s"
echo "Running diff only on lines starting with '@'. If there are no differences the following line is blank"
diff --ignore-matching-lines=?@ "$F2S_atm1_OUTPUT" "$F2S_atm2_OUTPUT"
echo
echo "Again running diff (ignoring header lines starting with '@'"
diff --ignore-matching-lines=@ "$F2S_atm1_OUTPUT" "$F2S_atm2_OUTPUT" > /dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: f2s and s2f conversions are consistent!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: f2s and s2f conversions are not consistent${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

rm -r "$TEMP_DIR" || die "Removing $TEMP_DIR failed"

exit 0