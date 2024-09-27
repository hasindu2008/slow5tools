#!/bin/bash

# MIT License

# Copyright (c) 2020 Hiruna Samarakoon
# Copyright (c) 2020 Sasha Jenner
# Copyright (c) 2020,2023 Hasindu Gamaarachchi

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

###############################################################################

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

ex_fail() {
    if  "$@" 2> /dev/null; then
        fail "$@"
    else
        pass=$((pass+1))
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


for type in "lossy" "lossless"
do

    ####### One fast5 automatic file format detection using extension #####

    ####### from slow5 ASCII to various
    # slow5 ASCII-> blow5 zlib
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" -s none -o "$OUT/one_fast5/out_1_${type}_zlib.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib.blow5" "$OUT/one_fast5/out_1_${type}_zlib.blow5" -q
    # slow5 ASCII-> blow5 binary
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" -s none  -o "$OUT/one_fast5/out_1_${type}.blow5" -c none
    my_diff "$EXP/one_fast5/exp_1_${type}.blow5" "$OUT/one_fast5/out_1_${type}.blow5" -q
    # slow5 ASCII-> slow5 ASCII
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" -o "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q
if [ -z "$bigend" ]; then
    # slow5 ASCII-> blow5 zlib-svb
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" -s svb-zd -o "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5" -q
    # slow5 ASCII-> blow5 zlib-ex-zd
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" -s ex-zd -o "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_ex_zd.blow5" "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5" -q
fi
    ####### from blow5 binary to various
    # blow5 binary -> blow5 zlib
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" -s none -o "$OUT/one_fast5/out_1_${type}_zlib.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib.blow5" "$OUT/one_fast5/out_1_${type}_zlib.blow5" -q
    # blow5 binary -> blow5 binary
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" -s none -o "$OUT/one_fast5/out_1_${type}.blow5" -c none
    my_diff "$EXP/one_fast5/exp_1_${type}.blow5" "$OUT/one_fast5/out_1_${type}.blow5" -q
    # blow5 binary -> slow5 ASCII
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" -o "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q
if [ -z "$bigend" ]; then
    # blow5 binary -> blow5 zlib-svb
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" -s svb-zd -o "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5" -q
    # blow5 binary -> blow5 zlib-ex-zd
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" -s ex-zd -o "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_ex_zd.blow5" "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5" -q
fi
    ####### from blow5 zlib to various
    # blow5 zlib -> blow5 zlib
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" -s none  -o "$OUT/one_fast5/out_1_${type}_zlib.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib.blow5" "$OUT/one_fast5/out_1_${type}_zlib.blow5" -q
    # blow5 zlib -> blow5 none
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" -c none -s none -o "$OUT/one_fast5/out_1_${type}.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.blow5" "$OUT/one_fast5/out_1_${type}.blow5" -q
    # blow5 zlib -> slow5 ASCII
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" -o "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q

if [ -z "$bigend" ]; then
    # blow5 zlib -> blow5 zlib-svb
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" -s svb-zd -o "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5" -q

    ####### from blow5 zlib-svb to various
    # blow5 zlib-svb  -> slow5 ASCII
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" -o "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_v0.2.0.slow5" "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5" -q
    # blow5 zlib-svb  -> blow5 binary
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" -c none -s none -o "$OUT/one_fast5/out_1_${type}_v0.2.0.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_v0.2.0.blow5" -q
    # blow5 zlib-svb  -> blow5 zlib
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" -s none -o "$OUT/one_fast5/out_1_${type}_zlib_v0.2.0.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zlib_v0.2.0.blow5" -q
    # blow5 zlib-svb  -> blow5 zlib-svb
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" -s svb-zd -o "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5" -q
    # blow5 zlib-svb  -> blow5 zlib-ex-zd
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" -s ex-zd -o "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_ex_zd.blow5" "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5" -q

    ####### from blow5 zlib-ex-zd to various
    # blow5 zlib-ex-zd  -> slow5 ASCII
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_ex_zd.blow5" -o "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_v0.2.0.slow5" "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5" -q
    # blow5 zlib-ex-zd  -> zlib-ex-zd
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_ex_zd.blow5" -s ex-zd -o "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_ex_zd.blow5" "$OUT/one_fast5/out_1_${type}_zlib_ex_zd.blow5" -q

fi

    ####### One fast5 --to and stdout redirection #####

    ####### from slow5 ASCII to various
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" --to blow5  -s none > "$OUT/one_fast5/out_1_${type}_zlib.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib.blow5" "$OUT/one_fast5/out_1_${type}_zlib.blow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" --to blow5 -c none -s none  > "$OUT/one_fast5/out_1_${type}.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.blow5" "$OUT/one_fast5/out_1_${type}.blow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" --to slow5 > "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" > "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q
if [ -z "$bigend" ]; then
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" --to blow5 -s svb-zd > "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zlib_svb_v0.2.0.blow5" -q
fi
    ####### from blow5 binary to various
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" --to blow5  -s none > "$OUT/one_fast5/out_1_${type}_zlib.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib.blow5" "$OUT/one_fast5/out_1_${type}_zlib.blow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" --to blow5 -c none  -s none > "$OUT/one_fast5/out_1_${type}.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.blow5" "$OUT/one_fast5/out_1_${type}.blow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" --to slow5 > "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.blow5" > "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q

    ####### from slow5 zlib to various
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" --to blow5  -s none > "$OUT/one_fast5/out_1_${type}_zlib.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}_zlib.blow5" "$OUT/one_fast5/out_1_${type}_zlib.blow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" --to blow5 -c none  -s none > "$OUT/one_fast5/out_1_${type}.blow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.blow5" "$OUT/one_fast5/out_1_${type}.blow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" --to slow5 > "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q
    ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" > "$OUT/one_fast5/out_1_${type}.slow5"
    my_diff "$EXP/one_fast5/exp_1_${type}.slow5" "$OUT/one_fast5/out_1_${type}.slow5" -q

    ######## selective zstd tests
    if [ "$zstd" = "1" ]; then
        # # slow5 ASCII -> blow5 zstd-svb
        # ex "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" -c zstd -s svb-zd -o "$OUT/one_fast5/out_1_${type}_zstd_svb_v0.2.0.blow5"
        # my_diff "$EXP/one_fast5/exp_1_${type}_zstd_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zstd_svb_v0.2.0.blow5" -q

        # # blow5 zlib -> blow5 zstd-svb
        # ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib.blow5" -c zstd -s svb-zd -o "$OUT/one_fast5/out_1_${type}_zstd_svb_v0.2.0.blow5"
        # my_diff "$EXP/one_fast5/exp_1_${type}_zstd_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zstd_svb_v0.2.0.blow5" -q

        # # blow5 zlib-svb -> blow5 zstd-svb
        # ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zlib_svb_v0.2.0.blow5" -c zstd -s svb-zd -o "$OUT/one_fast5/out_1_${type}_zstd_svb_v0.2.0.blow5"
        # my_diff "$EXP/one_fast5/exp_1_${type}_zstd_svb_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zstd_svb_v0.2.0.blow5" -q

    if [ -z "$bigend" ]; then
        # blow5 zstd-svb ->  slow5 ASCII
        ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zstd_svb_v0.2.0.blow5" -o "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5"
        my_diff "$EXP/one_fast5/exp_1_${type}_v0.2.0.slow5" "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5" -q
        # blow5 zstd-svb -> blow5 none
        ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zstd_svb_v0.2.0.blow5" -c none -s none -o "$OUT/one_fast5/out_1_${type}_v0.2.0.blow5"
        my_diff "$EXP/one_fast5/exp_1_${type}_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_v0.2.0.blow5" -q
        # blow5 zstd ->  slow5 ASCII
        ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zstd_v0.2.0.blow5" -o "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5"
        my_diff "$EXP/one_fast5/exp_1_${type}_v0.2.0.slow5" "$OUT/one_fast5/out_1_${type}_v0.2.0.slow5" -q
        # blow5 zstd ->  blow5 zlib
        ex "$S5T" view "$EXP/one_fast5/exp_1_${type}_zstd_v0.2.0.blow5" -s none -o "$OUT/one_fast5/out_1_${type}_zlib_v0.2.0.blow5"
        my_diff "$EXP/one_fast5/exp_1_${type}_zlib_v0.2.0.blow5" "$OUT/one_fast5/out_1_${type}_zlib_v0.2.0.blow5" -q
    fi
    fi
done

# the following should exit with error

#conflict in --to format and -o format
ex_fail "$S5T" view "$EXP/one_fast5/exp_1_lossless.slow5" --to slow5 -o $OUT/one_fast5/fail.blow5
#if the requested compression does not exist, must exit with error
if [ "$zstd" != "1" ]; then
    ex_fail "$S5T" view "$EXP/one_fast5/exp_1_${type}.slow5" --to blow5 -c zstd -o $OUT/one_fast5/fail.blow5
fi



echo "$pass/$tot"

exit $ret
