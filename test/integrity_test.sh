#!/bin/bash
# Run f2s, s2f, and again f2s and check if first produced slow5s are same as the last set.
Usage="integrity_test.sh [path to fast5 directory] [path to create a temporary directory] [path to slow5tools executable]"

if [[ "$#" -lt 3 ]]; then
	echo "Usage: $Usage"
fi


FAST5_DIR=$1
TEMP_DIR="$2/integrity_test"
SLOW5_EXEC=$3
F2S_atm1_OUTPUT="$TEMP_DIR/f2s_attempt1"
S2F_OUTPUT="$TEMP_DIR/s2f"
F2S_atm2_OUTPUT="$TEMP_DIR/f2s_attempt2"

mkdir "$TEMP_DIR"
mkdir "$F2S_atm1_OUTPUT"
mkdir "$S2F_OUTPUT"
mkdir "$F2S_atm2_OUTPUT"


echo "-------------------f2s attempt 1-------------------"
echo
if ! $SLOW5_EXEC f2s $FAST5_DIR -d $F2S_atm1_OUTPUT --iop 64 -s; then
    echo "f2s attempt 1 failed"
fi

echo
echo "-------------------s2f attempt-------------------"
echo
if ! $SLOW5_EXEC s2f $F2S_atm1_OUTPUT -o $S2F_OUTPUT --iop 64; then
    echo "s2f failed"
fi

echo
echo "-------------------f2s attempt 2-------------------"
echo
if ! $SLOW5_EXEC f2s $S2F_OUTPUT -d $F2S_atm2_OUTPUT --iop 64 -s; then
    echo "f2s attempt 2 failed"
fi

echo "running diff on attempt 1 and attempt 2"
diff -bur $F2S_atm1_OUTPUT $F2S_atm2_OUTPUT


