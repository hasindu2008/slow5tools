#!/bin/bash
# Run f2s, s2f, and again f2s and check if first produced slow5s are same as the last set.
Usage="f2s_s2f_integrity_test.sh [path to fast5 directory] [path to create a temporary directory] [path to slow5tools executable]"

if [[ "$#" -lt 3 ]]; then
	echo "Usage: $Usage"
	exit
fi

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

FAST5_DIR=$1
TEMP_DIR="$2/f2s_s2f_integrity_test"
SLOW5_EXEC=$3
F2S_atm1_OUTPUT="$TEMP_DIR/f2s_attempt1"
S2F_OUTPUT="$TEMP_DIR/s2f"
F2S_atm2_OUTPUT="$TEMP_DIR/f2s_attempt2"

rm -r "$TEMP_DIR"
mkdir "$TEMP_DIR" || exit 1
mkdir "$F2S_atm1_OUTPUT" || exit 1
mkdir "$S2F_OUTPUT" || exit 1
mkdir "$F2S_atm2_OUTPUT" || exit 1


echo "-------------------f2s attempt 1-------------------"
echo
if ! $SLOW5_EXEC f2s $FAST5_DIR -d $F2S_atm1_OUTPUT --iop 64 -s; then
    echo "f2s attempt 1 failed" 
    exit 1
fi


echo
echo "-------------------s2f attempt-------------------"
echo
if ! $SLOW5_EXEC s2f $F2S_atm1_OUTPUT -o $S2F_OUTPUT --iop 64; then
    echo "s2f failed"
    exit 1
fi

echo
echo "-------------------f2s attempt 2-------------------"
echo
if ! $SLOW5_EXEC f2s $S2F_OUTPUT -d $F2S_atm2_OUTPUT --iop 64 -s; then
    echo "f2s attempt 2 failed"
    exit 1
fi

echo "running diff on f2s attempt 1 and f2s attempt 2"
cmp -s $F2S_atm1_OUTPUT $F2S_atm2_OUTPUT

if [ $? -eq 0 ]; then
	echo -e "${GREEN}SUCCESS: f2s and s2f conversions are consistent!${NC}"
elif [ $? -eq 1 ]; then
	echo -e "${RED}FAILURE: f2s and s2f conversions are not consistent${NC}"
else
	echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
fi

exit