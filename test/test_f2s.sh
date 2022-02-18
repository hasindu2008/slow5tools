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
verbose=1
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

TESTCASE_NO=1
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:file process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=2
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:file process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=3
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=4
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/directory_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=5
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:file process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=6
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:file process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=7
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5>$OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/directory_multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/multi-fast5/*
TESTCASE_NO=8
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm2.slow5 $OUTPUT_DIR/multi-fast5/ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm3.slow5 $OUTPUT_DIR/multi-fast5/ssm3.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/stdout.slow5
#input:directory on a single process cannot be tested as the order of the files read by the program can vary.
TESTCASE_NO=9
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:file process:single_process output:stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5/1ssm1.fast5 $FAST5_DIR/single-and-multi-fast5/2sss1.fast5 $FAST5_DIR/single-and-multi-fast5/3ssm2.fast5 $FAST5_DIR/single-and-multi-fast5/4sss2.fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/file_single-and-multi-fast5.slow5 $OUTPUT_DIR/stdout.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=10
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 1 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/0.slow5 $OUTPUT_DIR/single-and-multi-fast5/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/1ssm1.slow5 $OUTPUT_DIR/single-and-multi-fast5/1ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/3ssm2.slow5 $OUTPUT_DIR/single-and-multi-fast5/3ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:single_process output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

# ----------------------------------------------- multi process --------------------------------------------

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=11
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:file process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5/sss1.fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single-fast5 input:file process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=12
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-fast5 -d $OUTPUT_DIR/single-fast5-output --iop 4 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/0.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:multi output"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:multi output"
diff -q $EXP_SLOW5_DIR/single-fast5-output/a_single-fast5.slow5 $OUTPUT_DIR/single-fast5-output/2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'f2s format:single-fast5 input:directory process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/multi-fast5/*
TESTCASE_NO=13
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:file process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/file_multi-fast5.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/multi-fast5/*
TESTCASE_NO=14
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/multi-fast5/ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm2.slow5 $OUTPUT_DIR/multi-fast5/ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
diff -q $EXP_SLOW5_DIR/multi-fast5-output/ssm3.slow5 $OUTPUT_DIR/multi-fast5/ssm3.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:multi-fast5 input:file process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-and-multi-fast5/*
TESTCASE_NO=15
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:multi output:directory-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/single-and-multi-fast5 --iop 4 --to slow5 -d $OUTPUT_DIR/single-and-multi-fast5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/1ssm1.slow5 $OUTPUT_DIR/single-and-multi-fast5/1ssm1.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:multi output"
diff -q $EXP_SLOW5_DIR/single-and-multi-fast5-output/3ssm2.slow5 $OUTPUT_DIR/single-and-multi-fast5/3ssm2.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for 'format:single_and_multi-fast5 input:directory process:multi output"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

#----------------------------------------- run id conflicts -------------------------------------------
echo
TESTCASE_NO=16
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=17
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=18
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=19
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:stdout run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 > $OUTPUT_DIR/stdout.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=20
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:directory run_id_conflicts-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 -d $OUTPUT_DIR/single_and_multi-fast5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=21
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:stdout run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 --iop 1 --to slow5 --allow > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
rm $OUTPUT_DIR/single-fast5-output/*
TESTCASE_NO=22
echo "------------------- f2s testcase $TESTCASE_NO: format:single-fast5 input:directory process:single_process output:directory run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 -d $OUTPUT_DIR/single-fast5-output --iop 1 --to slow5 --allow || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=23
echo "------------------- f2s testcase $TESTCASE_NO: format:multi-fast5 input:directory process:single_process output:stdout run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 --allow > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=24
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:stdout run_id_conflicts allowed-------------------"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --to slow5 --allow > $OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=25
echo "------------------- f2s testcase $TESTCASE_NO: format:single_and_multi-fast5 input:directory process:single_process output:directory run_id_conflicts allowed-------------------"
test -d "$OUTPUT_DIR/single_and_multi-fast5" && rm -r "$OUTPUT_DIR/single_and_multi-fast5"
$SLOW5_EXEC_WITHOUT_VALGRIND f2s $FAST5_DIR/run_id_conflicts/single_fast5 $FAST5_DIR/run_id_conflicts/multi_fast5 --iop 1 --allow -d $OUTPUT_DIR/single_and_multi-fast5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

#---------------------------------------------------------------------------------------------------

echo
TESTCASE_NO=26
echo "------------------- f2s testcase $TESTCASE_NO >>> current directory:fast5 file directory output: stdout-------------------"
cd $FAST5_DIR/single-fast5
CD_BACK=../../../../..
$CD_BACK/slow5tools f2s sss1.fast5 --iop 1 --to slow5 > $CD_BACK/$OUTPUT_DIR/stdout.slow5 || die "testcase $TESTCASE_NO failed"
cd -
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=27
echo "------------------- f2s testcase $TESTCASE_NO >>> auxiliary field missing fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/median_before_missing.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/median_before_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/unusual_fast5/median_before_missing.slow5 $OUTPUT_DIR/unusual_fast5/median_before_missing.slow5 || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for auxiliary field missing fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=28
echo "------------------- f2s testcase $TESTCASE_NO >>> primary field missing fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/offset_missing.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/offset_missing.slow5 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=29
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_first_read_group_tracking_id fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_first_read_group_tracking_id.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=30
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_first_read_group_read but is in the tracking_id group fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_first_read_group_read.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=31
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_fifth_read_group_read but is in the tracking_id group fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_fifth_read_group_read.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 || die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=32
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_first_read_group_in_both_read_and_tracking_id fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_first_read_group_in_both_read_and_tracking_id.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=33
echo "------------------- f2s testcase $TESTCASE_NO >>> run_id_missing_in_fifth_read_group_in_both_read_and_tracking_id fast5-------------------"
mkdir -p $OUTPUT_DIR/unusual_fast5 || die "creating $OUTPUT_DIR/unusual_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/unusual_fast5/run_id_missing_in_fifth_read_group_in_both_read_and_tracking_id.fast5 --iop 1 -o $OUTPUT_DIR/unusual_fast5/run_id_missing.slow5 --to slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=34
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason fast5-------------------"
mkdir -p $OUTPUT_DIR/end_reason_fast5 || die "creating $OUTPUT_DIR/end_reason_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason0.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason0.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason0.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason0.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

#TESTCASE_NO=35
#echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason found only in the second read group fast5 1-------------------"
#mkdir -p $OUTPUT_DIR/end_reason_fast5 || die "creating $OUTPUT_DIR/end_reason_fast5 failed"
#$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason1.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason1.slow5 || die "testcase $TESTCASE_NO failed"
#diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason1.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason1.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
#echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=35
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason found only in the first read group fast5 2-------------------"
mkdir -p $OUTPUT_DIR/end_reason_fast5 || die "creating $OUTPUT_DIR/end_reason_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason2.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason2.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason2.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason2.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=36
echo "------------------- f2s testcase $TESTCASE_NO >>> not a fast5 -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/not_a_fast5.fast5  ||  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=37
echo "------------------- f2s testcase $TESTCASE_NO >>> non existent file -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/there_is_no_such_fast5.fast5  ||  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=38
echo "------------------- f2s testcase $TESTCASE_NO >>> empty directory -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/empty_dir  &&  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=39
echo "------------------- f2s testcase $TESTCASE_NO >>> empty fast5 file -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/empty_fast5.fast5  &&  die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


TESTCASE_NO=40
echo "------------------- f2s testcase $TESTCASE_NO >>> blow5 zlib output using -o -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 -o $OUTPUT_DIR/ssm1.blow5  -c zlib -s none
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1_zlib.blow5 $OUTPUT_DIR/ssm1.blow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for blow zlib out"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=41
echo "------------------- f2s testcase $TESTCASE_NO >>> blow5 zlib-svb output using -o -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 -o $OUTPUT_DIR/ssm1.blow5 -c zlib -s svb-zd
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1_zlib_svb.blow5  $OUTPUT_DIR/ssm1.blow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for blow zlib-svb out"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=42
echo "------------------- f2s testcase $TESTCASE_NO >>> slow5 output using -o -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 -o $OUTPUT_DIR/ssm1.slow5
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5  $OUTPUT_DIR/ssm1.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for slow5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4


TESTCASE_NO=43
echo "------------------- f2s testcase $TESTCASE_NO >>> blow5 zlib-svb output to stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to blow5 -c zlib -s svb-zd > $OUTPUT_DIR/ssm1.blow5
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1_zlib_svb.blow5 $OUTPUT_DIR/ssm1.blow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for blow zlib out"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=44
echo "------------------- f2s testcase $TESTCASE_NO >>> slow5 output to stdout-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to slow5  > $OUTPUT_DIR/ssm1.slow5
diff $EXP_SLOW5_DIR/multi-fast5-output/ssm1.slow5 $OUTPUT_DIR/ssm1.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for slow5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=45
echo "------------------- f2s testcase $TESTCASE_NO >>> output extension and --to mismatch -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to slow5  -o $OUTPUT_DIR/ssm1.blow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=46
echo "------------------- f2s testcase $TESTCASE_NO >>> compression requested with slow5 -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/multi-fast5/ssm1.fast5 --to slow5  -c zlib $OUTPUT_DIR/err.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=47
echo "------------------- f2s testcase $TESTCASE_NO >>> # in reead id at the beginning -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/malformed_readid1.fast5 -o $OUTPUT_DIR/err.slow5 && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=48
echo "------------------- f2s testcase $TESTCASE_NO >>> @ in reead id at the beginning -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/malformed_readid1.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=49
echo "------------------- f2s testcase $TESTCASE_NO >>> primary field range missing -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/err_fast5/missing_primary_field.fast5 -o $OUTPUT_DIR/err.slow5  && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=50
echo "------------------- f2s testcase $TESTCASE_NO >>> current directory:fast5 file directory output: file-------------------"
cd $FAST5_DIR/single-fast5
CD_BACK=../../../../..
$CD_BACK/slow5tools f2s sss1.fast5 --iop 1 --to slow5 -o $CD_BACK/$OUTPUT_DIR/$TESTCASE_NO.slow5 || die "testcase $TESTCASE_NO failed"
cd -
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=51
echo "------------------- f2s testcase $TESTCASE_NO >>> current directory:fast5 file directory output: directory-------------------"
cd $FAST5_DIR/single-fast5
CD_BACK=../../../../..
$CD_BACK/slow5tools -v 7 f2s sss1.fast5 --iop 1 --to slow5 -d $CD_BACK/$OUTPUT_DIR/$TESTCASE_NO || die "testcase $TESTCASE_NO failed"
cd -
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

verbose=1

echo
TESTCASE_NO=52
echo "------------------- f2s testcase $TESTCASE_NO >>> retain_dir_structure without --retain failure expected -------------------"
$SLOW5_EXEC f2s $FAST5_DIR/retain_dir_structure -d $OUTPUT_DIR/retain_dir_structure && die "testcase $TESTCASE_NO failed"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

echo
TESTCASE_NO=53
echo "------------------- f2s testcase $TESTCASE_NO >>> retain_dir_structure-------------------"
$SLOW5_EXEC f2s $FAST5_DIR/retain_dir_structure -d $OUTPUT_DIR/retain_dir_structure --retain || die "testcase $TESTCASE_NO failed"
diff $EXP_SLOW5_DIR/retain_dir_structure  $OUTPUT_DIR/retain_dir_structure > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

TESTCASE_NO=54
echo "------------------- f2s testcase $TESTCASE_NO >>> end_reason datatype is uint8_t-------------------"
mkdir -p $OUTPUT_DIR/end_reason_fast5 || die "creating $OUTPUT_DIR/end_reason_fast5 failed"
$SLOW5_EXEC f2s $FAST5_DIR/end_reason_fast5/end_reason_datatype_uint8_t.fast5 -o $OUTPUT_DIR/end_reason_fast5/end_reason_datatype_uint8_t.slow5 || die "testcase $TESTCASE_NO failed"
diff -q $EXP_SLOW5_DIR/end_reason_fast5/end_reason_datatype_uint8_t.slow5 $OUTPUT_DIR/end_reason_fast5/end_reason_datatype_uint8_t.slow5 > /dev/null || die "ERROR: diff failed f2s_test testcase $TESTCASE_NO for end_reason fast5"
echo -e "${GREEN}testcase $TESTCASE_NO passed${NC}" 1>&3 2>&4

rm -r $OUTPUT_DIR || die "Removing $OUTPUT_DIR failed"

exit 0
