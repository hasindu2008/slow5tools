#!/bin/bash

# @author: Hiruna Samarakoon (hirunas@eng.pdn.ac.lk)

###############################################################################

# do f2s and then merge several times with different number of processes and threads

NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'

Usage="multi_process_f2s_threaded_merge.sh [path to fast5 directory] [path to create a temporary directory] [path to slow5tools executable]"

if [[ "$#" -lt 3 ]]; then
	echo "Usage: $Usage"
	exit
fi

FAST5_DIR=$1
TEST_DIR=$2/multi_process_f2s_threaded_merge_$(date +%s)
SLOWTOOLS=$3
F2S_OUTPUT_DIR=$TEST_DIR
MERGED_OUTPUT_DIR=$TEST_DIR
SLOW5_FORMAT=blow5
# set TEMP_DIR to use a faster storage system
TEMP_DIR="-f $TEST_DIR"
IOP=4
LOSSY_F2S=-l
LOSSY_MERGE=-l
EXTRA_FLAGS=""

THREAD_LIST="32 24 16 8 4 2 1" #same as number of processes
# THREAD_LIST="1"


clean_file_system_cache() {
	clean_fscache
	#sync
	#echo 3 | tee /proc/sys/vm/drop_caches
}

LOG="$TEST_DIR/multi_process_f2s_threaded_merge.log"

slow5_f2s_merge_varied_processes_threads () {

	for num in $THREAD_LIST
	do

		num_threads=$num
		MERGED_BLOW5="$MERGED_OUTPUT_DIR/merged_thread_$num_threads.$SLOW5_FORMAT"


		clean_file_system_cache
		echo >> $LOG
		echo "-------------$num_threads threads/processes---------" >> $LOG

		echo "-------------running f2s using $num_threads processes---------"
		echo "-------------running f2s using $num_threads processes---------" >> $LOG
		if ! /usr/bin/time -v $SLOWTOOLS f2s $FAST5_DIR -d $F2S_OUTPUT_DIR --iop $num_threads $LOSSY_F2S $EXTRA_FLAGS 2>>$LOG;then
			echo -e "${RED}FAILED${NC}"
			exit 1
		fi

		# clean_file_system_cache
		echo "-------------running merge using $num_threads threads-------"
		echo "-------------running merge using $num_threads threads-------" >> $LOG
		if ! /usr/bin/time -v $SLOWTOOLS merge $F2S_OUTPUT_DIR $TEMP_DIR/$num $LOSSY_MERGE -t $num_threads -o $MERGED_BLOW5 $EXTRA_FLAGS 2>> $LOG;then
			echo -e "${RED}FAILED${NC}"
			exit 1
		fi

		rm $MERGED_BLOW5 # delete merge file if necessary
		rm $F2S_OUTPUT_DIR/* # delete f2s output
		clean_file_system_cache

	done

}

# create test directory
test -d  $TEST_DIR && rm -r $TEST_DIR
mkdir $TEST_DIR

# --------------------------------------------------------------------------------
echo -e "${GREEN}SLOW5_FORMAT=blow5${NC}"
# f2s .blow5s
SLOW5_FORMAT=blow5
F2S_OUTPUT_DIR=$TEST_DIR/blow5s
test -d  $F2S_OUTPUT_DIR && rm -r $F2S_OUTPUT_DIR
mkdir $F2S_OUTPUT_DIR

MERGED_OUTPUT_DIR=$TEST_DIR/merged_blow5s
test -d  $MERGED_OUTPUT_DIR && rm -r $MERGED_OUTPUT_DIR
mkdir $MERGED_OUTPUT_DIR

echo "-------------SLOW5_FORMAT:$SLOW5_FORMAT---------" >> $LOG
slow5_f2s_merge_varied_processes_threads

# remove f2s or merge output if necessary
rm -r $F2S_OUTPUT_DIR
rm -r $MERGED_OUTPUT_DIR

echo -e "${GREEN}SUCCESS${NC}"
echo

# --------------------------------------------------------------------------------
echo -e "${GREEN}SLOW5_FORMAT=blow5_compressed${NC}"
# f2s .compressed_blow5s
SLOW5_FORMAT=blow5
F2S_OUTPUT_DIR=$TEST_DIR/compressed_blow5s
test -d  $F2S_OUTPUT_DIR && rm -r $F2S_OUTPUT_DIR
mkdir $F2S_OUTPUT_DIR

MERGED_OUTPUT_DIR=$TEST_DIR/merged_compressed_blow5s
test -d  $MERGED_OUTPUT_DIR && rm -r $MERGED_OUTPUT_DIR
mkdir $MERGED_OUTPUT_DIR

echo "-------------SLOW5_FORMAT:$SLOW5_FORMAT_compressed---------" >> $LOG
EXTRA_FLAGS="-c gzip"
slow5_f2s_merge_varied_processes_threads

# remove f2s or merge output if necessary
rm -r $F2S_OUTPUT_DIR
rm -r $MERGED_OUTPUT_DIR

echo -e "${GREEN}SUCCESS${NC}"
echo



# --------------------------------------------------------------------------------
# f2s slow5
echo -e "${GREEN}SLOW5_FORMAT=slow5${NC}"
SLOW5_FORMAT=slow5
F2S_OUTPUT_DIR=$TEST_DIR/slow5s
test -d  $F2S_OUTPUT_DIR && rm -r $F2S_OUTPUT_DIR
mkdir $F2S_OUTPUT_DIR

MERGED_OUTPUT_DIR=$TEST_DIR/merged_compressed_blow5s
test -d  $MERGED_OUTPUT_DIR && rm -r $MERGED_OUTPUT_DIR
mkdir $MERGED_OUTPUT_DIR

echo "-------------SLOW5_FORMAT:$SLOW5_FORMAT---------" >> $LOG
EXTRA_FLAGS="-b slow5"
slow5_f2s_merge_varied_processes_threads

# remove f2s or merge output if necessary
rm -r $F2S_OUTPUT_DIR
rm -r $MERGED_OUTPUT_DIR

echo -e "${GREEN}SUCCESS${NC}"
echo


exit