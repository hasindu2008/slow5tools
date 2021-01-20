#!/bin/bash

./slow5tools f2s test/data/raw/chr22_meth_example-subset-multi > /dev/null || exit 1

#gcc -Wall test/unit_test.c -o test/unit_test -I src/ && echo "compiled"
#test/unit_test && echo "success"
