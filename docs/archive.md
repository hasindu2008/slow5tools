# Paranoid's guide on using slow5 for archiving

BLOW5 is a great alternative to archive raw nanopore signal data, not just for doing fast analysis. You cna save heaptsof space, for instance, recently inhouse data from 2021 which were xxTB is FAST5 reduced to X TB with BLOW5. One concern one might be having is, will my will the integrity of my data be retained? Anwer is yes, BLOW5 conversionis fully lossless, you can convert back to fast5 any time. But still many of you (including us) will be a bit concerned about deleting existing FAST5 after converting to BLOW5, as if a problem is found later, original files would be no longer there. To fix those concerns, this article walks through many possible integrity checks one could do to make sure the converted data is perfect. Note that some of these tests are redundant and someof them are time-consuming, so depending on how paranoid you are, you can pick the tests you want.

We suggest converting all fast5 files for a single sequencing run into a single merged blow5 file beccause then it is easy to manage - this article is centred around that method. This is our personnel preference, you can either have a number of blow5 files per sample or even merge multiple sequencing runs into one BLOW5. But we suggest not trying to merge drastically different runs (e.g., a promethION run with a MinION run, a run done in 2020 with a run in 2022) into one blow5 file for archiving purposes. This is because those FAST5 file structure can vary significantly and it is better not to mix them up. But for analysis purposes, you can merge as you wish as you have original files if something goes wrong.

For a given sequencing run, call slow5tools f2s to convert each fast5 file to blow5. Then merge all SLOW5 into one. Given below is a bash code snippet:

```
DIR_FAST5=/path/to/input/fast5/
DIR_TMP_SLOW5=/path/to/temporary/slow5/
SLOW5_FILE=/path/to/merged.blow5
NUM_THREADS=8

slow5tools f2s ${DIR_FAST5} -d ${DIR_TMP_SLOW5}  -p${NUM_THREADS}
slow5tools merge ${DIR_TMP_SLOW5}/ -o ${SLOW5_FILE} -t${NUM_THREADS}

rm -r ${DIR_TMP_SLOW5}
```

## Read Count based sanity checks

One of the simplest sanity checks is to check if the read counts match. Getting the number of read records in a merged blow5 file is quite simple. A bash command to do this and to save the count into a variable:

```
NUM_SLOW5_READS=$(SLOW5TOOLS stats $SLOW5_FILE | grep "number of records" | awk '{print $NF}')
```

Now we can use this `NUM_SLOW5_READS` variable to compare with the count from FAST5 files. Unfortunatelly we are not aware of a straightforward way to get the number of records from a set of FAST5 files quickly. So far, the most accurate (but time consuming) method we found is to use `strings` command to print out all the strings in a fast5 file and to `grep` for the strings of the format *read_.*-.*-.** that represents a read ID. If you have GNU parallel (`apt-get install parallel`) we can do this in parallel for multiple files in a directory and sum it up using `awk`:
```
NUM_READS=$(find $FAST5_DIR -name '*.fast5' | parallel -I% --max-args 1 strings % | grep "read_.*-.*-.*" | wc -l | awk 'BEGIN {count=0;} {count=count+$0} END {print count;}')
```

Otherwise, do it serially (will be slow) using `xargs` as:
```
NUM_READS=$(find $FAST5_DIR -name '*.fast5' | xargs --max-args 1 strings | grep "read_.*-.*-.*" | wc -l | awk 'BEGIN {count=0;} {count=count+$0} END {print count;}')
```

Now we can simply compare `NUM_READS` to `NUM_SLOW5_READS` and see if they are equal:
```
if [ ${NUM_READS} -ne ${NUM_SLOW5_READS} ]
then
    echo "ERROR: fall back sanity check also failed. $NUM_READS in FAST5, but $NUM_SLOW5_READS reads in SLOW5"
    exit 1
else
    echo "$NUM_READS in FAST5, $NUM_SLOW5_READS reads in SLOW5"
fi
```

