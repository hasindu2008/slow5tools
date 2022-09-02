#!/bin/bash
# Run f2s with different file, input and output formats.
Usage="test_s2f.sh"

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"

RED='\033[0;31m' ; GREEN='\033[0;32m' ; NC='\033[0m' # No Color

# terminate script
die() {
    echo -e "${RED}$1${NC}" 1>&3 2>&4
    echo
    exit 1
}
#redirect
verbose=0
exec 3>&1
exec 4>&2
if ((verbose)); then
  echo "verbose=1"
else
  echo "verbose=0"
  exec 1>/dev/null
  exec 2>/dev/null
fi
#echo "this should be seen if verbose"
#echo "this should always be seen" 1>&3 2>&4

OUTPUT_DIR="$REL_PATH/data/out/f2s"
test -d  $OUTPUT_DIR && rm -r "$OUTPUT_DIR"
mkdir "$OUTPUT_DIR" || die "Creating $OUTPUT_DIR failed"

FAST5_DIR=$REL_PATH/data/raw/f2s
EXP_SLOW5_DIR=$REL_PATH/data/exp/f2s
SLOW5_EXEC_WITHOUT_VALGRIND=$REL_PATH/../slow5tools

if [ "$1" = 'mem' ]; then
    SLOW5_EXEC="valgrind --leak-check=full --error-exitcode=1 $SLOW5_EXEC_WITHOUT_VALGRIND"
else
    SLOW5_EXEC=$SLOW5_EXEC_WITHOUT_VALGRIND
fi

echo "-------------------slow5tools version-------------------"
$SLOW5_EXEC --version || die "slow5tools version failed"
echo

#### Single process tests

