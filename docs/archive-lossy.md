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
The most important sanity check is to ensure that basecalling accuracy has not
been adversely affected.

First, basecall the data and obtain the BAM files. For example, using
slow5-dorado <https://github.com/hiruna72/slow5-dorado> :

	MODEL=dna_r9.4.1_e8_hac@v3.3 # path to basecalling model
	BAM=data.bam # path to bam output
	BAM_LOSSY=lossy.bam # path to lossy bam output

	slow5-dorado basecaller "$MODEL" "$SLOW5_FILE" > "$BAM"
	slow5-dorado basecaller "$MODEL" "$SLOW5_LOSSY_FILE" > "$BAM_LOSSY"

Next, obtain the identity scores using the following shell script for DNA

<https://github.com/hasindu2008/biorand/blob/master/bin/identitydna.sh>

and

<https://github.com/hasindu2008/biorand/blob/master/bin/identityrna.sh>

for RNA. For example:

	GENOME=hg38noAlt.idx # path to fasta/idx genome
	SCORE=score.tsv # path to score output
	SCORE_LOSSY=score_lossy.tsv # path to lossy score output

	identitydna.sh "$GENOME" "$BAM" > "$SCORE"
	identitydna.sh "$GENOME" "$BAM_LOSSY" > "$SCORE_LOSSY"

Check that both identity scores satisfy the following inequalities:

- mean >= 0.93,
- median >= 0.97,
- read count >= NUM_SLOW5_READS, and
- read count <= 1.2 * NUM_SLOW5_READS.

This can be achieved using the following shell snippet:

	die() {
		echo "$1" >&2
		exit 1
	}

	assert() {
		x=$(echo "if ($1) 1" | bc)
		if [ "$x" != 1 ]
		then
			die "failed: $x"
		fi
	}

	score_chk() {
		SCORE_HEADER='sample	mean	sstdev	q1	median	q3	n'
		path=$1

		hdr=$(head -n1 "$path")
		if [ "$hdr" != "$SCORE_HEADER" ]
		then
			die 'invalid header'
		fi

		data=$(tail -n1 "$path")
		mean=$(echo "$data" | cut -f2)
		med=$(echo "$data" | cut -f5)
		n=$(echo "$data" | cut -f7)

		assert "$mean >= 0.93"
		assert "$med >= 0.97"
		assert "$n >= $NUM_SLOW5_READS"
		assert "$n <= (1.2 * $NUM_SLOW5_READS)"
	}

	# quicker than section "Read Count"
	NUM_SLOW5_READS=$(slow5tools skim --rid "$SLOW5_LOSSY_FILE" | wc -l)

	score_chk "$SCORE"
	score_chk "$SCORE_LOSSY"

Finally, check that the following pairwise inequalities are satisfied:

- mean (original) - mean (lossy) <= 0.001,
- median (original) - median (lossy) <= 0.001,
- read count (original) - read count (lossy) >= 0, and
- read count (original) - read count (lossy) <= 0.001 * read count (original).

Continuing from the previous shell snippet:

	score_pair_chk() {
		path=$1
		path_lossy=$2

		data=$(tail -n1 "$path")
		mean=$(echo "$data" | cut -f2)
		med=$(echo "$data" | cut -f5)
		n=$(echo "$data" | cut -f7)

		data_lossy=$(tail -n1 "$path_lossy")
		mean_lossy=$(echo "$data_lossy" | cut -f2)
		med_lossy=$(echo "$data_lossy" | cut -f5)
		n_lossy=$(echo "$data_lossy" | cut -f7)

		assert "($mean - $mean_lossy) <= 0.001"
		assert "($med - $med_lossy) <= 0.001"
		assert "($n - $n_lossy) >= 0"
		assert "($n - $n_lossy) <= (0.001 * $n)"
	}

	score_pair_chk "$SCORE" "$SCORE_LOSSY"

Methylation
-----------
Another related sanity check is to see whether the methylation frequencies have
been adversely affected. We can achieve this by obtaining their Pearson
correlation coefficient and making sure that it is above a certain threshold.

Again, basecall the data, but this time use modification calling. For example,
using slow5-dorado:

	MODEL=dna_r9.4.1_e8_hac@v3.3 # path to basecalling model
	BASES=5mCG_5hmCG # or m6A_DRACH for rna
	BAM=meth.bam # path to bam output
	BAM_LOSSY=meth_lossy.bam # path to lossy bam output

	slow5-dorado basecaller "$MODEL" "$SLOW5_FILE" --modified-bases "$BASES" > "$BAM"
	slow5-dorado basecaller "$MODEL" "$SLOW5_LOSSY_FILE" --modified-bases "$BASES" > "$BAM_LOSSY"

Then map the BAM files to the reference genome and index them:

	map() {
		bam=$1
		genome=$2

		samtools fastq -TMM,ML "$bam" | minimap2 -x map-ont -a -y -Y --secondary=no "$genome" - | samtools sort -@32 -
	}

	GENOME=hg38noAlt.fa # path to genome fasta/idx
	BAM_MAP=meth_map.bam # path to mapped bam output
	BAM_LOSSY_MAP=meth_lossy_map.bam # path to mapped lossy bam output

	map "$BAM" "$GENOME" > "$BAM_MAP"
	map "$BAM_LOSSY" "$GENOME" > "$BAM_LOSSY_MAP"

	samtools index "$BAM_MAP"
	samtools index "$BAM_LOSSY_MAP"

Acquire the methylation frequencies using minimod
<https://github.com/warp9seq/minimod> :

	MODS=mods.mm.tsv # path to meth frequencies output
	MODS_LOSSY=mods_lossy.mm.tsv # path to lossy meth frequencies output

	minimod mod-freq "$GENOME" "$BAM_MAP" > "$MODS"
	minimod mod-freq "$GENOME" "$BAM_LOSSY_MAP" > "$MODS_LOSSY"

Finally, obtain the Pearson correlation coefficient using this Python script

<https://github.com/warp9seq/minimod/blob/main/test/compare.py>

	corr=$(python3 compare.py "$MODS" "$MODS_LOSSY")

and ensure that it is above a chosen threshold (say 0.95):

	assert "$corr >= 0.95" # using function from section "Basecalling"
