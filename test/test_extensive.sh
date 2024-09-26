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

Usage="test_extensive.sh"

set -e
set -x

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

# terminate script
die() {
    echo -e "${RED}$1${NC}" >&2
    echo
    exit 1
}

TMP_DIR=/data/slow5-testdata/tmp/
test -d $TMP_DIR && rm -r $TMP_DIR
rm -f *.log
guppy_basecaller --version > /dev/null || die "guppy_basecaller not in path"

echo "*******************************NA12878_prom_subsubsample**************************************"
DATA_NA12878_SUBSUB=/data/slow5-testdata/NA12878_prom_subsubsample
test -d $DATA_NA12878_SUBSUB || die "ERROR: $DATA_NA12878_SUBSUB not found. Download from https://slow5.bioinf.science/na12878_prom_subsub and extract"
mkdir $TMP_DIR || die "Creating $TMP_DIR failed"
test/test_with_guppy.sh $DATA_NA12878_SUBSUB/fast5 $TMP_DIR ./slow5tools guppy_basecaller &> test_s2f_with_guppy_subsub.log || die "test_s2f_with_guppy failed"
rm -r $TMP_DIR
echo "Guppy test passed yey!"
echo ""

echo "********************************NA12878_prom_subsample****************************************"
DATA_NA12878=/data/slow5-testdata/NA12878_prom_subsample
test -d $DATA_NA12878 || die "ERROR: $DATA_NA12878 not found. Download from https://slow5.bioinf.science/na12878_prom_sub and extract"
mkdir $TMP_DIR || die "Creating $TMP_DIR failed"
test/test_with_guppy.sh $DATA_NA12878/fast5 $TMP_DIR ./slow5tools guppy_basecaller &> test_s2f_with_guppy_sub.log || die "test_s2f_with_guppy failed"
rm -r $TMP_DIR
echo "Guppy test passed yey!"
mkdir $TMP_DIR || die "Creating $TMP_DIR failed"
test/test_f2s_s2f_integrity.sh $DATA_NA12878/fast5 $TMP_DIR &> f2s_s2f_integrity_test.txt || die "f2s_s2f_integrity_test failed"
rm -r $TMP_DIR
echo "test_f2s_s2f_integrity passed!"
echo ""

echo "**************************************fast5-soup**********************************************"
DATA_MISC=/data/slow5-testdata/fast5-soup/
test -d $DATA_MISC || die "ERROR: $DATA_MISC not found. Download from https://slow5.bioinf.science/fast5-soup and extract"
mkdir $TMP_DIR || die "Creating $TMP_DIR failed"
test/test_with_guppy.sh $DATA_MISC $TMP_DIR ./slow5tools guppy_basecaller &> test_s2f_with_guppy_soup.log || die "test_s2f_with_guppy failed for fast5 soup"
rm -r $TMP_DIR
echo "Guppy test passed yey!"
# mkdir -p $TMP_DIR/slow5_tmp $TMP_DIR/tmp || die "Creating $TMP_DIR failed"
# ./slow5tools f2s $DATA_MISC -d $TMP_DIR/slow5_tmp -p40
# test/merge_split_integrity_test.sh $TMP_DIR/slow5_tmp $TMP_DIR/tmp ./slow5tools || die "merge_split_integrity_test failed"
# rm -r $TMP_DIR

echo "all done!"
exit 0
