#!/bin/bash
# test slow5 index


# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

# terminate script
die() {
    echo -e "${RED}$1${NC}" >&2
    echo
    exit 1
}

OUTPUT_DIR="$REL_PATH/data/out/slow5tools_index"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir $OUTPUT_DIR || die "Creating $OUTPUT_DIR failed"

SLOW5_DIR=$REL_PATH/data/exp/index
SLOW5_EXEC_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $SLOW5_EXEC_WITHOUT_VALGRIND"
else
    SLOW5_EXEC=$SLOW5_EXEC_WITHOUT_VALGRIND
fi

echo "-------------------slow5tools version-------------------"
$SLOW5_EXEC --version || die "slow5tools version failed"

echo
echo "------------------- slow5tools index testcase 1 -------------------"
$SLOW5_EXEC index $SLOW5_DIR/example_multi_rg_v0.1.0.blow5 || die "testcase 1 failed"
diff -s $SLOW5_DIR/example_multi_rg_v0.1.0.blow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.1.0.blow5.idx &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'slow5tools index testcase 1'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 1 passed${NC}"

echo
echo "------------------- slow5tools index testcase 2 -------------------"
$SLOW5_EXEC index $SLOW5_DIR/example_multi_rg_v0.1.0.slow5 || die "testcase 2 failed"
diff -s $SLOW5_DIR/example_multi_rg_v0.1.0.slow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.1.0.slow5.idx &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'slow5tools index testcase 2'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 2 passed${NC}"

echo
echo "------------------- slow5tools index testcase 3 -------------------"
$SLOW5_EXEC index $SLOW5_DIR/example_multi_rg_v0.2.0.blow5 || die "testcase 3 failed"
diff -s $SLOW5_DIR/example_multi_rg_v0.2.0.blow5.idx.exp $SLOW5_DIR/example_multi_rg_v0.2.0.blow5.idx &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'slow5tools index testcase 3'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 3 passed${NC}"

rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
