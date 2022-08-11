#!/bin/bash

# An example script that properly converts a set of multi-fast5 generated
# using ONT's multi_to_single_fast5 utility.
# ONT's multi_to_single_fast5 packs reads with mutltiple run_ids in to an individual multi-fast5 file
# slow5tools f2s does directly support converting such files and this script is an example of how to do it.
# First this script uses ONT's single_to_multi_fast5 utility to convert multi-fast5 back to single-fast5 files
# Then we classify the single-fast5 files into multiple directories based on the run_id
# Then we use slow5tools f2s to convert each of those directory separately to blow5
# Next we merge all the blow5 files into a single blow5 file
# Finally we do a sanity check to see if number of records in the blow5 file matches the number of records in the original multi-fast5
# as well as a slow5tools index to see if all the read IDS in the merged BLOW5 file are unique

# Dependencies
# - multi_to_single_fast5 in ont_fast5_api
# - h5dump
# - parallel
# - slow5tools


set -e

FAST5_DIR=$1
TMP_FAST5=tmp_fast5
SINGLE_FAST5=tmp_single_fast5
TMP_BLOW5=tmp_blow5
MERGED_BLOW5=reads.blow5
[ -z ${SLOW5TOOLS} ] && SLOW5TOOLS=slow5tools
[ -z ${NUM_THREADS} ] && NUM_THREADS=$(nproc)

# terminate script
die() {
	echo "$1" >&2
	echo
	exit 1
}

if [ "$#" -ne 1 ]; then
    die "Usage: $0 <fast5_dir>"
fi

test -d $FAST5_DIR || die "Directory $FAST5_DIR does not exist"
test -d $SINGLE_FAST5 && die "Directory $SINGLE_FAST5 already exists. Please delete that first."
test -d $TMP_FAST5 && die "Directory $TMP_FAST5 already exists. Please delete that first."
test -d $TMP_BLOW5 && die "Directory $TMP_BLOW5 already exists. Please delete that first."
test -e $MERGED_BLOW5 && die "$MERGED_BLOW5 already exists. Please delete that first."

multi_to_single_fast5 --version || die "Could not find single_to_multi_fast5. Check if ont_fast5_api [https://github.com/nanoporetech/ont_fast5_api] is installed"
h5dump --version || die "Could not find h5dump. Install using  sudo apt-get install hdf5-tools"
parallel --version || die "Could not find command 'parallel'. On Ubuntu, install using  sudo apt-get install parallel"
$SLOW5TOOLS --version || die "Could not find $SLOW5TOOLS. Add slow5tools to PATH or set SLOW5TOOLS environment variable"

mkdir $TMP_FAST5 $TMP_BLOW5 || die "Creating directory $TMP_FAST5 and $TMP_BLOW5 failed"

multi_to_single_fast5 -i $FAST5_DIR -s $SINGLE_FAST5 --recursive -t $NUM_THREADS || die "Converting multi-fast5 to single-fast5 failed"

read_id_based_classify_func(){

	file=$1
	TMP_FAST5=$2
	# while read file;
	# do
        #RUN_ID=$(strings $file | grep -w -A1 "run_id" | tail -1)
        RUN_ID=$(h5dump -A $file  | grep -w "run_id" -A20 | grep "(0):" | head -1 | awk  '{print $NF}' | tr -d '"')
		#echo "file $file has runID $RUN_ID"
		test -z $RUN_ID && { echo "Could not deduce run_id in $file"; exit 1; }

		mkdir -p $TMP_FAST5/$RUN_ID
		mv -i $file $TMP_FAST5/$RUN_ID || { echo "moving $file to $TMP_FAST5/$RUN_ID failed"; exit 1;}
	# done

}
export -f read_id_based_classify_func

#classify
find $SINGLE_FAST5 -name '*.fast5' | parallel -I% --max-args 1 read_id_based_classify_func % $TMP_FAST5 || die "Classifying single-fast5 based on read ID failed"

#convert
for each in $TMP_FAST5/*; do
    echo "Converting $each";
	DIR=$(basename $each)
    slow5tools f2s $TMP_FAST5/$DIR/ -d $TMP_BLOW5/$DIR -p $NUM_THREADS || die "Converting $each to BLOW5 failed"
done

#merge
slow5tools merge $TMP_BLOW5/ -o $MERGED_BLOW5 -t $NUM_THREADS -K 1000 || die "Merging to a single BLOW5 failed"

#sanity check
NUM_SLOW5_READS=$(slow5tools stats $MERGED_BLOW5 | grep "number of records" | awk '{print $NF}')
NUM_READS=$(find $FAST5_DIR -name '*.fast5' | parallel -I% --max-args 1 strings % | grep "read_.*-.*-.*" | wc -l | awk 'BEGIN {count=0;} {count=count+$0} END {print count;}')

if [ ${NUM_READS} -ne ${NUM_SLOW5_READS} ]
then
	echo "WARNING: Sanity check failed. $NUM_READS in FAST5, but $NUM_SLOW5_READS reads in SLOW5"
	echo "Trying fallback method of checking with multi_to_single_fast5 file count"
	NUM_READS_SINGLE=$(find $TMP_FAST5 -name '*.fast5' | wc -l)
	if [ ${NUM_READS_SINGLE} -ne ${NUM_SLOW5_READS} ]
	then
		echo "ERROR: Sanity check failed. $NUM_READS_SINGLE in TMP_FAST5, but $NUM_SLOW5_READS reads in SLOW5"
		exit 1
	fi
	echo "$NUM_READS_SINGLE in TMP_FAST5, $NUM_SLOW5_READS reads in SLOW5"
else
	echo "$NUM_READS in FAST5, $NUM_SLOW5_READS reads in SLOW5"
fi

#check if unique read id
slow5tools index $MERGED_BLOW5 || die "Indexing BLOW5 failed"

rm -r $SINGLE_FAST5 $TMP_FAST5 $TMP_BLOW5 || die "Removing temporary directories failed"