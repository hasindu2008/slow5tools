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
        if ! valgrind --leak-check=full --error-exitcode=1 "$@"; then
            fail
        fi
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
    if ! ex diff "$1" "$2" -q; then
        fail
    fi
}

prep() {
    rm 'test/data/exp/one_fast5/exp_1_default.slow5.idx'
    rm 'test/data/err/parse_bad.slow5.idx'
    cp 'test/data/exp/one_fast5/exp_1_default.slow5' 'test/data/out/exp_1_default_add_empty.slow5'
    cp 'test/data/exp/one_fast5/exp_1_default.slow5' 'test/data/out/exp_1_default_add_valid.slow5'
    cp 'test/data/exp/one_fast5/exp_1_default.slow5' 'test/data/out/exp_1_default_add_duplicate.slow5'

    rm 'test/data/exp/one_fast5/exp_1_default.blow5.idx'
    rm 'test/data/exp/one_fast5/exp_1_default_gzip.blow5.idx'

    rm 'test/data/out/one_fast5/slow5_to_blow5_uncomp.blow5'
    rm 'test/data/out/one_fast5/slow5_to_blow5_gzip.blow5'
    rm 'test/data/out/one_fast5/blow5_uncomp_to_slow5.slow5'
    rm 'test/data/out/one_fast5/blow5_gzip_to_slow5.slow5'
    rm 'test/data/out/one_fast5/blow5_gzip_to_blow5_uncomp.blow5'
    rm 'test/data/out/one_fast5/blow5_uncomp_to_blow5_gzip.blow5'
}

ret=0

prep

echo_test 'cli test'
if ! ex ./slow5tools f2s test/data/raw/chr22_meth_example-subset-multi > /dev/null; then
    fail
fi

echo_test 'endian test'
if gcc -Wall test/endian_test.c -o test/bin/endian_test; then
    ex test/bin/endian_test
else
    not_compiled
fi

echo_test 'unit test helpers'
if gcc -Wall -Werror -g test/unit_test_helpers.c -o test/bin/unit_test_helpers src/slow5.c src/misc.c src/slow5idx.c src/press.c -I src/ -lz; then
    if ! ex test/bin/unit_test_helpers; then
        fail
    fi
else
    not_compiled
fi

echo_test 'unit test press'
if gcc -Wall -Werror -g test/unit_test_press.c -o test/bin/unit_test_press src/press.c -I src/ -lz; then
    if ! ex test/bin/unit_test_press > test/data/out/unit_test_out_press; then
        fail
    fi
else
    not_compiled
fi

echo_test 'unit test ascii'
if gcc -Wall -Werror -g test/unit_test_ascii.c -o test/bin/unit_test_ascii src/slow5.c src/misc.c src/slow5idx.c src/press.c -I src/ -lz; then
    if ! ex test/bin/unit_test_ascii > test/data/out/unit_test_out_ascii; then
        fail
    fi
else
    not_compiled
fi


echo_test 'unit test binary'
if gcc -Wall -Werror -g test/unit_test_binary.c -o test/bin/unit_test_binary src/slow5.c src/misc.c src/slow5idx.c src/press.c -I src/ -lz; then
    if ! ex test/bin/unit_test_binary > test/data/out/unit_test_out_binary; then
        fail
    fi
else
    not_compiled
fi

echo_test 'slow5 conversion test'
if gcc -Wall -Werror -g test/convert_slow5_test.c -o test/bin/convert_slow5_test src/slow5.c src/misc.c src/slow5idx.c src/press.c -I src/ -lz; then
    if ! ex test/bin/convert_slow5_test; then
        fail
    fi
else
    not_compiled
fi

echo_test 'diff test'
# Unit test output diffs
my_diff 'test/data/out/unit_test_out_ascii' 'test/data/exp/unit_test_exp_ascii'
my_diff 'test/data/out/unit_test_out_fprint' 'test/data/exp/unit_test_exp_fprint'
my_diff 'test/data/out/unit_test_out_binary' 'test/data/exp/unit_test_exp_binary'
my_diff 'test/data/out/unit_test_out_press' 'test/data/exp/unit_test_exp_press'
# Adding records diffs
my_diff 'test/data/out/exp_1_default_add_empty.slow5' 'test/data/exp/exp_1_default_add_empty.slow5'
my_diff 'test/data/out/exp_1_default_add_valid.slow5' 'test/data/exp/exp_1_default_add_valid.slow5'
my_diff 'test/data/out/exp_1_default_add_duplicate.slow5' 'test/data/exp/exp_1_default_add_duplicate.slow5'
# Conversion diffs
my_diff 'test/data/out/one_fast5/slow5_to_blow5_uncomp.blow5' 'test/data/exp/one_fast5/exp_1_default.blow5'
my_diff 'test/data/out/one_fast5/slow5_to_blow5_gzip.blow5' 'test/data/exp/one_fast5/exp_1_default_gzip.blow5'
my_diff 'test/data/out/one_fast5/blow5_uncomp_to_slow5.slow5' 'test/data/exp/one_fast5/exp_1_default.slow5'
my_diff 'test/data/out/one_fast5/blow5_gzip_to_slow5.slow5' 'test/data/exp/one_fast5/exp_1_default.slow5'
my_diff 'test/data/out/one_fast5/blow5_gzip_to_blow5_uncomp.blow5' 'test/data/exp/one_fast5/exp_1_default.blow5'
my_diff 'test/data/out/one_fast5/blow5_uncomp_to_blow5_gzip.blow5' 'test/data/exp/one_fast5/exp_1_default_gzip.blow5'

exit $ret
