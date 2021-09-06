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

# terminate script
die() {
    echo -e "${RED}$1${NC}" >&2
    echo
    exit 1
}

SLOW5_DIR=$1
TEMP_DIR="$2/merge_split_integrity_test"
SLOW5_EXEC=$3
MERGE_atm1_OUTPUT="$TEMP_DIR/merge_attempt1"
SPLIT_OUTPUT="$TEMP_DIR/split"
MERGE_atm2_OUTPUT="$TEMP_DIR/merge_attempt2"
MERGE_atm3_OUTPUT="$TEMP_DIR/merge_attempt3"

test -d  "$OUTPUT_DIR" && rm -r "$OUTPUT_DIR"
mkdir "$TEMP_DIR" || die "Creating $TEMP_DIR failed"
mkdir "$MERGE_atm1_OUTPUT" || die "Creating $MERGE_atm1_OUTPUT failed"
mkdir "$SPLIT_OUTPUT" || die "Creating $SPLIT_OUTPUT failed"
mkdir "$MERGE_atm2_OUTPUT" || die "Creating $MERGE_atm2_OUTPUT failed"
mkdir "$MERGE_atm3_OUTPUT" || die "Creating $MERGE_atm3_OUTPUT failed"

echo "-------------------merge attempt 1-------------------"
echo
$SLOW5_EXEC merge "$SLOW5_DIR" -o "$MERGE_atm1_OUTPUT/merged1.slow5" -t 16 --to slow5 || die "merge attempt 1 failed"

echo
echo "-------------------group split attempt-------------------"
echo
$SLOW5_EXEC split "$MERGE_atm1_OUTPUT" -d "$SPLIT_OUTPUT/group_split" --iop 64 -g || die "group split failed"


echo
echo "-------------------file split attempt-------------------"
echo
$SLOW5_EXEC split "$SPLIT_OUTPUT/group_split" -d "$SPLIT_OUTPUT/file_split" --iop 64 -f 2 || die "file split failed"

echo
echo "-------------------read split attempt-------------------"
echo
$SLOW5_EXEC split "$SPLIT_OUTPUT/file_split" -d "$SPLIT_OUTPUT/read_split" --iop 64 -r 3 || die "file split failed"

echo
echo "-------------------merge attempt 2-------------------"
echo
$SLOW5_EXEC merge "$SPLIT_OUTPUT/read_split" -o "$MERGE_atm2_OUTPUT/merged2.slow5" -t 16 --to slow5 || die "merge attempt 2 failed"

sort "$MERGE_atm1_OUTPUT/merged1.slow5" > "$MERGE_atm1_OUTPUT/sorted_merged1.slow5" || die "sort failed"
rm "$MERGE_atm1_OUTPUT/merged1.slow5"
sort "$MERGE_atm2_OUTPUT/merged2.slow5" > "$MERGE_atm2_OUTPUT/sorted_merged2.slow5" || die "sort failed"
# rm $MERGE_atm2_OUTPUT/merged2.slow5

echo "running diff on merge attempt 1 and merge attempt 2"
cmp -s "$MERGE_atm1_OUTPUT/sorted_merged1.slow5" "$MERGE_atm2_OUTPUT/sorted_merged2.slow5"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: merge and split conversions are consistent!${NC}"
    rm -r "$TEMP_DIR"
    exit
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: attempting merge for the third time to make sure run_ids are assigned consistently${NC}"
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
    exit
fi

rm -r "$MERGE_atm1_OUTPUT"
rm -r "$SPLIT_OUTPUT"
mkdir "$SPLIT_OUTPUT" || die "Creating $SPLIT_OUTPUT failed"

echo
echo "-------------------group split attempt 2 -------------------"
echo
$SLOW5_EXEC split "$MERGE_atm2_OUTPUT/merged2.slow5" -d "$SPLIT_OUTPUT/group_split" --iop 64 -g || die "group split failed"

echo
echo "-------------------file split attempt 2-------------------"
echo
$SLOW5_EXEC split "$SPLIT_OUTPUT/group_split" -d "$SPLIT_OUTPUT/file_split" --iop 64 -f 2 || die "file split failed"

echo
echo "-------------------read split attempt 2-------------------"
echo
$SLOW5_EXEC split "$SPLIT_OUTPUT/file_split" -o "$SPLIT_OUTPUT/read_split" --iop 64 -r 3 || die "file split failed"
echo
echo "-------------------merge attempt 3-------------------"
echo
$SLOW5_EXEC merge "$SPLIT_OUTPUT/read_split" -d "$MERGE_atm3_OUTPUT/merged3.slow5" -t 16 --to slow5 || die "merge attempt 2 failed"
# sort $MERGE_atm2_OUTPUT/merged2.slow5 > $MERGE_atm2_OUTPUT/sorted_merged2.slow5
# rm $MERGE_atm2_OUTPUT/merged2.slow5
sort "$MERGE_atm3_OUTPUT/merged3.slow5" > "$MERGE_atm3_OUTPUT/sorted_merged3.slow5" || die "sort failed"
rm "$MERGE_atm3_OUTPUT/merged3.slow5"
rm "$MERGE_atm2_OUTPUT/merged2.slow5"

echo "running diff on merge attempt 2 and merge attempt 3"
cmp -s "$MERGE_atm2_OUTPUT/sorted_merged2.slow5" "$MERGE_atm3_OUTPUT/sorted_merged3.slow5"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: merge and split conversions are consistent!${NC}"
    rm -r "$TEMP_DIR"
elif [ $? -eq 1 ]; then
    echo -e "${RED}FAILURE: merge and split conversions are not consistent${NC}"
else
    echo -e "${RED}ERROR: diff failed for some weird reason${NC}"
fi

rm -r "$OUTPUT_DIR" || die "Removing $OUTPUT_DIR failed"

exit 0