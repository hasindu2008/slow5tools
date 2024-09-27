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

set -e
set -x

FAST5_DIR=$1
OUTPUT_DIR=$2/test_with_guppy_test
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

#for the fast5 soup
if [ -d "$FAST5_DIR/single" ]; then
    mkdir -p $F2S_OUTPUT_DIR/single || die "mkdir $F2S_OUTPUT_DIR failed"
    $SLOW5TOOLS f2s -c zlib -s svb-zd $FAST5_DIR/multi -d $F2S_OUTPUT_DIR/multi --iop $IOP || die "slow5tools f2s failed"
    for dir in $FAST5_DIR/single/*
    do
        $SLOW5TOOLS f2s -c zlib -s svb-zd $dir -d $F2S_OUTPUT_DIR/single/$(basename $dir)/ --iop $IOP || die "slow5tools f2s failed"
    done
    $SLOW5TOOLS merge $F2S_OUTPUT_DIR/single/ $F2S_OUTPUT_DIR/multi/{7.blow5,batch*,FAL*,FAN*,PAD*,PAF*,PAG*}  -o $OUTPUT_DIR/merge1.blow5 || die "slow5tools f2s failed"
    $SLOW5TOOLS merge $F2S_OUTPUT_DIR/multi/{FAO*,FAQ*,PAH*} -o $OUTPUT_DIR/merge2.blow5 || die "slow5tools f2s failed"
    $SLOW5TOOLS split -g $OUTPUT_DIR/merge* -d $OUTPUT_DIR/split || die "slow5tools f2s failed"
    CONFIG=dna_r9.4.1_450bps_fast.cfg
#for NA12878
else
    $SLOW5TOOLS f2s -c zlib -s svb-zd $FAST5_DIR -d $F2S_OUTPUT_DIR --iop $IOP || die "slow5tools f2s failed"
    $SLOW5TOOLS merge -c zlib -s none $F2S_OUTPUT_DIR -o $OUTPUT_DIR/merged.blow5 -t $IOP || die "slow5tools merge failed"
    $SLOW5TOOLS view  -c zstd -s svb-zd $OUTPUT_DIR/merged.blow5 -o $OUTPUT_DIR/zstd_svb.blow5 -t $IOP || die "slow5tools view failed"
    $SLOW5TOOLS view  -c none -s none $OUTPUT_DIR/zstd_svb.blow5  -o $OUTPUT_DIR/binary.blow5 -t $IOP || die "slow5tools view failed"
    $SLOW5TOOLS view  -c zlib -s ex-zd $OUTPUT_DIR/binary.blow5  -o $OUTPUT_DIR/zlib_ex-zd.blow5 -t $IOP || die "slow5tools view failed"
    $SLOW5TOOLS split -c zstd -s none $OUTPUT_DIR/zlib_ex-zd.blow5  -d $OUTPUT_DIR/split -r 4000 || die "slow5tools split failed"
    CONFIG=dna_r9.4.1_450bps_fast_prom.cfg
fi

$SLOW5TOOLS s2f $OUTPUT_DIR/split -d $S2F_OUTPUT_DIR --iop $IOP || die "slow5tools s2f failed"

$GUPPY_BASECALLER -c ${CONFIG} -i $FAST5_DIR -s $GUPPY_OUTPUT_ORIGINAL -r --device cuda:all || die "Guppy failed"
$GUPPY_BASECALLER -c ${CONFIG}  -i $S2F_OUTPUT_DIR -s $GUPPY_OUTPUT_S2F -r --device cuda:all || die "Guppy failed"

find $GUPPY_OUTPUT_S2F/ -name '*.fastq' -exec cat {} + | paste - - - -  | sort -k1,1  | tr '\t' '\n' > $OUTPUT_DIR/guppy_output_s2f_sorted.fastq  || die "GUPPY_OUTPUT_S2F/*.fastq cat failed"
if [ -d "$FAST5_DIR/single" ]; then
    find $GUPPY_OUTPUT_ORIGINAL/ -name '*.fastq' -exec cat {} + | paste - - - - | sort -k1,1  | tr '\t' '\n' | sed 's/sampleid=unknown/sampleid=/g'  > $OUTPUT_DIR/guppy_output_original_sorted.fastq || die "GUPPY_OUTPUT_ORIGINAL/*.fastq cat failed"
else
    find $GUPPY_OUTPUT_ORIGINAL/ -name '*.fastq' -exec cat {} + | paste - - - - | sort -k1,1  | tr '\t' '\n' > $OUTPUT_DIR/guppy_output_original_sorted.fastq || die "GUPPY_OUTPUT_ORIGINAL/*.fastq cat failed"
fi
echo "diff sorted fastq files"
diff -q $OUTPUT_DIR/guppy_output_s2f_sorted.fastq $OUTPUT_DIR/guppy_output_original_sorted.fastq  || die "ERROR: diff failed for guppy_output_s2f_sorted.fastq guppy_output_original_sorted.fastq files"
echo -e "${GREEN}diff of fastq passed${NC}"

cut -f2,3,5- $GUPPY_OUTPUT_ORIGINAL/sequencing_summary.txt | sort -k1 > $GUPPY_OUTPUT_ORIGINAL/sorted_sequencing_summary.txt
cut -f2,3,5- $GUPPY_OUTPUT_S2F/sequencing_summary.txt | sort -k1 > $GUPPY_OUTPUT_S2F/sorted_sequencing_summary.txt
diff -q $GUPPY_OUTPUT_ORIGINAL/sorted_sequencing_summary.txt $GUPPY_OUTPUT_S2F/sorted_sequencing_summary.txt > /dev/null || die "diff sorted sequencing summary files failed"
echo -e "${GREEN}diff of sequencing summary files passed${NC}"

rm -r "$OUTPUT_DIR" || die "could not delete $OUTPUT_DIR"

exit
