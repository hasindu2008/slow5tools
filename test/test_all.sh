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

# download a test dataset and run test_s2f_with_guppy.sh

Usage="test_all.sh [link optional]"


NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

# terminate script
die() {
    echo -e "${RED}$1${NC}" >&2
    echo
    exit 1
}

SLOW5TOOLS=./slow5tools
GUPPY_BASECALLER=guppy_basecaller

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"
TEST_DIR=$REL_PATH/test_all_dir
FAST5_DIR=$TEST_DIR/fast5_files
LINK=https://cloudstor.aarnet.edu.au/plus/s/9afW5kgWy1w8ZpQ/download
TARBALL=download_dataset

test -d $TEST_DIR && rm -r "$TEST_DIR"
mkdir "$TEST_DIR" || die "Creating $TEST_DIR failed"

test -d $FAST5_DIR && rm -r $FAST5_DIR
mkdir "$FAST5_DIR" || die "Creating $FAST5_DIR failed"

wget -O $TARBALL $LINK || curl -o $TARBALL $LINK || die "Downloading dataset from $LINK failed."
tar -xf $TARBALL -C $FAST5_DIR || die "Extracting $TARBALL failed"
rm $TARBALL

$REL_PATH/test_s2f_with_guppy.sh $FAST5_DIR $TEST_DIR $SLOW5TOOLS guppy_basecaller || die "test_s2f_with_guppy failed"
$REL_PATH/f2s_s2f_integrity_test.sh $FAST5_DIR $TEST_DIR || die "f2s_s2f_integrity_test failed"

rm -r $TEST_DIR || die "Removing $TEST_DIR failed"

exit 0
