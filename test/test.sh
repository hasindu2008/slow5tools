#!/bin/sh

if [ "$1" = "mem" ]; then
    mem=1
else
    mem=0
fi

echo_test() {
    printf "\n--%s--\n" "$1"
}

ex() {
    if [ $mem -eq 1 ]; then
        valgrind "$@"
    else
        "$@"
    fi
}

fail() {
    echo "FAILURE"
    ret=1
}

not_compiled() {
    echo "NOT COMPILED"
    ret=1
}

ret=0

echo_test "cli test"
if ! ex ./slow5tools f2s test/data/raw/chr22_meth_example-subset-multi > /dev/null; then
    fail
fi

echo_test "endian test"
if gcc -Wall test/endian_test.c -o test/endian_test; then
    ex test/endian_test
else
    not_compiled
fi


echo_test "unit test"
if gcc -Wall -Werror -g test/unit_test.c -o test/unit_test src/slow5.c src/misc.c src/slow5idx_clean.c src/press.c -I src/ -lz; then
    if ! ex test/unit_test > test/unit_test_out.txt; then
        fail
    fi
else
    not_compiled
fi

exit $ret
