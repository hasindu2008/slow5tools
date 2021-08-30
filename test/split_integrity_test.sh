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

echo "-------------------testcase 0: slow5tools version-------------------"
$SLOW5_EXEC --version || die "testcase 0: slow5tools versio"

echo
echo "-------------------testcase 1: spliting groups-------------------"
OUTPUT_DIR="$REL_PATH/data/out/split"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Creating $OUTPUT_DIR failed"

$SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_slow5s --to slow5 || die "testcase 1: splitting groups failed"
echo "comparing group split: output and expected"
diff $REL_PATH/data/exp/split/expected_group_split_slow5s $OUTPUT_DIR/splitted_groups_slow5s &>/dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: splitting groups worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: splitting groups not working properly${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

echo
echo "-------------------testcase 2: lossy spliting groups-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/lossy_splitted_groups_slow5s --to slow5 || die "testcase 2: lossy splitting groups failed"
echo "comparing lossy group split : output and expected"
diff $REL_PATH/data/exp/split/lossy_expected_slow5s $OUTPUT_DIR/lossy_splitted_groups_slow5s &>/dev/null

if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: lossy splitting groups worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: lossy splitting groups not working properly${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

echo
echo "-------------------testcase 3: split by reads-------------------"
$SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_slow5s --to slow5 || die "testcase 3:  split by reads failed"
echo "comparing reads split: output and expected"
diff $REL_PATH/data/exp/split/expected_split_reads_slow5s $OUTPUT_DIR/split_reads_slow5s &>/dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: read splitting worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: read splitting not working properly${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

echo
echo "-------------------testcase 4: split to files-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase 4: split to files failed"
echo "comparing files split: output and expected"
diff $REL_PATH/data/exp/split/expected_split_files_slow5s $OUTPUT_DIR/split_files_slow5s &>/dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: files splitting worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: files splitting not working properly${NC}"
    exit 1
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit 1
fi

echo
rm -r $OUTPUT_DIR/split_files_slow5s
echo "-------------------testcase 5: split by groups input:directory-------------------"
$SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/ -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase 5: split by groups input:directory failed"
echo -e "${GREEN}SUCCESS: testcase 5${NC}"


echo
rm -r $OUTPUT_DIR/split_files_slow5s
echo "-------------------testcase 6: split to files input:directory-------------------"
$SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/ -d $OUTPUT_DIR/split_files_slow5s --to slow5 || die "testcase 6: split to files input:directory failed"
echo -e "${GREEN}SUCCESS: testcase 6${NC}"

echo
echo "-------------------testcase 7: split to files current directory:fast5 file stored directory-------------------"
rm -r $OUTPUT_DIR/split_files_slow5s
cd $REL_PATH/data/raw/split/single_group_slow5s
CD_BACK=../../../../..
if ! $CD_BACK/slow5tools split -f 3 -l false 11reads.slow5 -d $CD_BACK/$OUTPUT_DIR/split_files_slow5s --to slow5; then
    echo -e "testcase 7: split to files current directory:fast5 file stored directory"
    exit 1
fi
echo -e "${GREEN}SUCCESS: testcase 7${NC}"

cd -
rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
