#!/bin/bash

# customize tests


# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 



FAST5_DIR= 		#should be absolute or relative to the slow5tools executable
TEMP_DIR= 		#should be absolute or relative to the slow5tools executable
SLOW5TOOLS=
TEST_DATASET_LINK="https://cloudstor.aarnet.edu.au/plus/s/9afW5kgWy1w8ZpQ/download"


./$REL_PATH/download_test_dataset.sh $FAST5_DIR $TEST_DATASET_LINK

./$REL_PATH/integrity_test.sh $FAST5_DIR $TEMP_DIR $SLOW5TOOLS

./$REL_PATH/test_merge_threads.sh $FAST5_DIR $TEMP_DIR $SLOW5TOOLS