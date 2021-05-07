#!/bin/bash

# @author: Hiruna Samarakoon (hirunas@eng.pdn.ac.lk)

###############################################################################

# first do f2s, s2f and then basecall

SLOWTOOLS=/media/shan/OS/slow5-work/slow5/slow5tools
FAST5_DIR=/media/shan/OS/slow5-work/slow5/test/keep_local/three_small_multifast5s
TEST_DIR=/media/shan/OS/slow5-work/slow5/build/test/s2f_guppy_test
F2S_OUTPUT_DIR=$TEST_DIR/f2s
S2F_OUTPUT_DIR=$TEST_DIR/s2f

GUPPY_BASECALLER=
GUPPY_OUTPUT_ORIGINAL=
GUPPY_OUTPUT_S2F=

IOP=4
LOSSY_F2S=-l

$SLOWTOOLS f2s $FAST5_DIR -d $F2S_OUTPUT_DIR --iop $IOP
$SLOWTOOLS s2f $F2S_OUTPUT_DIR -o $S2F_OUTPUT_DIR --iop $IOP

$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $FAST5_DIR -s $GUPPY_OUTPUT_ORIGINAL -r
$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $S2F_OUTPUT_DIR -s $GUPPY_OUTPUT_S2F -r

diff $GUPPY_OUTPUT_ORIGINAL/sequencing_summary.txt $GUPPY_OUTPUT_S2F/sequencing_summary.txt > diff_sequencing_summary.txt
diff $GUPPY_OUTPUT_ORIGINAL/pass $GUPPY_OUTPUT_S2F/pass > diff_pass_fastq.txt
diff $GUPPY_OUTPUT_ORIGINAL/fail $GUPPY_OUTPUT_S2F/fail > diff_fail_fastq.txt