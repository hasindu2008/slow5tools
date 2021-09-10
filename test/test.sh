#!/bin/sh

FIRST_FAILED_SET_OF_TESTCASES="NOT SET"

if [ "$1" = 'mem' ]; then
    mem=1
else
    mem=0
fi

echo_test() {
    printf '\n--%s--\n' "$1"
}

ex() {
    if [ $mem -eq 1 ]; then
        valgrind --leak-check=full --error-exitcode=1 "$@"
    else
        "$@"
    fi
}

fail() {
    echo 'FAILURE'
    ret=1
    FIRST_FAILED_SET_OF_TESTCASES=$1
}

not_compiled() {
    echo 'NOT COMPILED'
    ret=1
}

my_diff() {
    if ! diff "$1" "$2" -q; then
        fail "my diff"
    fi
}

prep() {
    mkdir -p 'test/bin'
    mkdir -p 'test/data/out/one_fast5'
    mkdir -p 'test/data/out/two_rg'
    mkdir -p 'test/data/out/aux_array'
}

prep_unit() {
    rm test/data/out/*.idx
    rm test/data/out/one_fast5/*
    rm test/data/out/two_rg/*
    rm test/data/out/aux_array/*
    rm test/data/exp/one_fast5/*.idx
    rm test/data/exp/two_rg/*.idx
    rm test/data/exp/aux_array/*.idx
    rm test/data/test/*.idx
    rm test/data/err/*.idx

    cp 'test/data/exp/one_fast5/exp_1_default.slow5' 'test/data/out/exp_1_default_add_empty.slow5'
    cp 'test/data/exp/one_fast5/exp_1_default.slow5' 'test/data/out/exp_1_default_add_valid.slow5'
    cp 'test/data/exp/one_fast5/exp_1_default.slow5' 'test/data/out/exp_1_default_add_duplicate.slow5'
}

prep_view() {
    rm test/data/out/one_fast5/*
}

ret=0

prep

TESTCASE_NAME="endian test"
echo_test $TESTCASE_NAME
if gcc -Wall test/endian_test.c -o test/bin/endian_test; then
    ex test/bin/endian_test
else
    not_compiled
fi

prep_view

TESTCASE_NAME="view test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_view.sh mem; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_view.sh; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="f2s test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_f2s.sh mem; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_f2s.sh; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="merge test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_merge.sh mem; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_merge.sh; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="split integrity test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_split.sh mem; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_split.sh; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="f2s_s2f integrity test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_f2s_s2f_integrity.sh mem; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_f2s_s2f_integrity.sh; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="index test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_index.sh mem; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_index.sh; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="get test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_get.sh mem; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_get.sh; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="f2s_view_diff_test"
echo_test $TESTCASE_NAME
FAST5_FILE=./test/data/raw/f2s_view_diff/sss_median_before_edited.fast5
OUTPUT_DIR=./test/data/out/f2s_view_diff
INPUT_ARGS="$FAST5_FILE $OUTPUT_DIR"
if [ $mem -eq 1 ]; then
    if ! ./test/test_f2s_view_diff.sh $INPUT_ARGS mem ; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_f2s_view_diff.sh $INPUT_ARGS ; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="cat test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_cat.sh mem ; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_cat.sh ; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="stats test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_stats.sh mem ; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_stats.sh ; then
        fail "$TESTCASE_NAME"
    fi
fi

TESTCASE_NAME="quickcheck test"
echo_test $TESTCASE_NAME
if [ $mem -eq 1 ]; then
    if ! ./test/test_quickcheck.sh mem ; then
        fail "$TESTCASE_NAME"
    fi
else
    if ! ./test/test_quickcheck.sh ; then
        fail "$TESTCASE_NAME"
    fi
fi

if [ $ret -eq 1 ]; then
  echo ">>>>>One or more test cases have failed. The first failed set of testcases is $FIRST_FAILED_SET_OF_TESTCASES<<<<<"
fi

exit $ret
