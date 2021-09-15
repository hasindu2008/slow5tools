#!/bin/bash

# @author: Hiruna Samarakoon (hirunas@eng.pdn.ac.lk)

###############################################################################

# first do f2s, s2f and then basecall

Usage="test_s2f_with_guppy.sh [path to fast5 directory] [path to create a temporary directory] [path to slow5tools executable] [path to guppy executable]"

if [[ "$#" -lt 4 ]]; then
	echo "Usage: $Usage"
	exit 1
fi

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color
die() { echo -e "${RED}$1${NC}" >&2 ; echo ; exit 1 ; } # terminate script
#ask before deleting. if yes then delete. if no then exit.
# to automatically answer:$ yes | script or use yes n | script
ask() { echo -n "Directory $1 exists. Delete and create again? (y/n)? " ; read answer ; if [ "$answer" != "${answer#[Nn]}" ] ;then exit ; fi ; echo ; }


FAST5_DIR=$1
OUTPUT_DIR=$2/s2f_with_guppy_test
SLOW5TOOLS=$3
GUPPY_BASECALLER=$4

F2S_OUTPUT_DIR=$OUTPUT_DIR/f2s
S2F_OUTPUT_DIR=$OUTPUT_DIR/s2f
GUPPY_OUTPUT_ORIGINAL=$OUTPUT_DIR/guppy_output_original
GUPPY_OUTPUT_S2F=$OUTPUT_DIR/guppy_output_s2f

# create test directory
test -d "$OUTPUT_DIR" && ask "$OUTPUT_DIR"
test -d  $OUTPUT_DIR && rm -r $OUTPUT_DIR
mkdir $OUTPUT_DIR || die "mkdir $OUTPUT_DIR failed"

IOP=40

$SLOW5TOOLS f2s -c zlib -s svb-zd $FAST5_DIR -d $F2S_OUTPUT_DIR --iop $IOP || die "slow5tools f2s failed"
$SLOW5TOOLS merge -c zlib -s none $F2S_OUTPUT_DIR -o $OUTPUT_DIR/merged.blow5 -t $IOP || die "slow5tools merge failed"
$SLOW5TOOLS view  -c zstd -s svb-zd $OUTPUT_DIR/merged.blow5 -o $OUTPUT_DIR/zstd_svb.blow5 -t $IOP || die "slow5tools view failed"
$SLOW5TOOLS view  -c none -s none $OUTPUT_DIR/zstd_svb.blow5  -o $OUTPUT_DIR/binary.blow5 -t $IOP || die "slow5tools view failed"
$SLOW5TOOLS split -c zstd -s none $OUTPUT_DIR/binary.blow5  -d $OUTPUT_DIR/split -r 4000 || die "slow5tools split failed"
$SLOW5TOOLS s2f $OUTPUT_DIR/split -d $S2F_OUTPUT_DIR --iop $IOP || die "slow5tools s2f failed"

$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $FAST5_DIR -s $GUPPY_OUTPUT_ORIGINAL -r --device cuda:all || die "Guppy failed"
$GUPPY_BASECALLER -c dna_r9.4.1_450bps_hac.cfg -i $S2F_OUTPUT_DIR -s $GUPPY_OUTPUT_S2F -r --device cuda:all || die "Guppy failed"

PASS_FAIL_STRUCTURE=0

if test -d $GUPPY_OUTPUT_S2F/pass; then
    PASS_FAIL_STRUCTURE=1
#    cat $GUPPY_OUTPUT_S2F/pass/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_pass_sorted.fastq || die "Cat failed"
#    cat $GUPPY_OUTPUT_ORIGINAL/pass/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_original_pass_sorted.fastq || die "Cat failed"

    cat $GUPPY_OUTPUT_S2F/pass/*.fastq | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_pass_sorted.fastq || die "GUPPY_OUTPUT_S2F/pass/*fastq Cat failed"
    cat $GUPPY_OUTPUT_ORIGINAL/pass/*.fastq | sort -k1,1  | tr '\t' '\n' > guppy_output_original_pass_sorted.fastq || die "GUPPY_OUTPUT_ORIGINAL/pass/*fastq Cat failed"

    echo "diff sorted pass files"
    diff guppy_output_s2f_pass_sorted.fastq guppy_output_original_pass_sorted.fastq >/dev/null || die "ERROR: diff failed for guppy_output_s2f_pass_sorted.fastq guppy_output_original_pass_sorted.fastq files"
    echo -e "${GREEN}diff passed${NC}"
fi

if test -d $GUPPY_OUTPUT_S2F/fail; then
    PASS_FAIL_STRUCTURE=1
#    cat $GUPPY_OUTPUT_S2F/fail/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_fail_sorted.fastq
#    cat $GUPPY_OUTPUT_ORIGINAL/fail/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_original_fail_sorted.fastq

    cat $GUPPY_OUTPUT_S2F/fail/*.fastq | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_fail_sorted.fastq || die "GUPPY_OUTPUT_S2F/fail/*fastq cat failed"
    cat $GUPPY_OUTPUT_ORIGINAL/fail/*.fastq | sort -k1,1  | tr '\t' '\n' > guppy_output_original_fail_sorted.fastq || die "GUPPY_OUTPUT_ORIGINAL/fail/*fastq cat failed"

    echo "diff sorted fail files"
    diff guppy_output_s2f_fail_sorted.fastq guppy_output_original_fail_sorted.fastq >/dev/null || die "ERROR: diff failed for guppy_output_s2f_fail_sorted.fastq and guppy_output_original_fail_sorted.fastq files"
    echo -e "${GREEN}diff passed${NC}"
fi

if [ $PASS_FAIL_STRUCTURE -eq 0 ]; then
#    cat $GUPPY_OUTPUT_S2F/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_sorted.fastq
#    cat $GUPPY_OUTPUT_ORIGINAL/*.fastq | awk '{if(NR%4==1){print $1} else{print $0};}'  | paste - - - -  | sort -k1,1  | tr '\t' '\n' > guppy_output_original_sorted.fastq

    cat $GUPPY_OUTPUT_S2F/*.fastq | sort -k1,1  | tr '\t' '\n' > guppy_output_s2f_sorted.fastq || die "GUPPY_OUTPUT_S2F/*.fastq cat failed"
    cat $GUPPY_OUTPUT_ORIGINAL/*.fastq | sort -k1,1  | tr '\t' '\n' > guppy_output_original_sorted.fastq || die "GUPPY_OUTPUT_ORIGINAL/*.fastq cat failed"

    echo "diff sorted files"
    diff guppy_output_s2f_sorted.fastq guppy_output_original_sorted.fastq >/dev/null || die "ERROR: diff failed for guppy_output_s2f_sorted.fastq guppy_output_original_sorted.fastq files"
    echo -e "${GREEN}diff passed${NC}"
fi

sort $GUPPY_OUTPUT_ORIGINAL/sequencing_summary.txt > $GUPPY_OUTPUT_ORIGINAL/sorted_sequencing_summary.txt
sort $GUPPY_OUTPUT_S2F/sequencing_summary.txt > $GUPPY_OUTPUT_S2F/sorted_sequencing_summary.txt
diff $GUPPY_OUTPUT_ORIGINAL/sorted_sequencing_summary.txt $GUPPY_OUTPUT_S2F/sorted_sequencing_summary.txt > /dev/null || die "diff sorted sequencing summary files failed"

rm -r "$OUTPUT_DIR" || die "could not delete $OUTPUT_DIR"

exit
