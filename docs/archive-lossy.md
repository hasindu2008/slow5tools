Archiving Lossy Data
====================

Lossy compression of raw nanopore signal data can be a great way to save disk
space without significantly impacting basecalling accuracy. This makes it
particularly suitable for archiving. Naturally, one may be concerned that this
conversion would significantly deteriorate the quality of their data. To remedy
such concerns, this guide outlines a number of sanity checks which when
successful give confidence in the lossy conversion.

The Conversion
--------------
To lossy compress your data, set the following variables

	SLOW5_FILE=data.blow5 # path to original data
	SLOW5_LOSSY_FILE=lossy.blow5 # path to lossy output
	NUM_THREADS=8

and run:

	slow5tools degrade "$SLOW5_FILE" -o "$SLOW5_LOSSY_FILE" -t "$NUM_THREADS"

If the command fails with the message "No suitable bits suggestion", this is
because your dataset type has not yet been profiled by our team. Submit an issue
on GitHub <https://github.com/hasindu2008/slow5tools/issues> with your dataset
attached.

Read Count
----------
The simplest sanity check is to ensure that the number of reads is the same for
the original and lossy compressed data. The following shell snippet does the
check:

	get_num_reads() {
		slow5tools stats "$1" | grep 'number of records' | awk '{print $NF}'
	}

	NUM_SLOW5_READS=$(get_num_reads "$SLOW5_FILE")
	NUM_SLOW5_LOSSY_READS=$(get_num_reads "$SLOW5_LOSSY_FILE")

	echo "Read count: $NUM_SLOW5_READS in SLOW5, $NUM_SLOW5_LOSSY_READS in lossy SLOW5"

	if [ "$NUM_SLOW5_READS" -ne "$NUM_SLOW5_LOSSY_READS" ]
	then
		echo 'Failed sanity check: Read count differs' >&2
		exit 1
	fi

Uniqueness
----------
Next, we should ensure that there are no duplicate read IDs in the lossy data.
The simplest method is to index the lossy file:

	slow5tools index "$SLOW5_LOSSY_FILE"

This will fail if a duplicate read ID is encountered, or additionally if the file
is corrupted or truncated. However, a more comprehensive method is to use `slow5tools
view` which decompresses and parses the entire file, and thus does a more
detailed check for data corruption. You can achieve this using the following
shell pipeline:

	slow5tools view -t "$NUM_THREADS" -K 20000 "$SLOW5_LOSSY_FILE" | awk '{print $1}' | grep -v '^\#\|^\@' | sort | uniq -c | awk '{if($1!=1){print "Duplicate read ID found",$2; exit 1}}'

Basecalling
-----------
The most important sanity check is to ensure that the basecalling accuracy has
not been adversely affected.

After obtaining the identity scores of the original and lossy compressed data,
check that both satisfy the following inequalities:

- mean >= 0.93,
- median >= 0.97,
- read count >= NUM_SLOW5_READS, and
- read count <= 1.2 * NUM_SLOW5_READS.

Next, check that the following pairwise inequalities are satisfied:

- mean (original) - mean (lossy) <= 0.001,
- median (original) - median (lossy) <= 0.001,
- read count (original) - read count (lossy) >= 0, and
- read count (original) - read count (lossy) <= 0.001 * NUM_SLOW5_READS.
