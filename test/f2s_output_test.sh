#!/bin/bash
# Run f2s with different file, input and output formats.
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
echo "------------------- f2s testcase 1 >>> format:single-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
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
echo "------------------- f2s testcase 2 >>> format:single-fast5 input:file process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5; then
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
echo "------------------- f2s testcase 3 >>> format:single-fast5 input:directory process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
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
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 4 >>> format:single-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5; then
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
echo "------------------- f2s testcase 5 >>> format:multi-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5; then
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
echo "------------------- f2s testcase 6 >>> format:multi-fast5 input:file process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
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
echo "------------------- f2s testcase 7 >>> format:multi-fast5 input:directory process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5; then
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
rm $OUTPUT_DIR/multi-fast5/*
echo "------------------- f2s testcase 8 >>> format:multi-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
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
rm $OUTPUT_DIR/stdout.slow5
#input:directory on a single process cannot be tested as the order of the files read by the program can vary.
echo "------------------- f2s testcase 9 >>> format:single_and_multi-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5/1ssm1.fast5 $FAST5_DIR/single-and-multi-fast5/2sss1.fast5 $FAST5_DIR/single-and-multi-fast5/3ssm2.fast5 $FAST5_DIR/single-and-multi-fast5/4sss2.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/file_single-and-multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- f2s testcase 10 >>> format:single_and_multi-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5; then
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
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 11 >>> format:single-fast5 input:file process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5; then
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
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 12 >>> format:single-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5; then
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
rm $OUTPUT_DIR/multi-fast5/*
echo "------------------- f2s testcase 13 >>> format:multi-fast5 input:file process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
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
rm $OUTPUT_DIR/multi-fast5/*
echo "------------------- f2s testcase 14 >>> format:multi-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
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
rm $OUTPUT_DIR/single-and-multi-fast5/*
echo "------------------- f2s testcase 15 >>> format:single_and_multi-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5; then
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
echo "------------------- f2s testcase 16 >>> format:single-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 17 >>> format:single-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- f2s testcase 18 >>> format:multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

echo
echo "------------------- f2s testcase 19 >>> format:single_and_multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"


echo
echo "------------------- f2s testcase 20 >>> format:single_and_multi-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 -d $OUTPUT_DIR/single_and_multi-fast5; then
    echo "${RED}testcase failed${NC}" 
    exit 1
fi
echo -e "${GREEN}testcase passed${NC}"

rm -r $OUTPUT_DIR
exit
