#!/bin/bash

# @author: Hiruna Samarakoon (hirunas@eng.pdn.ac.lk)

###############################################################################

# download a test dataset and run test_s2f_with_guppy.sh

Usage="test_all.sh [link optional]"

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

SLOW5TOOLS=./slow5tools
GUPPY_BASECALLER=guppy_basecaller

# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/"
TEST_DIR=$REL_PATH/test_all_dir
FAST5_DIR=$TEST_DIR/fast5_files
LINK=https://cloudstor.aarnet.edu.au/plus/s/9afW5kgWy1w8ZpQ/download
TARBALL=download_dataset

test -d $TEST_DIR && rm -r "$TEST_DIR"
mkdir "$TEST_DIR" || exit 1

test -d $FAST5_DIR && rm -r $FAST5_DIR
mkdir "$FAST5_DIR" || exit 1
wget -O $TARBALL $LINK || curl -o $TARBALL $LINK || die "Downloading dataset from $LINK failed."
tar -xf $TARBALL -C $FAST5_DIR || die "Extracting $TARBALL failed"
rm $TARBALL

$REL_PATH/test_s2f_with_guppy.sh $FAST5_DIR $TEST_DIR $SLOW5TOOLS guppy_basecaller || exit 1
$REL_PATH/f2s_s2f_integrity_test.sh $FAST5_DIR $TEST_DIR || exit 1

exit 0
