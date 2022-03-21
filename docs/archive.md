# A pedant's guide on using slow5 for archiving

BLOW5 format is not only useful for fast signal-level analysis, it is also a great alternative for archiving raw nanopore signal data. You can save heaps of space, for instance, our in-house data generated at the Garvan Institute during 2021 comprised 245 TB FAST5 files, which were reduced to just 109 TB after conversion to BLOW5. A concern one might be having is on whether the integrity of original data be retained with BLOW5. We assure users that FAST5 to BLOW5 conversion is fully lossless, and can be converted back to FAST5 any time. But still many users (including us) can be a bit paranoid and it's good to be pedantic when it comes to potential data loss.
This vignette walks you through a number of sanity checks that remedies those concerns. Note that some of these tests are redundant and very time-consuming. Depending on how pedantic you are, you can pick the tests you want.

# Suggested conversion structure

We suggest converting all FAST5 files from a single sequencing run into a single merged BLOW5 file. This way, it is very convenient to manage. The examples in this articles are centred around this conversion structure. However, this is our personnel preference, you can either have any number of BLOW5 files per sequencing run or even merge multiple sequencing runs into one BLOW5. But we suggest not to merge drastically different runs (e.g., a PromethION run with a MinION run, a run done in year 2020 with a run in 2022) into one BLOW5 file for archiving purposes. This is because the FAST5 file structure can vary significantly and mixing them up makes things unnecessarily complex. However, for analysis purposes, you can merge as you wish, as you have original files if something goes wrong.

For a given sequencing run, call `slow5tools f2s` to convert each FAST5 file to BLOW5. Then, merge all SLOW5 files into a single file using `slow5tools  merge`. Given below is a bash code snippet:

```bash
FAST5_DIR=/path/to/input/fast5/
DIR_TMP_SLOW5=/path/to/temporary/slow5/
SLOW5_FILE=/path/to/merged.blow5
NUM_THREADS=8

slow5tools f2s ${FAST5_DIR} -d ${DIR_TMP_SLOW5} -p ${NUM_THREADS}
slow5tools merge ${DIR_TMP_SLOW5}/ -o ${SLOW5_FILE} -t${NUM_THREADS}

rm -r ${DIR_TMP_SLOW5}
```

## Sanity check by counting number of reads

One of the simplest sanity checks is to check if the read count in the merged BLOW5 file is same as those in input FAST5 files. Getting the number of read records in a merged BLOW5 file is quite simple. A bash command that gets the number of read records using `slow5tools stats` and save the count into a variable:

```bash
NUM_SLOW5_READS=$(slow5tools stats $SLOW5_FILE | grep "number of records" | awk '{print $NF}')
```

Now this `NUM_SLOW5_READS` variable can be used to compare with the count from FAST5 files. Unfortunately we are not aware of a straightforward way to get the number of records from a set of FAST5 files quickly (please let us know if there is). So far, the most accurate (but time consuming) method we found is to use `strings` command to print out all the strings in a FAST5 file and then to `grep` for the strings of the format `read_.*-.*-.*` that represents a UUID-based read ID. If you have GNU parallel (`apt-get install parallel`) this can be done in parallel for multiple FAST5 files in a directory and summed up using `awk`:
```bash
NUM_READS=$(find $FAST5_DIR -name '*.fast5' | parallel -I% --max-args 1 strings % | grep "read_.*-.*-.*" | wc -l | awk 'BEGIN {count=0;} {count=count+$0} END {print count;}')
```

If GNU parallel is not available,  this can be done serially (will be slow obviously) using `xargs` as:
```bash
NUM_READS=$(find $FAST5_DIR -name '*.fast5' | xargs --max-args 1 strings | grep "read_.*-.*-.*" | wc -l | awk 'BEGIN {count=0;} {count=count+$0} END {print count;}')
```

Now, we can simply compare `NUM_READS` to `NUM_SLOW5_READS` and see if they are equal. A bash code snippet for this comparison is given below:

