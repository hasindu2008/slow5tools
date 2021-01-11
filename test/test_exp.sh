#!/bin/sh

# Testing output of slow5tools

SLOW5TOOLS='../slow5tools'

"$SLOW5TOOLS" f2s 'data/raw/one_fast5/fast5_files/exp_1.fast5' > 'data/out/one_fast5/exp_1.slow5.out'
diff 'data/exp/one_fast5/exp_1.slow5' 'data/out/one_fast5/exp_1.slow5.out' -q
