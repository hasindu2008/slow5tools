#!/bin/bash

set -e

FAST5_DIR=$1
TMP_FAST5=tmp_fast5
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
test -d $TMP_FAST5 && die "Directory $TMP_FAST5 already exists. Please delete that first."
test -d $TMP_BLOW5 && die "Directory $TMP_BLOW5 already exists. Please delete that first."
test -e $MERGED_BLOW5 && die "$MERGED_BLOW5 already exists. Please delete that first."

parallel --version || die "Could not find command 'parallel'. On Ubuntu, install using  sudo apt-get install parallel"
$SLOW5TOOLS --version || die "Could not find $SLOW5TOOLS. Add slow5tools to PATH or set SLOW5TOOLS environment variable"
h5dump --version || die "Could not find h5dump. Install using  sudo apt-get install hdf5-tools"

mkdir $TMP_FAST5 $TMP_BLOW5 || die "Creating directory $TMP_FAST5 and $TMP_BLOW5 failed"

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
		cp -i $file $TMP_FAST5/$RUN_ID || { echo "copying $file to $TMP_FAST5/$RUN_ID failed"; exit 1;}
	# done

}
export -f read_id_based_classify_func

#classify
find $FAST5_DIR -name '*.fast5' | parallel -I% --max-args 1 read_id_based_classify_func % $TMP_FAST5 || die "Classifying single-fast5 based on read ID failed"

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
NUM_READS_SINGLE=$(find $FAST5_DIR -name '*.fast5' | wc -l)
if [ ${NUM_READS_SINGLE} -ne ${NUM_SLOW5_READS} ]
then
    echo "ERROR: Sanity check failed. $NUM_READS_SINGLE in TMP_FAST5, but $NUM_SLOW5_READS reads in SLOW5"
    exit 1
fi
echo "$NUM_READS_SINGLE in FAST5, $NUM_SLOW5_READS reads in SLOW5"

#check if unique read id
slow5tools index $MERGED_BLOW5 || die "Indexing BLOW5 failed"

rm -r $TMP_FAST5 $TMP_BLOW5 || die "Removing temporary directories failed"

# list=$(ls -1 | sed 's/_ch.*.fast5//g' | sort -u)
# for each in $list; do
#     echo $each; mkdir $TMP_FAST5/$each;
#     mv $each* $TMP_FAST5/$each/
#     slow5tools f2s $TMP_FAST5/$each/ -d $TMP_BLOW5/$each
# done

# slow5tools merge $TMP_BLOW5/ -o $MERGED_BLOW5
