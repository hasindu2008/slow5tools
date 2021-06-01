#!/bin/bash

# @author: Hiruna Samarakoon (hirunas@eng.pdn.ac.lk)

###############################################################################

# first do f2s, s2f and then basecall

Usage="test_s2f_with_guppy.sh [path to fast5 directory] [path to create a temporary directory] [path to slow5tools executable] [path to guppy executable]"

if [[ "$#" -lt 4 ]]; then
	echo "Usage: $Usage"
	exit
fi

FAST5_DIR=$1
TEST_DIR=$2
SLOWTOOLS=$3
GUPPY_BASECALLER=$4

F2S_OUTPUT_DIR=$TEST_DIR/f2s
S2F_OUTPUT_DIR=$TEST_DIR/s2f
GUPPY_OUTPUT_ORIGINAL=$TEST_DIR/guppy_output_original
GUPPY_OUTPUT_S2F=$TEST_DIR/guppy_output_s2f

IOP=4
LOSSY_F2S=-l

$SLOWTOOLS f2s $FAST5_DIR -d $F2S_OUTPUT_DIR --iop $IOP
$SLOWTOOLS s2f $F2S_OUTPUT_DIR -d $S2F_OUTPUT_DIR --iop $IOP

$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $FAST5_DIR -s $GUPPY_OUTPUT_ORIGINAL -r
$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $S2F_OUTPUT_DIR -s $GUPPY_OUTPUT_S2F -r

diff $GUPPY_OUTPUT_ORIGINAL/sequencing_summary.txt $GUPPY_OUTPUT_S2F/sequencing_summary.txt > diff_sequencing_summary.txt
diff $GUPPY_OUTPUT_ORIGINAL/pass $GUPPY_OUTPUT_S2F/pass > diff_pass_fastq.txt
diff $GUPPY_OUTPUT_ORIGINAL/fail $GUPPY_OUTPUT_S2F/fail > diff_fail_fastq.txt