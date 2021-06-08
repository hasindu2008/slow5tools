#!/bin/bash
# Run f2s with different file, input and output formats.
Usage="f2s_output_test.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

OUTPUT_DIR="$REL_PATH/data/out/f2s_output"
test -d  $OUTPUT_DIR
rm -r $OUTPUT_DIR
mkdir $OUTPUT_DIR


FAST5_DIR=$REL_PATH/data/raw/f2s_output
EXP_SLOW5_DIR=$REL_PATH/data/exp/f2s_output

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $REL_PATH/../slow5tools"
else
    SLOW5_EXEC=$REL_PATH/../slow5tools
fi

echo "-------------------slow5tools version-------------------"
if ! $SLOW5_EXEC --version; then
    echo "slow5tools version failed"
    exit 1
fi
echo
echo "------------------- f2s testcase 1 >>> format:single-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 1 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single-fast5 input:file process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 1 passed${NC}"

echo
echo "------------------- f2s testcase 2 >>> format:single-fast5 input:file process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5; then
    echo "${RED}testcase 2 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single-fast5 input:file process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 2 passed${NC}"

echo
echo "------------------- f2s testcase 3 >>> format:single-fast5 input:directory process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 3 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'f2s format:single-fast5 input:directory process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 3 passed${NC}"

echo
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 4 >>> format:single-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5; then
    echo "${RED}testcase 4 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'f2s format:single-fast5 input:directory process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 4 passed${NC}"

echo
echo "------------------- f2s testcase 5 >>> format:multi-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 5 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 5 passed${NC}"

echo
echo "------------------- f2s testcase 6 >>> format:multi-fast5 input:file process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase 6 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:single_process output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 6 passed${NC}"

echo
echo "------------------- f2s testcase 7 >>> format:multi-fast5 input:directory process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 7 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/directory_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:directory process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 7 passed${NC}"

echo
rm $OUTPUT_DIR/multi-fast5/*
echo "------------------- f2s testcase 8 >>> format:multi-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase 8 failed ${NC}"
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
echo -e "${GREEN}testcase 8 passed${NC}"

echo
rm $OUTPUT_DIR/stdout.slow5
#input:directory on a single process cannot be tested as the order of the files read by the program can vary.
echo "------------------- f2s testcase 9 >>> format:single_and_multi-fast5 input:file process:single_process output:stdout-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5/1ssm1.fast5 $FAST5_DIR/single-and-multi-fast5/2sss1.fast5 $FAST5_DIR/single-and-multi-fast5/3ssm2.fast5 $FAST5_DIR/single-and-multi-fast5/4sss2.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 9 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-and-multi-fast5-output/file_single-and-multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single_and_multi-fast5 input:directory process:single_process output:stdout'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 9 passed${NC}"

echo
echo "------------------- f2s testcase 10 >>> format:single_and_multi-fast5 input:directory process:single_process output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5; then
    echo "${RED}testcase 10 failed ${NC}"
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
echo -e "${GREEN}testcase 10 passed${NC}"

# ----------------------------------------------- multi process --------------------------------------------

echo
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 11 >>> format:single-fast5 input:file process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5; then
    echo "${RED}testcase 11 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:single-fast5 input:file process:multi output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 11 passed${NC}"

echo
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 12 >>> format:single-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5; then
    echo "${RED}testcase 12 failed ${NC}"
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
echo -e "${GREEN}testcase 12 passed${NC}"

echo
rm $OUTPUT_DIR/multi-fast5/*
echo "------------------- f2s testcase 13 >>> format:multi-fast5 input:file process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase 13 failed ${NC}"
    exit 1
fi
diff -s $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: diff failed for 'format:multi-fast5 input:file process:multi output:directory'${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 13 passed${NC}"

echo
rm $OUTPUT_DIR/multi-fast5/*
echo "------------------- f2s testcase 14 >>> format:multi-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5; then
    echo "${RED}testcase 14 failed ${NC}"
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
echo -e "${GREEN}testcase 14 passed${NC}"

echo
rm $OUTPUT_DIR/single-and-multi-fast5/*
echo "------------------- f2s testcase 15 >>> format:single_and_multi-fast5 input:directory process:multi output:directory-------------------"
if ! $SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5; then
    echo "${RED}testcase 15 failed ${NC}"
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
echo -e "${GREEN}testcase 15 passed${NC}"

#----------------------------------------- run id conflicts -------------------------------------------

echo
echo "------------------- f2s testcase 16 >>> format:single-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 16 failed ${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 16 passed${NC}"

echo
rm $OUTPUT_DIR/single-fast5-output/*
echo "------------------- f2s testcase 17 >>> format:single-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5; then
    echo "${RED}testcase 17 failed ${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 17 passed${NC}"

echo
echo "------------------- f2s testcase 18 >>> format:multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 18 failed ${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 18 passed${NC}"

echo
echo "------------------- f2s testcase 19 >>> format:single_and_multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 19 failed ${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 19 passed${NC}"

echo
echo "------------------- f2s testcase 20 >>> format:single_and_multi-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
if $SLOW5_EXEC f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 -d $OUTPUT_DIR/single_and_multi-fast5; then
    echo "${RED}testcase 20 failed ${NC}"
    exit 1
fi
echo -e "${GREEN}testcase 20 passed${NC}"

echo
echo "------------------- f2s testcase 21 >>> current directory:fast5 file directory-------------------"
cd $FAST5_DIR/single-fast5
CD_BACK=../../../../..
if ! $CD_BACK/slow5tools f2s sss1.fast5 --iop 1 --to slow5 > $CD_BACK/$OUTPUT_DIR/stdout.slow5; then
    echo "${RED}testcase 21 failed ${NC}"
    exit 1
fi
cd -
echo -e "${GREEN}testcase 21 passed${NC}"

rm -r $OUTPUT_DIR
exit
