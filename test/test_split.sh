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
    info "running slow5tools_quickcheck for files in $PWD/${1}"
    ls -1 $PWD/${1}/**.[bs]low5 | xargs -n1 $SLOW5_EXEC quickcheck &> /dev/null
    if [ $? -eq 0 ]; then
        info "SUCCESS: slow5tools_quickcheck passed!"
    elif [ $? -eq 1 ]; then
        info "FAILURE: ${1} files are corrupted"
        exit 1
    else
        info "ERROR: slow5tools_quickcheck failed for some weird reason"
        exit 1
    fi
}

check() {
    info "${1}"
    slow5tools_quickcheck ${2} ${3}
    diff ${2} ${3} &>/dev/null
    if [ $? -eq 0 ]; then
        info "${GREEN}SUCCESS: ${1} worked properly!"
    elif [ $? -eq 1 ]; then
        info "${RED}FAILURE: ${1} not working properly"
        exit 1
    else
        info "${RED}ERROR: diff failed for some weird reason"
        exit 1
    fi
}

TESTCASE=0
info "-------------------testcase ${TESTCASE}: slow5tools version-------------------"
$SLOW5_EXEC --version || die "testcase ${TESTCASE}: slow5tools versio"

OUTPUT_DIR="$REL_PATH/data/out/split"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Creating $OUTPUT_DIR failed"

TESTCASE=1
info "-------------------testcase ${TESTCASE}: spliting groups-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
info "comparing group split: output and expected"
check "Testcase:$TESTCASE group split" $REL_PATH/data/exp/split/expected_group_split_slow5s $OUTPUT_DIR/splitted_groups_slow5s

TESTCASE=2
info "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/lossy_splitted_groups_slow5s --to slow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
info "comparing lossy group split : output and expected"
check "Testcase:$TESTCASE lossy splitting groups" $REL_PATH/data/exp/split/lossy_expected_slow5s $OUTPUT_DIR/lossy_splitted_groups_slow5s

TESTCASE=3
info "-------------------testcase ${TESTCASE}: split a multi read group slow5 file and produce blow5 output-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_blow5_output -c zlib -s svb-zd || die "testcase ${TESTCASE}: splitting groups failed"
info "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_blow5_output $OUTPUT_DIR/splitted_groups_blow5_output


TESTCASE=4
info "-------------------testcase ${TESTCASE}: split a multi read group v0.1.0 zlib blow5 file -------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_blow5s/example_multi_rg_v0.1.0.blow5 -d $OUTPUT_DIR/splitted_groups_blow5_input --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
info "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_blow5_input $OUTPUT_DIR/splitted_groups_blow5_input


TESTCASE=5
info "-------------------testcase ${TESTCASE}: split a multi read group with and without enum type single file-------------------"
$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_enum/with_and_without_enum.slow5 -d $OUTPUT_DIR/splitted_groups_enum --to slow5 || die "testcase ${TESTCASE}: splitting groups failed"
info "comparing group split: output and expected"
check "Testcase:$TESTCASE splitting groups" $REL_PATH/data/exp/split/expected_group_split_enum $OUTPUT_DIR/splitted_groups_enum


TESTCASE=6
info "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_slow5s --to slow5 || die "testcase ${TESTCASE}:  split by reads failed"
info "comparing reads split: output and expected"
check "Testcase:$TESTCASE read splitting" $REL_PATH/data/exp/split/expected_split_reads_slow5s $OUTPUT_DIR/split_reads_slow5s


TESTCASE=7
info "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase ${TESTCASE}: split to files failed"
info "comparing files split: output and expected"
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
slow5tools_quickcheck $OUTPUT_DIR/split_files_slow5s

TESTCASE=10
info "-------------------testcase ${TESTCASE}: split to files current directory:slow5 file stored directory-------------------"
rm -r $OUTPUT_DIR/split_files_slow5s
pwd
cd $REL_PATH/data/raw/split/single_group_slow5s
CD_BACK=../../../../..
if ! $CD_BACK/slow5tools split -f 3 -l false 11reads.slow5 -d $CD_BACK/$OUTPUT_DIR/split_files_slow5s --to slow5; then
    exit 1
fi
cd -
slow5tools_quickcheck $OUTPUT_DIR/split_files_slow5s

TESTCASE=11
info "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}:  split by reads failed"
slow5tools_quickcheck $OUTPUT_DIR/split_reads_blow5s_lossy

TESTCASE=12
info "-------------------testcase ${TESTCASE}: split by reads single file -------------------"
$SLOW5_EXEC split -r 2 -l true $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}:  split by reads failed"
slow5tools_quickcheck $OUTPUT_DIR/split_reads_blow5s_lossless

TESTCASE=13
info "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}: split to files failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_blow5s_lossy

TESTCASE=14
info "-------------------testcase ${TESTCASE}: split to files single file-------------------"
$SLOW5_EXEC split -f 3 -l true $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}: split to files failed"
slow5tools_quickcheck $OUTPUT_DIR/split_files_blow5s_lossless

TESTCASE=15
info "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/split_groups_blow5s_lossy --to blow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
slow5tools_quickcheck $OUTPUT_DIR/split_groups_blow5s_lossy

TESTCASE=16
info "-------------------testcase ${TESTCASE}: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l true $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/split_groups_blow5s_lossless --to blow5 || die "testcase ${TESTCASE}: lossy splitting groups failed"
slow5tools_quickcheck $OUTPUT_DIR/split_groups_blow5s_lossless

rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
