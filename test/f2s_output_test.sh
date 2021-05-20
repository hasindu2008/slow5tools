#!/bin/bash
# Run f2s, s2f, and again f2s and check if first produced slow5s are same as the last set.
Usage="f2s_output_test.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 

if [[ "$#" -ne 0 ]]; then
    echo "Usage: $Usage"
    exit
fi

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

OUTPUT_DIR="$REL_PATH/data/out/f2s_output"
test -d  $OUTPUT_DIR
rm -r $OUTPUT_DIR
mkdir $OUTPUT_DIR

SLOW5_EXEC=$REL_PATH/../slow5tools
FAST5_DIR=$REL_PATH/data/raw/f2s_output
EXP_SLOW5_DIR=$REL_PATH/data/exp/f2s_output

echo "-------------------slow5tools version-------------------"
if ! $SLOW5_EXEC --version; then
    echo "slow5tools version failed" 
    exit 1
fi


echo
echo "------------------- testcase 1 >>> f2s format:single-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 --iop 1 -s > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single-fast5 input:file process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 2 >>> f2s format:single-fast5 input:file process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 -s; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single-fast5 input:file process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 3 >>> f2s format:single-fast5 input:directory process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 --iop 1 -s > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'f2s format:single-fast5 input:directory process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 4 >>> f2s format:single-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 -s; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'f2s format:single-fast5 input:directory process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 5 >>> f2s format:multi-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 -s>$OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 6 >>> f2s format:multi-fast5 input:file process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 -s -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 7 >>> f2s format:multi-fast5 input:directory process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 -s>$OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/directory_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:directory process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 8 >>> f2s format:multi-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 -s -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:single_process output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/ssm2.slow5 $OUTPUT_DIR/multi-fast5/ssm2.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:single_process output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/ssm3.slow5 $OUTPUT_DIR/multi-fast5/ssm3.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 9 >>> f2s format:single_and_multi-fast5 input:directory process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 1 -s>$OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/file_single-and-multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 10 >>> f2s format:single_and_multi-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 1 -s -d $OUTPUT_DIR/single-and-multi-fast5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/0.slow5 $OUTPUT_DIR/single-and-multi-fast5/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:single_process output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/1ssm1.slow5 $OUTPUT_DIR/single-and-multi-fast5/1ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:single_process output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/3ssm2.slow5 $OUTPUT_DIR/single-and-multi-fast5/3ssm2.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

# ----------------------------------------------- multi process --------------------------------------------

echo
echo "------------------- testcase 11 >>> f2s format:single-fast5 input:file process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 -s; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single-fast5 input:file process:multi output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 12 >>> f2s format:single-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 -s; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'f2s format:single-fast5 input:directory process:multi output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'f2s format:single-fast5 input:directory process:multi output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/2.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'f2s format:single-fast5 input:directory process:multi output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 13 >>> f2s format:multi-fast5 input:file process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 4 -s -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:multi output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 14 >>> f2s format:multi-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 4 -s -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:multi output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/ssm2.slow5 $OUTPUT_DIR/multi-fast5/ssm2.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:multi output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/ssm3.slow5 $OUTPUT_DIR/multi-fast5/ssm3.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:multi output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 15 >>> f2s format:single_and_multi-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 4 -s -d $OUTPUT_DIR/single-and-multi-fast5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/1ssm1.slow5 $OUTPUT_DIR/single-and-multi-fast5/1ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:multi output:directory'${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/3ssm2.slow5 $OUTPUT_DIR/single-and-multi-fast5/3ssm2.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:multi output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

----------------------------------------- run id conflicts -------------------------------------------

echo
echo "------------------- testcase 16 >>> f2s format:single-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 --iop 1 -s > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 17 >>> f2s format:single-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 -s; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 18 >>> f2s format:multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 -s > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- testcase 19 >>> f2s format:single_and_multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 -s > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"


echo
echo "------------------- testcase 20 >>> f2s format:single_and_multi-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 -d $OUTPUT_DIR; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

exit