TESTCASE_NO=1.1
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:file process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=1.2
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:file process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=1.3
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=1.4
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=1.5
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:file process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=1.6
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:file process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=1.7
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/directory_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/multi-fast5/*
TESTCASE_NO=1.8
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm2.slow5 $OUTPUT_DIR/multi-fast5/ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm3.slow5 $OUTPUT_DIR/multi-fast5/ssm3.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/stdout.slow5
#input:directory on a single process cannot be tested as the order of the files read by the program can vary.
TESTCASE_NO=1.9
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:file process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5/1ssm1.fast5 $FAST5_DIR/single-and-multi-fast5/2sss1.fast5 $FAST5_DIR/single-and-multi-fast5/3ssm2.fast5 $FAST5_DIR/single-and-multi-fast5/4sss2.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/file_single-and-multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=1.10
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/0.slow5 $OUTPUT_DIR/single-and-multi-fast5/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/1ssm1.slow5 $OUTPUT_DIR/single-and-multi-fast5/1ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/3ssm2.slow5 $OUTPUT_DIR/single-and-multi-fast5/3ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=1.11
echo "------------------- f2s testcase $TESTCASE_NO >>> current directory:fast5 file directory output: stdout-------------------"
cd $FAST5_DIR/single-fast5
CD_BACK=../../../../..
$CD_BACK/slow5tools f2s sss1.fast5 --iop 1 --to slow5 > $CD_BACK/$OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
cd -
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


# ----------------------------------------------- multi process --------------------------------------------

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=2.1
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:file process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single-fast5 input:file process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=2.2
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:multi output"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:multi output"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/multi-fast5/*
TESTCASE_NO=2.3
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:file process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/multi-fast5/*
TESTCASE_NO=2.4
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm2.slow5 $OUTPUT_DIR/multi-fast5/ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm3.slow5 $OUTPUT_DIR/multi-fast5/ssm3.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-and-multi-fast5/*
TESTCASE_NO=2.5
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/1ssm1.slow5 $OUTPUT_DIR/single-and-multi-fast5/1ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:multi output"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/3ssm2.slow5 $OUTPUT_DIR/single-and-multi-fast5/3ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

#----------------------------------------- run id conflicts -------------------------------------------
echo
TESTCASE_NO=3.1
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=3.2
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3.3
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3.4
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3.5
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 -d $OUTPUT_DIR/single_and_multi-fast5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3.6
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:stdout run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 --iop 1 --to slow5 --allow > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=3.7
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:directory run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 --allow || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3.8
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:stdout run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 --allow > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3.9
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:stdout run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 --allow > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3.10
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:directory run_id_conflicts allowed-------------------"
test -d "$OUTPUT_DIR/single_and_multi-fast5" && rm -r "$OUTPUT_DIR/single_and_multi-fast5"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --allow -d $OUTPUT_DIR/single_and_multi-fast5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


## Output formats

TESTCASE_NO=4.1
echo "------------------- f2s testcase $TESTCASE_NO >>> blow5 zlib output using -o -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 -o $OUTPUT_DIR/ssm1.blow5  -c zlib -s none
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1_zlib.blow5 $OUTPUT_DIR/ssm1.blow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for blow zlib out"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=4.2
echo "------------------- f2s testcase $TESTCASE_NO >>> blow5 zlib-svb output using -o -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 -o $OUTPUT_DIR/ssm1.blow5 -c zlib -s svb-zd
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1_zlib_svb.blow5  $OUTPUT_DIR/ssm1.blow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for blow zlib-svb out"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.3
echo "------------------- f2s testcase $TESTCASE_NO >>> slow5 output using -o -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 -o $OUTPUT_DIR/ssm1.slow5
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5  $OUTPUT_DIR/ssm1.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for slow5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


TESTCASE_NO=4.4
echo "------------------- f2s testcase $TESTCASE_NO >>> blow5 zlib-svb output to stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to blow5 -c zlib -s svb-zd > $OUTPUT_DIR/ssm1.blow5
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1_zlib_svb.blow5 $OUTPUT_DIR/ssm1.blow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for blow zlib out"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=4.5
echo "------------------- f2s testcase $TESTCASE_NO >>> slow5 output to stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to slow5  > $OUTPUT_DIR/ssm1.slow5
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/ssm1.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for slow5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=4.6
echo "------------------- f2s testcase $TESTCASE_NO >>> output extension and --to mismatch -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to slow5  -o $OUTPUT_DIR/ssm1.blow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=4.7
echo "------------------- f2s testcase $TESTCASE_NO >>> compression requested with slow5 -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to slow5  -c zlib $OUTPUT_DIR/err.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=4.8
echo "------------------- f2s testcase $TESTCASE_NO >>> current directory:fast5 file directory output: file-------------------"
cd $FAST5_DIR/single-fast5
CD_BACK=../../../../..
$CD_BACK/slow5tools f2s sss1.fast5 --iop 1 --to slow5 -o $CD_BACK/$OUTPUT_DIR/$TESTCASE_NO.slow5 || die "testcase $TESTCASE_NO failed"
cd -
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=4.9
echo "------------------- f2s testcase $TESTCASE_NO >>> current directory:fast5 file directory output: directory-------------------"
cd $FAST5_DIR/single-fast5
CD_BACK=../../../../..
$CD_BACK/slow5tools -v 7 f2s sss1.fast5 --iop 1 --to slow5 -d $CD_BACK/$OUTPUT_DIR/$TESTCASE_NO || die "testcase $TESTCASE_NO failed"
cd -
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


echo
TESTCASE_NO=4.10
echo "------------------- f2s testcase $TESTCASE_NO >>> retain_dir_structure without --retain failure expected -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/retain_dir_structure -d $OUTPUT_DIR/retain_dir_structure && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=4.11
echo "------------------- f2s testcase $TESTCASE_NO >>> retain_dir_structure-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/retain_dir_structure -d $OUTPUT_DIR/retain_dir_structure --retain || die "testcase $TESTCASE_NO failed"
diff $EXP_SLOW5_DIR/retain_dir_structure  $OUTPUT_DIR/retain_dir_structure > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=4.12
echo "------------------- f2s testcase $TESTCASE_NO >>> duplicate file name -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ $FAST5_DIR/multi-fast5/ -d $OUTPUT_DIR/dupli 2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE_NO failed"
grep -q "ERROR.* Two or more fast5 files have the same filename.*" $OUTPUT_DIR/err.log || die "ERROR: f2s_test testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

########################## Stupid end reason ####################


TESTCASE_NO=5.1
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason fast5-------------------"
mkdir -p $OUTPUT_DIR/end_reason_fast5 || die "creating $OUTPUT_DIR/end_reason_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason0.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason0.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason0.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason0.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


TESTCASE_NO=5.2
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason datatype is uint8_t-------------------"
mkdir -p $OUTPUT_DIR/end_reason_fast5 || die "creating $OUTPUT_DIR/end_reason_fast5 failed"
LOG=$OUTPUT_DIR/end_reason_fast5/err.log
#$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_datatype_uint8_t.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_datatype_uint8_t.slow5 || die "testcase $TESTCASE_NO failed"
#diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason_datatype_uint8_t.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason_datatype_uint8_t.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_datatype_uint8_t.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_datatype_uint8_t.slow5 2> $LOG && die "testcase $TESTCASE_NO failed"
grep -q -i "ERROR.*This is a known issue in ont_fast5_api's compress_fast5" $LOG || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=5.3
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason found only in the first read group fast5 2-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason2.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason2.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason2.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason2.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=5.4
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason datatype is string-------------------"
LOG=$OUTPUT_DIR/end_reason_fast5/err.log
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_string.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_string.slow5 2> $LOG  && die "testcase $TESTCASE_NO failed"
grep -q -i "ERROR.*The datatype of the attribute Raw/end_reason.*H5T_STRING instead of H5T_ENUM.* " $LOG || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=5.5
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason datatype is int32_t-------------------"
LOG=$OUTPUT_DIR/end_reason_fast5/err.log
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_int32.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_int32.slow5 2> $LOG  && die "testcase $TESTCASE_NO failed"
grep -q -i "ERROR.*The datatype of the attribute Raw/end_reason.*H5T_STD_I32LE instead of H5T_ENUM.* " $LOG || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=5.6
echo "------------------- f2s testcase $TESTCASE_NO >>> a different key label-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_differnt_key.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_differnt_key.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason_differnt_key.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason_differnt_key.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=5.7
echo "------------------- f2s testcase $TESTCASE_NO >>> a different order of label-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_differnt_key_order.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_differnt_key_order.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason_differnt_key_order.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason_differnt_key_order.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=5.8
echo "------------------- f2s testcase $TESTCASE_NO >>> a new keylabel-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_new_key.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_new_key.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason_new_key.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason_new_key.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

# we assume that if the run_id are the same, enum_labels are the same - so we don't test this
# also we assume that if the first read does not have the enum, the rest will not as well
# TESTCASE_NO=35
# echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason found only in the second read group fast5 1-------------------"
# mkdir -p $OUTPUT_DIR/end_reason_fast5 || die "creating $OUTPUT_DIR/end_reason_fast5 failed"
# $SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason1.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason1.slow5 || die "testcase $TESTCASE_NO failed"
# diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason1.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason1.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
# echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

#--------------------------------------Unusual FAST5------------------------------------

echo
TESTCASE_NO=6.1
echo "------------------- f2s testcase $TESTCASE_NO >>> auxiliary field missing fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/median_before_missing.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/median_before_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/unusual_fast5/median_before_missing.slow5 $OUTPUT_DIR/unusual_fast5/median_before_missing.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for auxiliary field missing fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=6.2
echo "------------------- f2s testcase $TESTCASE_NO >>> primary field missing fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/offset_missing.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/offset_missing.slow5 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=6.3
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_first_read_group_tracking_id fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_first_read_group_tracking_id.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.4
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_first_read_group_read but is in the tracking_id group fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_first_read_group_read.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.5
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_fifth_read_group_read but is in the tracking_id group fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_fifth_read_group_read.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.6
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_first_read_group_in_both_read_and_tracking_id fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_first_read_group_in_both_read_and_tracking_id.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.7
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_fifth_read_group_in_both_read_and_tracking_id fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_fifth_read_group_in_both_read_and_tracking_id.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.8
echo "------------------- f2s testcase $TESTCASE_NO >>> new non string attribute in context tags  -------------------"
# This is a header attribute, thus should be converted to string, but with a warning
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
LOG=$OUTPUT_DIR/unusual_fast5/new_attrib_in_context.log
OUTPUT_FILE=$OUTPUT_DIR/unusual_fast5/new_attrib_in_context.slow5
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_attrib_in_context.fast5 -o $OUTPUT_FILE 2> $LOG  || die "testcase $TESTCASE_NO failed"
grep -q -i "WARNING.*converting.*to string" $LOG || die "Warning in testcase $TESTCASE_NO failed"
diff $EXP_SLOW5_DIR/unusual_fast5/new_attrib_in_context.slow5 $OUTPUT_FILE  > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.9
echo "------------------- f2s testcase $TESTCASE_NO >>> new non string attribute in tracking id  -------------------"
# This is a header attribute, thus should be converted to string, but with a warning
LOG="$OUTPUT_DIR/unusual_fast5/new_attrib_in_tracking.log"
OUTPUT_FILE=$OUTPUT_DIR/unusual_fast5/new_attrib_in_tracking.slow5
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_attrib_in_tracking.fast5 -o $OUTPUT_FILE 2> $LOG || die "testcase $TESTCASE_NO failed"
grep -q -i "WARNING.*converting.*to string" $LOG || die "Warning in testcase $TESTCASE_NO failed"
diff $EXP_SLOW5_DIR/unusual_fast5/new_attrib_in_tracking.slow5 $OUTPUT_FILE  > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.10
echo "------------------- f2s testcase $TESTCASE_NO >>> new string attribute in context tags  -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_str_attrib_in_context.fast5 -o $OUTPUT_DIR/unusual_fast5/new_str_attrib_in_context.slow5
diff $EXP_SLOW5_DIR/unusual_fast5/new_str_attrib_in_context.slow5 $OUTPUT_DIR/unusual_fast5/new_str_attrib_in_context.slow5  > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.11
echo "------------------- f2s testcase $TESTCASE_NO >>> new string attribute in tracking id  -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_str_attrib_in_tracking.fast5 -o $OUTPUT_DIR/unusual_fast5/new_str_attrib_in_tracking.slow5
diff $EXP_SLOW5_DIR/unusual_fast5/new_str_attrib_in_tracking.slow5 $OUTPUT_DIR/unusual_fast5/new_str_attrib_in_tracking.slow5  > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.12
echo "------------------- f2s testcase $TESTCASE_NO >>> new attribute in raw  -------------------"
#This is likely to be a per-read attribute that we need to inspect manually. So must Error out unless -a is specified.
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_attrib_in_raw.fast5 -o $OUTPUT_DIR/err.slow5  2> $OUTPUT_DIR/err.log  && die "testcase $TESTCASE_NO failed"
cat $OUTPUT_DIR/err.log
grep -q "ERROR.*Attribute .* in .* is unexpected" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.13
echo "------------------- f2s testcase $TESTCASE_NO >>> new attribute in read -------------------"
# #This is likely to be a per-read attribute that we need to inspect manually. So must Error out unless -a is specified.
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_attrib_in_read.fast5 -o $OUTPUT_DIR/err.slow5 2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE_NO failed"
grep -q "ERROR.*unexpected" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.14
echo "------------------- f2s testcase $TESTCASE_NO >>> new attribute in channel -------------------"
# #This is likely to be a per-read attribute that we need to inspect manually. So must Error out unless -a is specified.
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_attrib_in_channel.fast5 -o $OUTPUT_DIR/err.slow5 2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE_NO failed"
grep -q "ERROR.*Attribute .* in .* is unexpected" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


TESTCASE_NO=6.15
echo "------------------- f2s testcase $TESTCASE_NO >>> new group   -------------------"
# Unless the group is /Analyses which we ignore anyway, we will need to manually inspect what the newly added group is
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/new_group.fast5 -o $OUTPUT_DIR/err.slow5  2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE_NO failed"
grep -q "ERROR.*Attribute .* in .* is unexpected" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=6.16
echo "------------------- f2s testcase $TESTCASE_NO >>> pore type set   -------------------"
# if pore type is something othe rthan being empty or "no_set", we need to manually investigate what the heck thi sis
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/pore_set.fast5 -o $OUTPUT_DIR/err.slow5 2> $OUTPUT_DIR/err.log  && die "testcase $TESTCASE_NO failed"
grep -q "ERROR.*expected to be 'not_set'" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4



####################### Erroeneous FAST5 ########################

TESTCASE_NO=7.1
echo "------------------- f2s testcase $TESTCASE_NO >>> not a fast5 -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/not_a_fast5.fast5  &&  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.2
echo "------------------- f2s testcase $TESTCASE_NO >>> non existent file -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/there_is_no_such_fast5.fast5  &&  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.3
echo "------------------- f2s testcase $TESTCASE_NO >>> empty directory -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/empty_dir  &&  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.4
echo "------------------- f2s testcase $TESTCASE_NO >>> empty fast5 file -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/empty_fast5.fast5  &&  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


TESTCASE_NO=7.5
echo "------------------- f2s testcase $TESTCASE_NO >>> # in read id at the beginning -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/malformed_readid1.fast5 -o $OUTPUT_DIR/err.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.6
echo "------------------- f2s testcase $TESTCASE_NO >>> @ in read id at the beginning -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/malformed_readid1.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.7
echo "------------------- f2s testcase $TESTCASE_NO >>> Duplicated attribute -------------------"
# If there is a duplicate header attribute coming from two separate groups, need some manual investigation
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/dupli_attrib.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.8
echo "------------------- f2s testcase $TESTCASE_NO >>> tab in attribute name 'operating system'-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/tab_in_attrib.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.9
echo "------------------- f2s testcase $TESTCASE_NO >>> new line in attribute 'exp_script_name' value-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/newline_in_attrib.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.10
echo "------------------- f2s testcase $TESTCASE_NO >>> new line in field name 'channel number'-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/newline_in_field.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.11
echo "------------------- f2s testcase $TESTCASE_NO >>> tab in field value 'channel number'-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/tab_in_field.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.12
echo "------------------- f2s testcase $TESTCASE_NO >>> primary field 'range' missing -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/missing_primary_field.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.13
echo "------------------- f2s testcase $TESTCASE_NO >>> primary field 'range' wrong type -------------------"
#any primary field should match to what we expect
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/primary_field_wrong_type.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.14
#any auxilliary field should match to what we expect (note: multiple datatypes can be expected, if we know about this)
echo "------------------- f2s testcase $TESTCASE_NO >>> aux field 'channel number' wrong type -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/aux_field_wrong_type.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.15
echo "------------------- f2s testcase $TESTCASE_NO >>> R7 file   -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/R7_1.fast5 -o $OUTPUT_DIR/err.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=7.16
echo "------------------- f2s testcase $TESTCASE_NO >>> all fast5 files were skipped   -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/ -o $OUTPUT_DIR/err.slow5 -p1 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

####################### Various versions of FAST5 ########################

mkdir -p $OUTPUT_DIR/various_versions || die "creating $OUTPUT_DIR/various_versions failed"

TESTCASE_NO=8.1
echo "------------------- f2s testcase $TESTCASE_NO >>> FAST5 compressed using compress_fast5 -------------------"
# $SLOW5_EXEC f2s $FAST5_DIR/various_versions/compress_fast5.fast5 -o $OUTPUT_DIR/various_versions/compress_fast5.slow5  2> $OUTPUT_DIR/err.log|| die "testcase $TESTCASE_NO failed"
# diff -q $EXP_SLOW5_DIR/various_versions/compress_fast5.slow5 $OUTPUT_DIR/various_versions/compress_fast5.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for compress_fast5"
# grep -q -i "WARNING.*Attribute Raw/end_reason in.*is corrupted" $OUTPUT_DIR/err.log || die "Warning in testcase $TESTCASE_NO failed"
$SLOW5_EXEC f2s $FAST5_DIR/various_versions/compress_fast5.fast5 -o $OUTPUT_DIR/various_versions/compress_fast5.slow5  2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE_NO failed"
grep -q -i "ERROR.*Attribute Raw/end_reason in.*is corrupted" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TEST_FAST5_VERSION () {
  FILE_NAME=$1
  echo "------------------- f2s testcase $TESTCASE_NO >>> ${FILE_NAME} -------------------"
  $SLOW5_EXEC f2s $FAST5_DIR/various_versions/${FILE_NAME}.fast5 -o $OUTPUT_DIR/various_versions/${FILE_NAME}.slow5 || die "testcase $TESTCASE_NO failed"
  diff -q $EXP_SLOW5_DIR/various_versions/${FILE_NAME}.slow5 $OUTPUT_DIR/various_versions/${FILE_NAME}.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for ${FILE_NAME}"
  echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4
}

TESTCASE_NO=8.2 TEST_FAST5_VERSION multi_fast5_v1.0
TESTCASE_NO=8.3 TEST_FAST5_VERSION multi_fast5_v1.0
TESTCASE_NO=8.4 TEST_FAST5_VERSION multi_fast5_v2.2_basecalled
TESTCASE_NO=8.5 TEST_FAST5_VERSION multi_fast5_v2.3_barcoded
TESTCASE_NO=8.6 TEST_FAST5_VERSION multi_fast5_v2.3
TESTCASE_NO=8.7 TEST_FAST5_VERSION single_fast5_v0.6
TESTCASE_NO=8.8 TEST_FAST5_VERSION single_fast5_v1.0
TESTCASE_NO=8.9 TEST_FAST5_VERSION single_fast5_v2.0
TESTCASE_NO=8.10 TEST_FAST5_VERSION single_v0.6_to_multi_v2.0_fast5_same_run_id
TESTCASE_NO=8.11 TEST_FAST5_VERSION single_v1.0_to_multi_v2.0_fast5_same_run_id
TESTCASE_NO=8.12 TEST_FAST5_VERSION single_vnull_to_multi_2.0

TESTCASE_NO=8.13
FILE=single_v0.6_to_multi_v2.0_fast5_diff_run_id
echo "------------------- f2s testcase $TESTCASE_NO >>>  $FILE -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/various_versions/$FILE.fast5 -o $OUTPUT_DIR/various_versions/$FILE.slow5  2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE_NO failed"
grep -q -i "ERROR.*Ancient fast5: Different run_ids found in an individual multi-fast5 file. Cannot create a single header slow5/blow5" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=8.14
FILE=single_v1.0_to_multi_v2.0_fast5_diff_run_id
echo "------------------- f2s testcase $TESTCASE_NO >>>  $FILE -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/various_versions/$FILE.fast5 -o $OUTPUT_DIR/various_versions/$FILE.slow5  2> $OUTPUT_DIR/err.log && die "testcase $TESTCASE_NO failed"
grep -q -i "ERROR.*Ancient fast5: Different run_ids found in an individual multi-fast5 file. Cannot create a single header slow5/blow5" $OUTPUT_DIR/err.log || die "Error in testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=8.15 TEST_FAST5_VERSION single_fast5_v1.0_starttime0

rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
