#!/bin/bash
# Run f2s with different file, input and output formats.
Usage="slow5tools_index_test.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

OUTPUT_DIR="$REL_PATH/data/out/f2s_output"
test -d  $OUTPUT_DIR
rm -r $OUTPUT_DIR
mkdir $OUTPUT_DIR

SLOW5_DIR=$REL_PATH/data/raw/index
SLOW5_EXEC_WITHOUT_VALGRIND=$REL_PATH/../slow5tools
if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $SLOW5_EXEC_WITHOUT_VALGRIND"
else
    SLOW5_EXEC=$SLOW5_EXEC_WITHOUT_VALGRIND
fi

echo "-------------------slow5tools version-------------------"
if ! $SLOW5_EXEC --version; then
    echo "slow5tools version failed"
    exit 1
fi
echo
echo "------------------- slow5tools index testcase 1 -------------------"
if ! $SLOW5_EXEC index $SLOW5_DIR/example2.slow5; then
    echo "${RED}testcase 1 failed ${NC}"
    exit 1
fi
diff -s $SLOW5_DIR/expected_example2.slow5.idx $SLOW5_DIR/example2.slow5.idx &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'slow5tools index testcase 1'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 1 passed${NC}"

exit
