#!/bin/bash

# Testing output of slow5tools

S5T='./slow5tools'
EXP='test/data/exp'
OUT='test/data/out'

if [ "$1" = 'mem' ]; then
    mem=1
else
    mem=0
fi

ex() {
    if [ $mem -eq 1 ]; then
        if ! valgrind --leak-check=full --error-exitcode=1 "$@"; then
            fail "$@"
        else
            pass=$((pass+1))
        fi
    else
        if ! "$@" 2> /dev/null; then
            fail "$@"
        else
            pass=$((pass+1))
        fi
    fi
    tot=$((tot+1))
}

fail() {
    echo 'failed:' "$@"
    ret=1
}

my_diff() {
    if ! diff "$1" "$2" -q; then
        fail "diff $1 $2"
    fi
}

ret=0
tot=0
pass=0

## One fast5 default
# All slow5 -> all slow5
ex "$S5T" view "$EXP/one_fast5/exp_1_default.slow5" -o "$OUT/one_fast5/out_1_default_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_default_zlib.blow5" "$OUT/one_fast5/out_1_default_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.slow5" -o "$OUT/one_fast5/out_1_default.blow5" -c none
my_diff "$EXP/one_fast5/exp_1_default.blow5" "$OUT/one_fast5/out_1_default.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.slow5" -o "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q

ex "$S5T" view "$EXP/one_fast5/exp_1_default.blow5" -o "$OUT/one_fast5/out_1_default_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_default_zlib.blow5" "$OUT/one_fast5/out_1_default_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.blow5" -o "$OUT/one_fast5/out_1_default.blow5" -c none
my_diff "$EXP/one_fast5/exp_1_default.blow5" "$OUT/one_fast5/out_1_default.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.blow5" -o "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q

ex "$S5T" view "$EXP/one_fast5/exp_1_default_zlib.blow5" -o "$OUT/one_fast5/out_1_default_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_default_zlib.blow5" "$OUT/one_fast5/out_1_default_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default_zlib.blow5" -o "$OUT/one_fast5/out_1_default.blow5" -c none
my_diff "$EXP/one_fast5/exp_1_default.blow5" "$OUT/one_fast5/out_1_default.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default_zlib.blow5" -o "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q

# All slow5 -> all slow5 different options
ex "$S5T" view "$EXP/one_fast5/exp_1_default.slow5" --to blow5 > "$OUT/one_fast5/out_1_default_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_default_zlib.blow5" "$OUT/one_fast5/out_1_default_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.slow5" --to blow5 -c none > "$OUT/one_fast5/out_1_default.blow5"
my_diff "$EXP/one_fast5/exp_1_default.blow5" "$OUT/one_fast5/out_1_default.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.slow5" --to slow5 > "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.slow5" > "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q

ex "$S5T" view "$EXP/one_fast5/exp_1_default.blow5" --to blow5 > "$OUT/one_fast5/out_1_default_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_default_zlib.blow5" "$OUT/one_fast5/out_1_default_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.blow5" --to blow5 -c none > "$OUT/one_fast5/out_1_default.blow5"
my_diff "$EXP/one_fast5/exp_1_default.blow5" "$OUT/one_fast5/out_1_default.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.blow5" --to slow5 > "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default.blow5" > "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q

ex "$S5T" view "$EXP/one_fast5/exp_1_default_zlib.blow5" --to blow5 > "$OUT/one_fast5/out_1_default_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_default_zlib.blow5" "$OUT/one_fast5/out_1_default_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default_zlib.blow5" --to blow5 -c none > "$OUT/one_fast5/out_1_default.blow5"
my_diff "$EXP/one_fast5/exp_1_default.blow5" "$OUT/one_fast5/out_1_default.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default_zlib.blow5" --to slow5 > "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_default_zlib.blow5" > "$OUT/one_fast5/out_1_default.slow5"
my_diff "$EXP/one_fast5/exp_1_default.slow5" "$OUT/one_fast5/out_1_default.slow5" -q

## One fast5 lossless
# All slow5 -> all slow5
ex "$S5T" view "$EXP/one_fast5/exp_1_lossless.slow5" -o "$OUT/one_fast5/out_1_lossless_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_lossless_zlib.blow5" "$OUT/one_fast5/out_1_lossless_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_lossless.slow5" -o "$OUT/one_fast5/out_1_lossless.blow5" -c none
my_diff "$EXP/one_fast5/exp_1_lossless.blow5" "$OUT/one_fast5/out_1_lossless.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_lossless.slow5" -o "$OUT/one_fast5/out_1_lossless.slow5"
my_diff "$EXP/one_fast5/exp_1_lossless.slow5" "$OUT/one_fast5/out_1_lossless.slow5" -q

ex "$S5T" view "$EXP/one_fast5/exp_1_lossless.blow5" -o "$OUT/one_fast5/out_1_lossless_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_lossless_zlib.blow5" "$OUT/one_fast5/out_1_lossless_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_lossless.blow5" -o "$OUT/one_fast5/out_1_lossless.blow5" -c none
my_diff "$EXP/one_fast5/exp_1_lossless.blow5" "$OUT/one_fast5/out_1_lossless.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_lossless.blow5" -o "$OUT/one_fast5/out_1_lossless.slow5"
my_diff "$EXP/one_fast5/exp_1_lossless.slow5" "$OUT/one_fast5/out_1_lossless.slow5" -q

ex "$S5T" view "$EXP/one_fast5/exp_1_lossless_zlib.blow5" -o "$OUT/one_fast5/out_1_lossless_zlib.blow5"
my_diff "$EXP/one_fast5/exp_1_lossless_zlib.blow5" "$OUT/one_fast5/out_1_lossless_zlib.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_lossless_zlib.blow5" -o "$OUT/one_fast5/out_1_lossless.blow5" -c none
my_diff "$EXP/one_fast5/exp_1_lossless.blow5" "$OUT/one_fast5/out_1_lossless.blow5" -q
ex "$S5T" view "$EXP/one_fast5/exp_1_lossless_zlib.blow5" -o "$OUT/one_fast5/out_1_lossless.slow5"
my_diff "$EXP/one_fast5/exp_1_lossless.slow5" "$OUT/one_fast5/out_1_lossless.slow5" -q

echo "$pass/$tot"

exit $ret
