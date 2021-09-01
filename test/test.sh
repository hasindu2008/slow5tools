#!/bin/sh

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
}

not_compiled() {
    echo 'NOT COMPILED'
    ret=1
}

my_diff() {
    if ! diff "$1" "$2" -q; then
        fail
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

echo_test 'endian test'
if gcc -Wall test/endian_test.c -o test/bin/endian_test; then
    ex test/bin/endian_test
else
    not_compiled
fi

prep_view

echo_test 'view test'
if [ $mem -eq 1 ]; then
    if ! ./test/test_view.sh mem; then
        fail
    fi
else
    if ! ./test/test_view.sh; then
        fail
    fi
fi

echo_test 'f2s output test'
if [ $mem -eq 1 ]; then
    if ! ./test/f2s_output_test.sh mem; then
        fail
    fi
else
    if ! ./test/f2s_output_test.sh; then
        fail
    fi
fi

echo_test 'merge integrity test'
if [ $mem -eq 1 ]; then
    if ! ./test/merge_integrity_test.sh mem; then
        fail
    fi
else
    if ! ./test/merge_integrity_test.sh; then
        fail
    fi
fi

echo_test 'split integrity test'
if [ $mem -eq 1 ]; then
    if ! ./test/split_integrity_test.sh mem; then
        fail
    fi
else
    if ! ./test/split_integrity_test.sh; then
        fail
    fi
fi

echo_test 'f2s_s2f integrity test'
if [ $mem -eq 1 ]; then
    if ! ./test/f2s_s2f_integrity_test.sh mem; then
        fail
    fi
else
    if ! ./test/f2s_s2f_integrity_test.sh; then
        fail
    fi
fi

echo_test 'index_test'
if [ $mem -eq 1 ]; then
    if ! ./test/index_test.sh mem; then
        fail
    fi
else
    if ! ./test/index_test.sh; then
        fail
    fi
fi

echo_test 'get_test'
if [ $mem -eq 1 ]; then
    if ! ./test/get_test.sh mem; then
        fail
    fi
else
    if ! ./test/get_test.sh; then
        fail
    fi
fi

echo_test 'f2s_view_diff_test'
FAST5_FILE=./test/data/raw/f2s_view_diff/sss_median_before_edited.fast5
OUTPUT_DIR=./test/data/out/f2s_view_diff
INPUT_ARGS="$FAST5_FILE $OUTPUT_DIR"
if [ $mem -eq 1 ]; then
    if ! ./test/f2s_view_diff_test.sh $INPUT_ARGS mem ; then
        fail
    fi
else
    if ! ./test/f2s_view_diff_test.sh $INPUT_ARGS ; then
        fail
    fi
fi

echo_test 'cat_test'
if [ $mem -eq 1 ]; then
    if ! ./test/cat_test.sh mem ; then
        fail
    fi
else
    if ! ./test/cat_test.sh ; then
        fail
    fi
fi

exit $ret
