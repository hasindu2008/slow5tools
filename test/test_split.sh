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

# WARNING: this file should be stored inside test directory
# WARNING: the executable should be found at ../
# WARNING: expected slow5s/directories should be found at ./data/exp/split

Usage="split_integrity.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

SLOW5_EXEC_WITHOUT_VALGRIND=$REL_PATH/../slow5tools

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $REL_PATH/../slow5tools"
else
    SLOW5_EXEC=$REL_PATH/../slow5tools
fi

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


slow5tools_quickcheck() {
    echo "running slow5tools_quickcheck for files in $PWD/${1}"
    ls -1 $PWD/${1}/**.[bs]low5 | xargs -n1 $SLOW5_EXEC quickcheck
    if [ $? -eq 0 ]; then
        echo "SUCCESS: slow5tools_quickcheck passed!"
    elif [ $? -eq 1 ]; then
        die "FAILURE: ${1} files are corrupted"
        exit 1
    else
        die "ERROR: slow5tools_quickcheck failed for some weird reason"
        exit 1
    fi
}

check() {
    echo "${1}"
    slow5tools_quickcheck ${2} ${3} || die "slow5tools_quickcheck failed for ${1}"
    diff ${2} ${3} &>/dev/null
    if [ $? -eq 0 ]; then
        echo "${GREEN}SUCCESS: ${1} worked properly!"
    elif [ $? -eq 1 ]; then
        die "${RED}FAILURE: ${1} not working properly"
        exit 1
    else
        die "${RED}ERROR: diff failed for some weird reason"
        exit 1
    fi
}
OUTPUT_DIR="$REL_PATH/data/out/split"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Creating $OUTPUT_DIR failed"

TESTCASE=0
info "-------------------testcase ${TESTCASE}: slow5tools version-------------------"
$SLOW5_EXEC --version || die "testcase ${TESTCASE}: slow5tools versio"


TESTCASE=1
info "-------------------testcase ${TESTCASE}: spliting groups-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE group split" $REL_PATH/data/exp/split/expected_group_split_slow5s $OUTPUT_DIR/splitted_groups_slow5s

TESTCASE=2
info "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/lossy_splitted_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
echo "comparing lossy group split : output and expected"
check "Testcase:$TESTCASE lossy splitting groups" $REL_PATH/data/exp/split/lossy_expected_slow5s $OUTPUT_DIR/lossy_splitted_groups_slow5s

if [ -z "$bigend" ]; then
TESTCASE=3
info "-------------------testcase ${TESTCASE}: split a multi read group slow5 file and produce blow5 output-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_blow5_output -c zlib -s svb-zd || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_blow5_output $OUTPUT_DIR/splitted_groups_blow5_output
fi

TESTCASE=4
info "-------------------testcase ${TESTCASE}: split a multi read group v0.1.0 zlib blow5 file -------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_blow5s/example_multi_rg_v0.1.0.blow5 -d $OUTPUT_DIR/splitted_groups_blow5_input --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_blow5_input $OUTPUT_DIR/splitted_groups_blow5_input


TESTCASE=5
info "-------------------testcase ${TESTCASE}: split a multi read group with and without enum type single file-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_enum/with_and_without_enum.slow5 -d $OUTPUT_DIR/splitted_groups_enum --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_enum $OUTPUT_DIR/splitted_groups_enum


TESTCASE=6
info "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_slow5s --to slow5 || die "testcase ${TESTCASE}:  split by reads failed"
echo "comparing reads split: output and expected"
check "Testcase:$TESTCASE read splitting" $REL_PATH/data/exp/split/expected_split_reads_slow5s $OUTPUT_DIR/split_reads_slow5s


TESTCASE=7
info "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase ${TESTCASE}: split to files failed"
echo "comparing files split: output and expected"
check "Testcase:$TESTCASE files splitting" $REL_PATH/data/exp/split/expected_split_files_slow5s $OUTPUT_DIR/split_files_slow5s

TESTCASE=8
rm -r $OUTPUT_DIR/split_files_slow5s
info "-------------------testcase ${TESTCASE}: split by groups input:directory-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/ -d $OUTPUT_DIR/split_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: split by groups input:directory failed"
check "Testcase:$TESTCASE group splitting" $REL_PATH/data/exp/split/expected_split_groups_slow5s $OUTPUT_DIR/split_groups_slow5s

TESTCASE=9
rm -r $OUTPUT_DIR/split_files_slow5s
info "-------------------testcase ${TESTCASE}: split to files input:directory-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/ -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase ${TESTCASE}: split to files input:directory failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_slow5s || die "testcase ${TESTCASE}: split to files input:directory failed"

TESTCASE=10
info "-------------------testcase ${TESTCASE}: split to files current directory:slow5 file stored directory-------------------"
rm -r $OUTPUT_DIR/split_files_slow5s
pwd
cd $REL_PATH/data/raw/split/single_group_slow5s
CD_BACK=../../../../..
if ! $CD_BACK/slow5tools split -f 3 -l false 11reads.slow5 -d $CD_BACK/$OUTPUT_DIR/split_files_slow5s --to slow5; then
    die "testcase ${TESTCASE}: split to files current directory:slow5 file stored directory failed"
fi
cd -
slow5tools_quickcheck $OUTPUT_DIR/split_files_slow5s || die "testcase ${TESTCASE}: split to files current directory:slow5 file stored directory failed"

if [ -z "$bigend" ]; then
TESTCASE=11
info "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}:  split by reads failed"
slow5tools_quickcheck $OUTPUT_DIR/split_reads_blow5s_lossy || die "testcase ${TESTCASE}:  split by reads failed"

TESTCASE=12
info "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l true $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}:  split by reads failed"
slow5tools_quickcheck $OUTPUT_DIR/split_reads_blow5s_lossless || die "testcase ${TESTCASE}:  split by reads failed"

TESTCASE=13
info "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}: split to files failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_blow5s_lossy || die "testcase ${TESTCASE}: split to files failed"

TESTCASE=14
info "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l true $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}: split to files failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_blow5s_lossless || die "testcase ${TESTCASE}: split to files failed"

TESTCASE=15
info "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/split_groups_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
slow5tools_quickcheck $OUTPUT_DIR/split_groups_blow5s_lossy || die "testcase ${TESTCASE}: lossy splitting groups failed"

TESTCASE=16
info "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l true $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/split_groups_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
slow5tools_quickcheck $OUTPUT_DIR/split_groups_blow5s_lossless || die "testcase ${TESTCASE}: lossy splitting groups failed"
fi

TESTCASE=17
name="testcase ${TESTCASE}: demultiplex one read, one barcode"
info "-------------------$name-------"
$SLOW5_EXEC split -x $REL_PATH/data/raw/split/demux1/barcode_summary.txt $REL_PATH/data/raw/split/demux1/example2_0.slow5 -d $OUTPUT_DIR/demux1 --to slow5 || die "$name"
check "$name" $REL_PATH/data/exp/split/demux1 $OUTPUT_DIR/demux1

if [ -z "$bigend" ]; then
    TESTCASE=18
    name="testcase ${TESTCASE}: demultiplex two reads, one barcode"
    info "-------------------$name-------"
    $SLOW5_EXEC split -x $REL_PATH/data/raw/split/demux2/barcode_summary.txt $REL_PATH/data/raw/split/demux2/example2_0.slow5 -d $OUTPUT_DIR/demux2 --to blow5 || die "$name"
    check "$name" $REL_PATH/data/exp/split/demux2 $OUTPUT_DIR/demux2

    TESTCASE=19
    name="testcase ${TESTCASE}: demultiplex one missing"
    info "-------------------$name-------"
    $SLOW5_EXEC split --to blow5 $REL_PATH/data/raw/split/demux3/example2_0.slow5 -d $OUTPUT_DIR/demux3 -x $REL_PATH/data/raw/split/demux3/bs.txt || die "$name"
    check "$name" $REL_PATH/data/exp/split/demux3 $OUTPUT_DIR/demux3

    TESTCASE=20
    name="testcase ${TESTCASE}: demultiplex two reads, two barcodes"
    info "-------------------$name-------"
    $SLOW5_EXEC split --to slow5 $REL_PATH/data/raw/split/demux4/example2_0.blow5 -d $OUTPUT_DIR/demux4 -x $REL_PATH/data/raw/split/demux4/summary || die "$name"
    check "$name" $REL_PATH/data/exp/split/demux4 $OUTPUT_DIR/demux4

    TESTCASE=21
    name="testcase ${TESTCASE}: demultiplex custom header"
    info "-------------------$name-------"
    $SLOW5_EXEC split --to slow5 $REL_PATH/data/raw/split/demux5/example2_0.blow5 -d $OUTPUT_DIR/demux5 --demux $REL_PATH/data/raw/split/demux5/custom --demux-rid-hdr=MyCustomId --demux-code-hdr 'BC0D35!' || die "$name"
    check "$name" $REL_PATH/data/exp/split/demux5 $OUTPUT_DIR/demux5

    TESTCASE=22
    name="testcase ${TESTCASE}: demultiplex one extra"
    info "-------------------$name-------"
    $SLOW5_EXEC split --to slow5 $REL_PATH/data/raw/split/demux6/example2_0.blow5 -d $OUTPUT_DIR/demux6 --demux $REL_PATH/data/raw/split/demux6/barcode_summary.txt && die "$name"

    TESTCASE=23
    name="testcase ${TESTCASE}: demultiplex duplicated read"
    info "-------------------$name-------"
    $SLOW5_EXEC split --to slow5 $REL_PATH/data/raw/split/demux7/example2_0.blow5 -d $OUTPUT_DIR/demux7 --demux $REL_PATH/data/raw/split/demux7/barcode_summary.txt || die "$name"
    check "$name" $REL_PATH/data/exp/split/demux7 $OUTPUT_DIR/demux7
fi

rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