```bash
if [ ${NUM_READS} -ne ${NUM_SLOW5_READS} ]
then
    echo "ERROR: fall back sanity check also failed. $NUM_READS in FAST5, but $NUM_SLOW5_READS reads in SLOW5"
    exit 1
else
    echo "$NUM_READS in FAST5, $NUM_SLOW5_READS reads in SLOW5"
fi
```

A bash function that combines the sanity check we discussed above that can be directly copy pasted into your bash script is given below:
```bash
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
		echo "ERROR: fall back sanity check also failed. $NUM_READS in FAST5, but $NUM_SLOW5_READS reads in SLOW5"
		exit 1
	else
		echo "$NUM_READS in FAST5, $NUM_SLOW5_READS reads in SLOW5"
	fi

}
```

Simply call it as `sanity_check_fast5_num_reads $FAST5_DIR $NUM_SLOW5_READS` in your bash script assuming you have set `FAST5_DIR` and `NUM_SLOW5_READS` as we discussed in the above.

If you think this is too time consuming, a quick method to estimate the count is by to getting the number of reads in one of the FAST5 files in the sequencing run and multiplying by the number of FAST5 files. But note that the number of reads in FAST5 files can vary from file to file - so in this case we will find the largest FAST5.  grab its read count and multiply it by the number of FAST5 files:

```bash
# find the largest fast5 file by file size
LARGEST_FAST5=$(find ${FAST5_DIR} -name '*.fast5' -printf "%s\t%p\n" | sort -n | tail -1 | awk '{print $2}')
# a hacky way to get the number of reads
NUM_READS_IN_FAST5=$(strings  ${LARGEST_FAST5} | grep "read_.*-.*-.*" | wc -l)
# get the number of fast5 files
NUMFAST5=$(find $FAST5_DIR -name '*.fast5' | wc -l)
# now do the multiplication
NUM_FAST5_READS=$(echo "($NUMFAST5)*($NUM_READS_IN_FAST5)" | bc)
```

This `NUM_FAST5_READS` will obviously be an over-estimate as we used the largest file. So we will have to check if the read count in SLOW5 is within a percentage:
```bash
# get the percentage of NUM_SLOW5_READS over NUM_FAST5_READS
PASS_PERCENT=$(echo "(((($NUM_SLOW5_READS))/($NUM_FAST5_READS))*100)" | bc -l)
# make PASS_PERCENT into an integer to be compared in bash
PASS_PERCENTINT=$(echo "$PASS_PERCENT/1" | bc)

# The test is done only if the fast5 file count is large enough, NUMFAST5>20 in this example
# If PASS_PERCENTINT<95, it is considered failed in this example 
if [ ${NUMFAST5} -gt 20 ] && [ ${PASS_PERCENTINT} -lt 95 ]
then
    echo "ERROR: Sanity check failed. Only $NUM_SLOW5_READS in SLOW5 out of $NUM_FAST5_READS estimated raw reads in FAST5 (${PASS_PERCENT}%)"
    exit 1
else
    echo "$NUM_SLOW5_READS in SLOW5, $NUM_FAST5_READS estimated raw reads in FAST5 (${PASS_PERCENT}%)"
fi
```    

Note that this test, despite being fast, can give false positive errors. So a better way would be to fall back to the accurate method we discussed above (`sanity_check_fast5_num_reads` function above), if this quick test fails. A bash function that does all this:

