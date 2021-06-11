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

IOP=40

$SLOWTOOLS f2s $FAST5_DIR -d $F2S_OUTPUT_DIR --iop $IOP
$SLOWTOOLS s2f $F2S_OUTPUT_DIR -d $S2F_OUTPUT_DIR --iop $IOP

$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $FAST5_DIR -s $GUPPY_OUTPUT_ORIGINAL -r --device cuda:all
$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $S2F_OUTPUT_DIR -s $GUPPY_OUTPUT_S2F -r --device cuda:all

PASS_FAIL_STRUCTURE=0

if test -d $GUPPY_OUTPUT_S2F/pass; then
    PASS_FAIL_STRUCTURE=1

    cat $GUPPY_OUTPUT_S2F/pass/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_pass_sorted.fastq
    cat $GUPPY_OUTPUT_ORIGINAL/pass/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_original_pass_sorted.fastq

    echo "diff sorted pass files"
    diff guppy_output_s2f_pass_sorted.fastq guppy_output_original_pass_sorted.fastq &>/dev/null
    

    if [ $? -ne 0 ]; then
      echo -e "${RED}ERROR: diff failed for guppy_output_s2f_pass_sorted.fastq guppy_output_original_pass_sorted.fastq files ${NC}"
      exit 1
    fi
    echo -e "${GREEN}diff passed${NC}"
fi

if test -d $GUPPY_OUTPUT_S2F/fail; then
    PASS_FAIL_STRUCTURE=1
    cat $GUPPY_OUTPUT_S2F/fail/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_fail_sorted.fastq
    cat $GUPPY_OUTPUT_ORIGINAL/fail/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_original_fail_sorted.fastq

    echo "diff sorted fail files"
    diff guppy_output_s2f_fail_sorted.fastq guppy_output_original_fail_sorted.fastq &>/dev/null

    if [ $? -ne 0 ]; then
      echo -e "${RED}ERROR: diff failed for guppy_output_s2f_fail_sorted.fastq and guppy_output_original_fail_sorted.fastq files ${NC}"
      exit 1
    fi
    echo -e "${GREEN}diff passed${NC}"
fi

if [ $PASS_FAIL_STRUCTURE -eq 0 ]; then
    cat $GUPPY_OUTPUT_S2F/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_sorted.fastq
    cat $GUPPY_OUTPUT_ORIGINAL/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_original_sorted.fastq

    echo "diff sorted files"
    diff guppy_output_s2f_sorted.fastq guppy_output_original_sorted.fastq &>/dev/null

    if [ $? -ne 0 ]; then
      echo -e "${RED}ERROR: diff failed for guppy_output_s2f_sorted.fastq guppy_output_original_sorted.fastq files ${NC}"
      exit 1
    fi
    echo -e "${GREEN}diff passed${NC}"
fi

exit


#diff $GUPPY_OUTPUT_ORIGINAL/sequencing_summary.txt $GUPPY_OUTPUT_S2F/sequencing_summary.txt > diff_sequencing_summary.txt