A bash function which you can directly copy paste into your script is here:
```
sanity_check_fast5_num_reads(){

	FAST5_DIR=$1
	NUM_SLOW5_READS=$2

	test -d  ${FAST5_DIR} || die "$FAST5_DIR not found"

	if parallel --version > /dev/null
	then
		NUM_READS=$(find $FAST5_DIR -name '*.fast5' | parallel -I% --max-args 1 strings % | grep "read_.*-.*-.*" | wc -l | awk 'BEGIN {count=0;} {count=count+$0} END {print count;}')
	else
		NUM_READS=$(find $FAST5_DIR -name '*.fast5' | xargs --max-args 1 strings | grep "read_.*-.*-.*" | wc -l | awk 'BEGIN {count=0;} {count=count+$0} END {print count;}')
	fi

	if [ ${NUM_READS} -ne ${NUM_SLOW5_READS} ]
	then
		die "ERROR: fall back sanity check also failed. $NUM_READS in FAST5, but $NUM_SLOW5_READS reads in SLOW5"
	else
		echo "$NUM_READS in FAST5, $NUM_SLOW5_READS reads in SLOW5"
	fi

}
```

Simply call it as `sanity_check_fast5_num_reads $FAST5_DIR $NUM_SLOW5_READS` in your bash script assuming you have set FAST5_DIR and NUM_SLOW5_READS as we discussed in the beginning of this article.

If you think this is too time consuming, a quick method to estimate the count is by to get the number of records in one of the fast5 files in the sample and multiplying that by the number of FAST5 files. But note that this count can vary from file to file sometimes - so in this case we will find the largest FAST5 and grab its read count and multiply it by the number of FAST5 files:

```
# find the largest fast5 file by file size
LARGEST_FAST5=$(find ${FAST5_DIR} -name '*.fast5' -printf "%s\t%p\n" | sort -n | tail -1 | awk '{print $2}')
# a hacky way to get the number of reads
NUM_READS_IN_FAST5=$(strings  ${LARGEST_FAST5} | grep "read_.*-.*-.*" | wc -l)
# get the number of fast5 files
NUMFAST5=$(find $FAST5_DIR -name '*.fast5' | wc -l)
# now do the multiplication
NUM_FAST5_READS=$(echo "($NUMFAST5)*($NUM_READS_IN_FAST5)" | bc)
```

This `NUM_FAST5_READS` will obviously be an over-estimate. So we will have to check if the read count in SLOW5 is within a percentage:
```
# get the percentage of NUM_SLOW5_READS over NUM_FAST5_READS
PASS_PERCENT=$(echo "(((($NUM_SLOW5_READS))/($NUM_FAST5_READS))*100)" | bc -l)
# make PASS_PERCENT into an integer to be compared in bash
PASS_PERCENTINT=$(echo "$PASS_PERCENT/1" | bc)

# The test is done only if the fast5 file count is large enough NUMFAST5>20 in this example
# If PASS_PERCENTINT<95 it is considered failed in this example 
if [ ${NUMFAST5} -gt 20 ] && [ ${PASS_PERCENTINT} -lt 95 ]
then
    echo "ERROR: Sanity check failed. Only $NUM_SLOW5_READS in SLOW5 out of $NUM_FAST5_READS estimated raw reads in FAST5 (${PASS_PERCENT}%)"
    exit 1
else
    echo "$NUM_SLOW5_READS in SLOW5, $NUM_FAST5_READS estimated raw reads in FAST5 (${PASS_PERCENT}%)"
fi
```    

Note that this test even though it is fast, it can give false positives. So a better way would be to fall back to the slow accurate  method we discuused above (sanity_check_fast5_num_reads function) if this test fails. A bash function that does all this:

```
sanity_check_fast5_num_reads_estimate(){

	FAST5_DIR=$1
	NUM_SLOW5_READS=$2

	test -d  ${FAST5_DIR} || die "$FAST5_DIR not found"

	#estimate number of reads in multi-fast5
	LARGEST_FAST5=$(find ${FAST5_DIR} -name '*.fast5' -printf "%s\t%p\n" | sort -n | tail -1 | awk '{print $2}')
	NUM_READS_IN_FAST5=$(strings  ${LARGEST_FAST5} | grep "read_.*-.*-.*" | wc -l)
	NUMFAST5=$(find $FAST5_DIR -name '*.fast5' | wc -l)
	NUM_FAST5_READS=$(echo "($NUMFAST5)*($NUM_READS_IN_FAST5)" | bc)

	PASS_PERCENT=$(echo "(((($NUM_SLOW5_READS))/($NUM_FAST5_READS))*100)" | bc -l)
	PASS_PERCENTINT=$(echo "$PASS_PERCENT/1" | bc)

	if [ ${NUMFAST5} -gt 20 ] && [ ${PASS_PERCENTINT} -lt 95 ]
	then
		echo "Estimated sanity check failed - Only $NUM_SLOW5_READS in SLOW5 out of $NUM_FAST5_READS estimated raw reads in FAST5 (${PASS_PERCENT}%). Trying accurate method."
        sanity_check_fast5_num_reads $FAST5_DIR $NUM_SLOW5_READS
	else
		echo "$NUM_SLOW5_READS in SLOW5, $NUM_FAST5_READS estimated raw reads in FAST5 (${PASS_PERCENT}%)"
	fi

}

```

