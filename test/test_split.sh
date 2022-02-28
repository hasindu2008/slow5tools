#!/bin/bash
# WARNING: this file should be stored inside test directory
# WARNING: the executable should be found at ../
# WARNING: expected slow5s/directories should be found at ./data/exp/split

Usage="split_integrity.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $REL_PATH/../slow5tools"
else
    SLOW5_EXEC=$REL_PATH/../slow5tools
fi

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

# terminate script
die() {
    echo -e "${RED}$1${NC}" >&2
    echo
    exit 1
}

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
    echo -e "${GREEN}running slow5tools_quickcheck for files in $PWD/${1}${NC}" 1>&3 2>&4
    ls -1 $PWD/${1}/**.[bs]low5 | xargs -n1 $SLOW5_EXEC quickcheck &> /dev/null
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}SUCCESS: slow5tools_quickcheck passed!${NC}" 1>&3 2>&4
    elif [ $? -eq 1 ]; then
        echo -e "${RED}FAILURE: ${1} files are corrupted${NC}" 1>&3 2>&4
        exit 1
    else
        echo -e "${RED}ERROR: slow5tools_quickcheck failed for some weird reason${NC}" 1>&3 2>&4
        exit 1
    fi
}

check() {
    echo -e "${GREEN}${1}${NC}" 1>&3 2>&4
    slow5tools_quickcheck ${2} ${3}
    diff ${2} ${3} &>/dev/null
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}SUCCESS: ${1} worked properly!${NC}"  1>&3 2>&4
    elif [ $? -eq 1 ]; then
        echo -e "${RED}FAILURE: ${1} not working properly${NC}" 1>&3 2>&4
        exit 1
    else
        echo -e "${RED}ERROR: diff failed for some weird reason${NC}" 1>&3 2>&4
        exit 1
    fi
}

TESTCASE=0
echo "-------------------testcase ${TESTCASE}: slow5tools version-------------------"
$SLOW5_EXEC --version || die "testcase ${TESTCASE}: slow5tools versio"

OUTPUT_DIR="$REL_PATH/data/out/split"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Creating $OUTPUT_DIR failed"

echo
TESTCASE=1
echo "-------------------testcase ${TESTCASE}: spliting groups-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE group split" $REL_PATH/data/exp/split/expected_group_split_slow5s $OUTPUT_DIR/splitted_groups_slow5s

TESTCASE=2
echo
echo "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/lossy_splitted_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
echo "comparing lossy group split : output and expected"
check "Testcase:$TESTCASE lossy splitting groups" $REL_PATH/data/exp/split/lossy_expected_slow5s $OUTPUT_DIR/lossy_splitted_groups_slow5s

TESTCASE=3
echo
echo "-------------------testcase ${TESTCASE}: split a multi read group slow5 file and produce blow5 output-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_blow5_output -c zlib -s svb-zd || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_blow5_output $OUTPUT_DIR/splitted_groups_blow5_output


TESTCASE=4
echo
echo "-------------------testcase ${TESTCASE}: split a multi read group v0.1.0 zlib blow5 file -------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_blow5s/example_multi_rg_v0.1.0.blow5 -d $OUTPUT_DIR/splitted_groups_blow5_input --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_blow5_input $OUTPUT_DIR/splitted_groups_blow5_input


TESTCASE=5
echo
echo "-------------------testcase ${TESTCASE}: split a multi read group with and without enum type single file-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_enum/with_and_without_enum.slow5 -d $OUTPUT_DIR/splitted_groups_enum --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
echo "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_enum $OUTPUT_DIR/splitted_groups_enum


TESTCASE=6
echo
echo "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_slow5s --to slow5 || die "testcase ${TESTCASE}:  split by reads failed"
echo "comparing reads split: output and expected"
check "Testcase:$TESTCASE read splitting" $REL_PATH/data/exp/split/expected_split_reads_slow5s $OUTPUT_DIR/split_reads_slow5s


TESTCASE=7
echo
echo "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase ${TESTCASE}: split to files failed"
echo "comparing files split: output and expected"
check "Testcase:$TESTCASE files splitting" $REL_PATH/data/exp/split/expected_split_files_slow5s $OUTPUT_DIR/split_files_slow5s

TESTCASE=8
echo
rm -r $OUTPUT_DIR/split_files_slow5s
echo "-------------------testcase ${TESTCASE}: split by groups input:directory-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/ -d $OUTPUT_DIR/split_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: split by groups input:directory failed"
check "Testcase:$TESTCASE group splitting" $REL_PATH/data/exp/split/expected_split_groups_slow5s $OUTPUT_DIR/split_groups_slow5s

TESTCASE=9
echo
rm -r $OUTPUT_DIR/split_files_slow5s
echo "-------------------testcase ${TESTCASE}: split to files input:directory-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/ -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase ${TESTCASE}: split to files input:directory failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_slow5s
echo -e "${GREEN}SUCCESS: testcase ${TESTCASE}${NC}"

TESTCASE=10
echo
echo "-------------------testcase ${TESTCASE}: split to files current directory:slow5 file stored directory-------------------"
rm -r $OUTPUT_DIR/split_files_slow5s
pwd
cd $REL_PATH/data/raw/split/single_group_slow5s
CD_BACK=../../../../..
if ! $CD_BACK/slow5tools split -f 3 -l false 11reads.slow5 -d $CD_BACK/$OUTPUT_DIR/split_files_slow5s --to slow5; then
    echo -e "testcase ${TESTCASE}: split to files current directory:fast5 file stored directory"
    exit 1
fi
cd -
slow5tools_quickcheck $OUTPUT_DIR/split_files_slow5s
echo -e "${GREEN}SUCCESS: testcase ${TESTCASE}${NC}"

TESTCASE=11
echo
echo "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}:  split by reads failed"
slow5tools_quickcheck $OUTPUT_DIR/split_reads_blow5s_lossy

TESTCASE=12
echo
echo "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l true $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}:  split by reads failed"
slow5tools_quickcheck $OUTPUT_DIR/split_reads_blow5s_lossless

TESTCASE=13
echo
echo "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}: split to files failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_blow5s_lossy

TESTCASE=14
echo
echo "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l true $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}: split to files failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_blow5s_lossless

TESTCASE=15
echo
echo "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/split_groups_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
slow5tools_quickcheck $OUTPUT_DIR/split_groups_blow5s_lossy

TESTCASE=16
echo
echo "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l true $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/split_groups_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
slow5tools_quickcheck $OUTPUT_DIR/split_groups_blow5s_lossless

rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
