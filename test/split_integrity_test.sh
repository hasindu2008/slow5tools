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

echo "-------------------testcase 0: slow5tools version-------------------"
if ! $SLOW5_EXEC --version; then
    echo -e "${RED}testcase 0: slow5tools version failed"
    exit 1
fi

echo
echo "-------------------testcase 1: spliting groups-------------------"
OUTPUT_DIR="$REL_PATH/data/out/split"
test -d  $OUTPUT_DIR
rm -r $OUTPUT_DIR
mkdir $OUTPUT_DIR

if ! $SLOW5_EXEC split -g $REL_PATH/data/raw/split/multi_group_slow5s/rg.slow5 -d $OUTPUT_DIR/splitted_groups_slow5s --to slow5; then
    echo -e "${RED}testcase 1: splitting groups failed${NC}"
    exit 1
fi
echo "comparing group split: output and expected"
diff $REL_PATH/data/exp/split/expected_slow5s $OUTPUT_DIR/splitted_groups_slow5s &>/dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: splitting groups worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: splitting groups not working properly${NC}"
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
fi

echo
echo "-------------------testcase 2: lossy spliting groups-------------------"
if ! $SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/lossy_rg.slow5 -d $OUTPUT_DIR/lossy_splitted_groups_slow5s --to slow5; then
    echo -e "${RED}testcase 2: lossy splitting groups failed${NC}"
    exit 1
fi
echo "comparing lossy group split : output and expected"
diff $REL_PATH/data/exp/split/lossy_expected_slow5s $OUTPUT_DIR/lossy_splitted_groups_slow5s &>/dev/null

if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: lossy splitting groups worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: lossy splitting groups not working properly${NC}"
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
fi

echo
echo "-------------------testcase 3: split by reads-------------------"
if ! $SLOW5_EXEC split -r 2 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_reads_slow5s --to slow5; then
    echo -e "${RED}testcase 3:  split by reads failed${NC}"
    exit 1
fi
echo "comparing reads split: output and expected"
diff $REL_PATH/data/exp/split/expected_split_reads_slow5s $OUTPUT_DIR/split_reads_slow5s &>/dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: read splitting worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: read splitting not working properly${NC}"
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
fi

echo
echo "-------------------testcase 4: split to files-------------------"
if ! $SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/11reads.slow5 -d $OUTPUT_DIR/split_files_slow5s --to slow5; then
    echo -e "${RED}testcase 4: split to files failed${NC}"
    exit 1
fi
echo "comparing files split: output and expected"
diff $REL_PATH/data/exp/split/expected_split_files_slow5s $OUTPUT_DIR/split_files_slow5s &>/dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: files splitting worked properly!${NC}"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: files splitting not working properly${NC}"
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
fi

echo
rm -r $OUTPUT_DIR/split_files_slow5s
echo "-------------------testcase 5: split by groups input:directory-------------------"
if ! $SLOW5_EXEC split -g -l false $REL_PATH/data/raw/split/multi_group_slow5s/ -d $OUTPUT_DIR/split_files_slow5s --to slow5; then
    echo -e "${RED}testcase 5: split by groups input:directory failed${NC}"
    exit 1
fi
echo -e "${GREEN}SUCCESS: testcase 5${NC}"


echo
rm -r $OUTPUT_DIR/split_files_slow5s
echo "-------------------testcase 6: split to files input:directory-------------------"
if ! $SLOW5_EXEC split -f 3 -l false $REL_PATH/data/raw/split/single_group_slow5s/ -d $OUTPUT_DIR/split_files_slow5s --to slow5; then
    echo -e "${RED}testcase 6: split to files input:directory failed${NC}"
    exit 1
fi
echo -e "${GREEN}SUCCESS: testcase 6${NC}"
exit