## Read ID uniqueness based sanity checks

What if the counts are matching but one file got merged twice (extremely unlikely). You can do a test if the read IDs are all unqiue.
The easiest way is to slow5tools index on you rmeged file, as it will fail if there are duplicate read IDs. Index will go through the whole file will also indicate if the file is corrupted or truncated. And the generated index also can be archived if necessary as then one could akip this step later during analysis. Following is a code snippet:

```
slow5tools index ${SLOW5_FILE} || { echo "Indexing failed"; exit 1; }
```

A very expesnsive and time-consuming uniqueness test can be done using `slow5tools view`. Unlike index command, slow5tools view will decomporess and parse each and every record, so this test will paranoidly verify if each and every record is perfect. Follwoing is a code snippet that use slow5tools view to print all the read IDs, sort them and count occurances of each read id using uniq -c command, sort them based on the counts and and then using awk to see if the largest count is still 1.

```
slow5tools view -t ${NUM_THREADS} $SLOW5_FILE |  awk '{print $1}' | grep  -v "^[#@]" |  sort | uniq -c  | sort -rn | awk '{if($1!=1){print "Duplicate read ID found",$2; exit 1}}' || { echo "ERROR: Sanity check failed. Duplicate reads in SLOW5"; exit 1; }

```

## Ultimate base-calling sanity check

The ultimate test would be to basecall the original files and the reconverted files using Guppy.
```
	# get the number of read groups in the SLOW5
    NUM_READ_GROUPS=$(slow5tools stats ${SLOW5_FILE}  | head | grep "number of read groups" | awk '{print $NF}')
	
    # if there are multiple read groups, first split by read group, and then into 4000 read SLOW5 files
    if [ ${NUM_READ_GROUPS} -ne 1 ]
	then
		slow5tools split  ${SLOW5_FILE} -d split_groups_tmp/ -g -t $NUM_THREADS 
		slow5tools split  split_groups_tmp/ -d split_reads_tmp/ -r 4000 -t $NUM_THREADS"
		rm -r split_groups_tmp/
	# if only one read group, directly split  4000 read SLOW5 files
    else
		slow5tools split ${SLOW5_FILE} -d split_reads_tmp/ -r 4000 -t $NUM_THREADS 
	fi

    # convert the split slow5 files to fast5
	slow5tools s2f split_reads_tmp -d s2f_fast5/ -p $NUM_THREADS 
	rm -r split_reads_tmp/

```

```
	guppy_basecaller -c basecallconfig.cfg -i ${FAST5_DIR} -s fast5_basecalls/  -r --device cuda:all
	guppy_basecaller -c basecallconfig.cfg -i s2f_fast5/ -s s2f_fast5_basecalls/  -r --device cuda:all
	rm -r s2f_fast5
    
```

see if the diff passes on sorted fastqs and sorted sequencing summaries. If the diff passes it means all the raw signal data is saved without loss and we do not need to worry. A function si below which you can call as `compare_basecalls fast5_basecalls/ s2f_fast5_basecalls/`;


```
compare_basecalls (){
	A=$1
	B=$2

	test -e $A || die "$A not present."
	test -e $B || die "$B not present."

    # We sort the fastq files based on the read_ids because the output order from guppy is not deterministic
	find $A -name '*.fastq' -exec cat {} + | paste - - - -  | sort -k1,1  | tr '\t' '\n' > a.fastq
	find $B -name '*.fastq' -exec cat {} + | paste - - - -  | sort -k1,1  | tr '\t' '\n' > b.fastq
	
    diff -q a.fastq b.fastq || { echo "Basecalls differ"; exit 1; }

    # We strip out the file names and then sort before comparing
	cut -f2,3,5- $A/sequencing_summary.txt | sort -k1 -T . > a.txt
	cut -f2,3,5- $B/sequencing_summary.txt | sort -k1 -T . > b.txt
	
    diff -q a.txt b.txt  || { echo "sequencing summary files differ"; exit 1; }

}
```

But note that sometimes this test will fail not due to slow5 but due to some issues in basecaller (https://github.com/hasindu2008/slow5tools/issues/70).