#!/bin/bash

# @author: Hiruna Samarakoon (hirunas@eng.pdn.ac.lk)

###############################################################################

# first do f2s, s2f and then basecall

Usage="test_s2f_with_guppy.sh [path to fast5 directory] [path to create a temporary directory] [path to slow5tools executable] [path to guppy executable]"

if [[ "$#" -lt 4 ]]; then
	echo "Usage: $Usage"
	exit 1
fi

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

FAST5_DIR=$1
TEST_DIR=$2
SLOWTOOLS=$3
GUPPY_BASECALLER=$4

F2S_OUTPUT_DIR=$TEST_DIR/f2s
S2F_OUTPUT_DIR=$TEST_DIR/s2f
GUPPY_OUTPUT_ORIGINAL=$TEST_DIR/guppy_output_original
GUPPY_OUTPUT_S2F=$TEST_DIR/guppy_output_s2f

# create test directory
test -d  $TEST_DIR && rm -r $TEST_DIR
mkdir $TEST_DIR

IOP=4
LOSSY_F2S=-l

$SLOWTOOLS f2s $FAST5_DIR -d $F2S_OUTPUT_DIR --iop $IOP
$SLOWTOOLS s2f $F2S_OUTPUT_DIR -d $S2F_OUTPUT_DIR --iop $IOP

$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $FAST5_DIR -s $GUPPY_OUTPUT_ORIGINAL -r --device cuda:all
$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $S2F_OUTPUT_DIR -s $GUPPY_OUTPUT_S2F -r --device cuda:all

PASS_FAIL_STRUCTURE=0

if test -d $GUPPY_OUTPUT_S2F/pass; then
    PASS_FAIL_STRUCTURE=1
    sed -i 's/sampleid=[^ ]*/sampleid=/g' $GUPPY_OUTPUT_ORIGINAL/pass/*.fast*
    sort -o $GUPPY_OUTPUT_ORIGINAL/pass/*.fast* $GUPPY_OUTPUT_ORIGINAL/pass/*.fast*

    sed -i 's/sampleid=[^ ]*/sampleid=/g' $GUPPY_OUTPUT_S2F/pass/*.fast*
    sort -o $GUPPY_OUTPUT_S2F/pass/*.fast* $GUPPY_OUTPUT_S2F/pass/*.fast*

    echo "diff sorted pass files"
    diff $GUPPY_OUTPUT_ORIGINAL/pass $GUPPY_OUTPUT_S2F/pass &>/dev/null

    if [ $? -ne 0 ]; then
      echo -e "${RED}ERROR: diff failed for $GUPPY_OUTPUT_ORIGINAL/pass and $GUPPY_OUTPUT_S2F/pass files ${NC}"
      exit 1
    fi
    echo -e "${GREEN}diff passed${NC}"
fi

if test -d $GUPPY_OUTPUT_S2F/fail; then
    PASS_FAIL_STRUCTURE=1
    sed -i 's/sampleid=[^ ]*/sampleid=/g' $GUPPY_OUTPUT_ORIGINAL/fail/*.fast*
    sort -o $GUPPY_OUTPUT_ORIGINAL/fail/*.fast* $GUPPY_OUTPUT_ORIGINAL/fail/*.fast*

    sed -i 's/sampleid=[^ ]*/sampleid=/g' $GUPPY_OUTPUT_S2F/fail/*.fast*
    sort -o $GUPPY_OUTPUT_S2F/fail/*.fast* $GUPPY_OUTPUT_S2F/fail/*.fast*

    echo "diff sorted fail files"
    diff $GUPPY_OUTPUT_ORIGINAL/fail $GUPPY_OUTPUT_S2F/fail &>/dev/null

    if [ $? -ne 0 ]; then
      echo -e "${RED}ERROR: diff failed for $GUPPY_OUTPUT_ORIGINAL/fail and $GUPPY_OUTPUT_S2F/fail files ${NC}"
      exit 1
    fi
    echo -e "${GREEN}diff passed${NC}"
fi

if [ $PASS_FAIL_STRUCTURE -eq 0 ]; then
    sed -i 's/sampleid=[^ ]*/sampleid=/g' $GUPPY_OUTPUT_ORIGINAL/*.fast*
    sort -o $GUPPY_OUTPUT_ORIGINAL/*.fast* $GUPPY_OUTPUT_ORIGINAL/*.fast*

    sed -i 's/sampleid=[^ ]*/sampleid=/g' $GUPPY_OUTPUT_S2F/*.fast*
    sort -o $GUPPY_OUTPUT_S2F/*.fast* $GUPPY_OUTPUT_S2F/fail/*.fast*

    echo "diff sorted files"
    diff $GUPPY_OUTPUT_ORIGINAL $GUPPY_OUTPUT_S2F &>/dev/null

    if [ $? -ne 0 ]; then
      echo -e "${RED}ERROR: diff failed for $GUPPY_OUTPUT_ORIGINAL and $GUPPY_OUTPUT_S2F files ${NC}"
      exit 1
    fi
    echo -e "${GREEN}diff passed${NC}"
fi

exit


#diff $GUPPY_OUTPUT_ORIGINAL/sequencing_summary.txt $GUPPY_OUTPUT_S2F/sequencing_summary.txt > diff_sequencing_summary.txt
