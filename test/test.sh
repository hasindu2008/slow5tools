#!/bin/bash

cd ../ && make clean && make && make test && cd test

./slow5tools f2s test/data/raw/chr22_meth_example-subset-multi > /dev/null || exit 1
./unit_test
