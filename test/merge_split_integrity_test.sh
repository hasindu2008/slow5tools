#!/bin/bash
# Run merge, split, merge, split and again merge and check if the original merged output is same as the last.
# We cannot directly compare after the first two merges because the merged files are not sorted according to the run_ids, hence could be different.
Usage="merge_split_integrity_test.sh [path to slow5 directory] [path to create a temporary directory] [path to slow5tools executable]"

if [[ "$#" -lt 3 ]]; then
	echo "Usage: $Usage"
	exit
fi

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

SLOW5_DIR=$1
TEMP_DIR="$2/merge_split_integrity_test"
SLOW5_EXEC=$3
MERGE_atm1_OUTPUT="$TEMP_DIR/merge_attempt1"
SPLIT_OUTPUT="$TEMP_DIR/split"
MERGE_atm2_OUTPUT="$TEMP_DIR/merge_attempt2"
MERGE_atm3_OUTPUT="$TEMP_DIR/merge_attempt3"

rm -r "$TEMP_DIR"
mkdir "$TEMP_DIR" || exit 1
mkdir "$MERGE_atm1_OUTPUT" || exit 1
mkdir "$SPLIT_OUTPUT" || exit 1
mkdir "$MERGE_atm2_OUTPUT" || exit 1
mkdir "$MERGE_atm3_OUTPUT" || exit 1


echo "-------------------merge attempt 1-------------------"
echo
if ! $SLOW5_EXEC merge $SLOW5_DIR -o $MERGE_atm1_OUTPUT/merged1.slow5 -t 16 -b slow5; then
    echo "merge attempt 1 failed" 
    exit 1
fi


echo
echo "-------------------group split attempt-------------------"
echo
if ! $SLOW5_EXEC split $MERGE_atm1_OUTPUT -o $SPLIT_OUTPUT/group_split --iop 64 -g; then
    echo "group split failed"
    exit 1
fi



echo
echo "-------------------file split attempt-------------------"
echo
if ! $SLOW5_EXEC split $SPLIT_OUTPUT/group_split -o $SPLIT_OUTPUT/file_split --iop 64 -f 2; then
    echo "file split failed"
    exit 1
fi


echo
echo "-------------------read split attempt-------------------"
echo
if ! $SLOW5_EXEC split $SPLIT_OUTPUT/file_split -o $SPLIT_OUTPUT/read_split --iop 64 -r 3; then
    echo "file split failed"
    exit 1
fi


echo
echo "-------------------merge attempt 2-------------------"
echo
if ! $SLOW5_EXEC merge $SPLIT_OUTPUT/read_split -o $MERGE_atm2_OUTPUT/merged2.slow5 -t 16 -b slow5; then
    echo "merge attempt 2 failed"
    exit 1
fi


sort $MERGE_atm1_OUTPUT/merged1.slow5 > $MERGE_atm1_OUTPUT/sorted_merged1.slow5
rm $MERGE_atm1_OUTPUT/merged1.slow5
sort $MERGE_atm2_OUTPUT/merged2.slow5 > $MERGE_atm2_OUTPUT/sorted_merged2.slow5
# rm $MERGE_atm2_OUTPUT/merged2.slow5

echo "running diff on merge attempt 1 and merge attempt 2"
cmp -s $MERGE_atm1_OUTPUT/sorted_merged1.slow5 $MERGE_atm2_OUTPUT/sorted_merged2.slow5

if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: merge and split conversions are consistent!${NC}"
    rm -r $TEMP_DIR
    exit
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: attempting merge for the third time to make sure run_ids are assigned consistently${NC}"
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit
fi


rm -r $MERGE_atm1_OUTPUT
rm -r $SPLIT_OUTPUT
mkdir "$SPLIT_OUTPUT" || exit 1



echo
echo "-------------------group split attempt 2 -------------------"
echo
if ! $SLOW5_EXEC split $MERGE_atm2_OUTPUT/merged2.slow5 -o $SPLIT_OUTPUT/group_split --iop 64 -g; then
    echo "group split failed"
    exit 1
fi


echo
echo "-------------------file split attempt 2-------------------"
echo
if ! $SLOW5_EXEC split $SPLIT_OUTPUT/group_split -o $SPLIT_OUTPUT/file_split --iop 64 -f 2; then
    echo "file split failed"
    exit 1
fi


echo
echo "-------------------read split attempt 2-------------------"
echo
if ! $SLOW5_EXEC split $SPLIT_OUTPUT/file_split -o $SPLIT_OUTPUT/read_split --iop 64 -r 3; then
    echo "file split failed"
    exit 1
fi

echo
echo "-------------------merge attempt 3-------------------"
echo
if ! $SLOW5_EXEC merge $SPLIT_OUTPUT/read_split -o $MERGE_atm3_OUTPUT/merged3.slow5 -t 16 -s; then
    echo "merge attempt 2 failed"
    exit 1
fi

# sort $MERGE_atm2_OUTPUT/merged2.slow5 > $MERGE_atm2_OUTPUT/sorted_merged2.slow5
# rm $MERGE_atm2_OUTPUT/merged2.slow5
sort $MERGE_atm3_OUTPUT/merged3.slow5 > $MERGE_atm3_OUTPUT/sorted_merged3.slow5
rm $MERGE_atm3_OUTPUT/merged3.slow5
rm $MERGE_atm2_OUTPUT/merged2.slow5


echo "running diff on merge attempt 2 and merge attempt 3"
cmp -s $MERGE_atm2_OUTPUT/sorted_merged2.slow5 $MERGE_atm3_OUTPUT/sorted_merged3.slow5

if [ $? -eq 0 ]; then
	echo -e "${GREEN}SUCCESS: merge and split conversions are consistent!${NC}"
    rm -r $TEMP_DIR
elif [ $? -eq 1 ]; then
	echo -e "${RED}FAILURE: merge and split conversions are not consistent${NC}"
else
	echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
fi

exit