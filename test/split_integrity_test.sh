#!/bin/bash
# merge four single group slow5s and diff with expected slow5 to check if slow5tools merge is working as expected

# WARNING: this file should be stored inside test directory
# WARNING: the executable should be found at ../
# WARNING: four slow5s should be found at ./data/test/merge/slow5s
# WARNING: expected slow5 should be found at ./data/test/merge/

Usage="split_integrity.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 

if [[ "$#" -ne 0 ]]; then
	echo "Usage: $Usage"
	exit
fi

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

echo "-------------------slow5tools version-------------------"
$REL_PATH/../slow5tools --version

echo
echo "-------------------spliting groups-------------------"

OUTPUT_DIR="$REL_PATH/data/out/split"
test -d  $OUTPUT_DIR
rm -r $OUTPUT_DIR
mkdir $OUTPUT_DIR

if ! $REL_PATH/../slow5tools split -g $REL_PATH/data/exp/split/rg.slow5 -o $OUTPUT_DIR/splitted_groups_slow5s -s; then
    echo "splitting groups failed" 
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
echo "-------------------lossy spliting groups-------------------"

if ! $REL_PATH/../slow5tools split -g -l $REL_PATH/data/exp/split/lossy_rg.slow5 -o $OUTPUT_DIR/lossy_splitted_groups_slow5s -s; then
    echo "splitting groups failed" 
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
echo "-------------------split by reads-------------------"

if ! $REL_PATH/../slow5tools split -r 2 -l $REL_PATH/data/exp/split/11reads.slow5 -o $OUTPUT_DIR/split_reads_slow5s -s; then
    echo "splitting groups failed" 
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
echo "-------------------split by files-------------------"

if ! $REL_PATH/../slow5tools split -f 3 -l $REL_PATH/data/exp/split/11reads.slow5 -o $OUTPUT_DIR/split_files_slow5s -s; then
    echo "splitting groups failed" 
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


exit