```bash
sanity_check_fast5_num_reads_estimate(){

	FAST5_DIR=$1
	NUM_SLOW5_READS=$2

	test -d  ${FAST5_DIR} || die "$FAST5_DIR not found"

	# estimate number of reads in multi-fast5
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

## Sanity check through Read ID uniqueness

What if the counts of reads are matching, but one file got merged twice (note: this is extremely unlikely). You can eleminate this doubt by checking if the read IDs are all unqiue.
The easiest way is to call `slow5tools index` on you merged BLOW5 file, as it will fail if there are duplicate read IDs. As `slow5tools index` goes through the whole BLOW5 file, this test complimentarily checks for the unlikely scaniro of file being corrupted or truncated. Also, the generated index can be archived if necessary as then one could skip this index step to save time later during analysis. Following is a code snippet:

```bash
slow5tools index ${SLOW5_FILE} || { echo "Indexing failed"; exit 1; }
```

A very expesnsive and time-consuming uniqueness test can be done using `slow5tools view`. Unlike index command, slow5tools view will decompress and parse each and every record, so this test will verify if indivudual bits in each and every record is perfect. Following is a bash code snippet that uses `slow5tools view` to print all the read IDs, sort them and count occurances of each read id using `uniq -c` command, sort them based on the counts and then using `awk` to see if the largest count is still 1.

```bash
slow5tools view -t ${NUM_THREADS} $SLOW5_FILE |  awk '{print $1}' | grep  -v "^[#@]" |  sort | uniq -c  | sort -rn | awk '{if($1!=1){print "Duplicate read ID found",$2; exit 1}}' || { echo "ERROR: Sanity check failed. Duplicate reads in SLOW5"; exit 1; }

```

## Sanity check by basecalling

The ultimate test would be to basecall the original FAST5 files, FAST5 generated by reconverting the created BLOW5 file, and then comparing the two basecalling outputs. 

First let us split the merged BLOW5 file in to multiple smaller BLOW5 files so that `slow5tools s2f` can convert to FAST5 in parallel. Following is a code snippet that does the splitting and conversion to fast5:

```bash
# get the number of read groups in the SLOW5
NUM_READ_GROUPS=$(slow5tools stats ${SLOW5_FILE}  | head | grep "number of read groups" | awk '{print $NF}')

# if there are multiple read groups, first split by read group, and then into 4000 read SLOW5 files
if [ ${NUM_READ_GROUPS} -ne 1 ]
then
    slow5tools split  ${SLOW5_FILE} -d split_groups_tmp/ -g -t $NUM_THREADS 
    slow5tools split  split_groups_tmp/ -d split_reads_tmp/ -r 4000 -t $NUM_THREADS
    rm -r split_groups_tmp/
# if only one read group, directly split  into 4000 read SLOW5 files
else
    slow5tools split ${SLOW5_FILE} -d split_reads_tmp/ -r 4000 -t $NUM_THREADS 
fi

# convert the split slow5 files to fast5
slow5tools s2f split_reads_tmp -d s2f_fast5/ -p $NUM_THREADS 
rm -r split_reads_tmp/

```

Now basecall the original FAST5 files as well as the reconverted FAST5 files. Following are some example commands, but make sure to set the basecalling profile to match your dataset and the CPU/GPU device based on your system.

```
guppy_basecaller -c dna_r9.4.1_450bps_fast.cfg -i ${FAST5_DIR} -s fast5_basecalls/  -r --device cuda:all
guppy_basecaller -c dna_r9.4.1_450bps_fast.cfg -s s2f_fast5_basecalls/ -r --device cuda:all
rm -r s2f_fast5
    
```

Now we can check if the basecalling outputs are the same. The order of reads produced by the basecaller is not determistic make sure you sort them before comparing using `diff`. If the diff command passes, that means the data in the BLOW5 file are identical to those in FAST5.
Given below is a bash function that you can dircetly copy paste into your bash script and call inside your bash script as `compare_basecalls fast5_basecalls/ s2f_fast5_basecalls/`;


```bash
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
	cut -f2,3,5- $A/sequencing_summary.txt | sort -k1  > a.txt
	cut -f2,3,5- $B/sequencing_summary.txt | sort -k1  > b.txt
	
    diff -q a.txt b.txt  || { echo "sequencing summary files differ"; exit 1; }

}
```

However, note that sometimes this test diff will cause false errors due to base-callers providing slightly different outputs in various circumstance (see https://github.com/hasindu2008/slow5tools/issues/70). We recently came through a situation where Guppy 4.4.1 on a system with multiple GPUs (GeForce 3090 and 3070) produced slightly different results, even on the same FAST5 input when run multiple times.
