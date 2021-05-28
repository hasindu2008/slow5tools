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

prep_cli() {
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

prep_cli

echo_test 'cli test'
if [ $mem -eq 1 ]; then
    if ! ./test/test_cli.sh mem; then
        fail
    fi
else
    if ! ./test/test_cli.sh; then
        fail
    fi
fi

exit $ret
