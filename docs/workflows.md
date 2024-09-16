# Example workflows

## To convert a FAST5 dataset to a single BLOW5 file and then convert back to FAST5

```bash
#convert fast5 files to slow5 files using 8 I/O processes
slow5tools f2s fast5_dir -d blow5_dir  -p 8
#Merge all the slow5 files in to a single file using 8 threads
slow5tools merge blow5_dir -o file.blow5 -t8
#remove the temporary directory
rm -rf  blow5_dir
#Split the single SLOW5 file into multiple SLOW5 files such that each file has 4000 reads
slow5tools split file.blow5 -d blow5_dir -r 4000
#Now convert to FAST5 using using 8 I/O processes
slow5tools s2f blow5_dir -d fast5  -p 8
```
## Methylation calling or eventalignment using *f5c*

[f5c](https://github.com/hasindu2008/f5c/) version 0.7 onwards supports SLOW5 file format. Existing FAST5 files can be converted to SLOW5 using slow5tools and used with *--slow5* option in *f5c*.

```bash
#convert fast5 files to slow5 files using 8 I/O processes
slow5tools f2s fast5_dir -d blow5_dir -p 8
#Merge all the slow5 files in to a single file using 8 threads
slow5tools merge blow5_dir -o signals.blow5 -t8
#f5c index
f5c index -t 8  reads.fq --slow5 signals.blow5
#f5c methylation calling
f5c call-methylation -t 8 -r reads.fq -g ref.fa -b reads.bam --slow5 signals.blow5 > meth.tsv
#f5c eventalign
f5c eventalign -t 8 -r reads.fq -g ref.fa -b reads.bam --slow5 signals.blow5 > meth.tsv
```

## Nanopolish

[Nanopolish](https://github.com/jts/nanopolish) version 0.14.0 onwards supports SLOW5 file format.

```bash
#convert fast5 files to slow5 files using 8 I/O processes
slow5tools f2s fast5_dir -d blow5_dir -p 8
#Merge all the slow5 files in to a single file using 8 threads
slow5tools merge blow5_dir -o signals.blow5 -t8
#nanopolish index
nanopolish index reads.fq --slow5 signals.blow5
#once indexed as above, other nanopolish commands do not need any special option. for insance for methylation calling:
nanopolish call-methylation -t 8 -r reads.fq -g ref.fa -b reads.bam > meth.tsv

```

##  Sigmap

[Sigmap](https://github.com/haowenz/sigmap) master branch now supports SLOW5 file format.

```bash
#convert fast5 files to slow5 files using 8 I/O processes
slow5tools f2s fast5_dir -d blow5_dir -p 8
# run sigmap
./sigmap -m -r ref.fa -p <model> -x index -s blow5_dir -o mapping.paf -t 8
```

## Basecalling

- SLOW5 support for ONT's Guppy basecaller is available as a [wrapper called buttery-eel here](https://github.com/Psy-Fer/buttery-eel) along with instructions.
- SLOW5 support for ONT's Dorado basecaller (including support for duplex calling) is available in our own [Dorado fork here](https://github.com/hiruna72/slow5-dorado/releases) along with instructions.
- SLOW5 support for ONT's Bonito basecaller is now available as a [pull request](https://github.com/nanoporetech/bonito/pull/252) along with usage instructions and benchmarks.

## Extracting a subset from S3 storage

If have an s3 bucket a large S/BLOW5 file is stored along with its index, you can directly extract a subset of reads without downloading a whole file. For this you will have to mount your s3 bucket locally and then use slow5tools get to fetch the necessary reads.

```bash
#install s3fs first
sudo apt-get install s3fs
#directory to mount the bucket
mkdir s3
#command to mount a public bucket called bucket_name onto s3
s3fs bucket_name s3/ -o public_bucket=1  -o url=http://s3.amazonaws.com/ -o dbglevel=info -o curldbg -o umask=0005 -o  uid=$(id -u)
#if there is a file called reads.blow5 with reads.blow5.idx in the s3 bucket, you can specify a list of read ids to slow5tools get
slow5tools get s3/reads.blow5 --list readid.list -o out.blow5
```

For mounting private buckets, put your ACCESS:KEY in ~/.passwd-s3fs (make sure 600 permission) and use the command `s3fs bucket_name s3/ -o url=http://s3.amazonaws.com/ -o dbglevel=info -o curldbg  -o umask=0005 -o uid=$(id -u)`

## Extract and re-basecall reads mapping to a particular genomic region

```bash
samtools view reads.bam chrX:147911919-147951125 | cut -f1  | sort -u > rid_list.txt
slow5tools get reads.blow5 --list rid_list.txt -o extracted.blow5
buttery-eel -i extracted.blow5  -g /path/to/ont-guppy/bin/ --config dna_r9.4.1_450bps_sup.cfg --device 'cuda:all' -o extracted_sup.fastq #see https://github.com/Psy-Fer/buttery-eel/ for butter-eel options
```

Note: If the read IDs in the BAM file are not the parent IDs (happens when read splitting is enabled during initial basecalling step), you can grab the parent read IDs from the FASTQ file as below and use that as the input the to slow5tools get.
```
grep -F -f rid_list.txt reads.fastq | sed -n -e 's/.*parent\_read\_id=//p' | awk '{print $1}' | sort -u > parent_rid_list.txt
```
The above assumes that `parent_read_id` tag is present in all reads including those that are not split, which seem to be the case when doing live-basecalling. But Guppy offline version seem to only output that `parent_read_id` tag for split reads. In that case do this:
```
# for split reads, get the parent_read_id tag
grep -F -f rid_list.txt reads.fastq | sed -n -e 's/.*parent\_read\_id=//p' | awk '{print $1}' > tmp.txt
# for non split reads, get the normal read ID
grep -F -f rid_list.txt reads.fastq | grep -v "parent\_read\_id" | awk '{print $1}' | tr -d '@' >> tmp.txt
# remove duplicates
sort -u tmp.txt > parent_rid_list.txt
```

## Extract 20,000 random reads from a BLOW5 file

```
slow5tools skim --rid reads.blow5 | sort -R | head -20000 > rand_20000_rid.txt # for slow5tools v0.7.0 onwards
slow5tools get reads.blow5 --list rand_20000_rid.txt -o reads_subsubsample.blow5
```

## Demultiplexing

From slow5tools v1.3.0 onwards, you can use the buttery-eel barcode summary file `barcode_summary.txt` to demultiplex a BLOW5 file.

```
slow5tools split -d blow5_dir -x barcode_summary.txt
```

BASH with slow5tools is an easy way to demultiplex a BLOW5 file. Say you have demultiplexed your run and have one FASTQ file per each barcode, namely barcode_0.fastq, barcode_1.fastq, barcode_2.fastq and barcode_3.fastq. If your merged BLOW5 file for the whole run is reads.blow5, you can create separate BLOW5 files for each barcode by using a bash loop as below:

```bash
for barcode in $(seq 0 3)
do
    awk '{if(NR%4==1) {print $1}}' barcode_${barcode}.fastq | tr -d '@' > read_id_${barcode}.txt
    cat read_id_${barcode}.txt | slow5tools get reads.blow5 -o barcode_${barcode}.blow5
done
```

Equivalently, using slow5tools v1.3.0 onwards:

```bash
printf 'read_id\tbarcode\n' > barcodes.tsv
for barcode in $(seq 0 3)
do
    awk '{if(NR%4==1) {printf "%s\t%s\n", $1, name}}' name=barcode_${barcode} barcode_${barcode}.fastq | tr -d '@' >> barcodes.tsv
done
slow5tools split reads.blow5 -d blow5_dir -x barcodes.tsv --demux-rid read_id --demux-code barcode
```

If Guppy has automatically done read splitting, you would see errors from slow5tools that some reads are not found.
In that case we need to locate these “parent read ids”, as explained under [this workflow](#extract-and-re-basecall-reads-mapping-to-a-particular-genomic-region). If anything is unclear, open an issue under slow5tools. A quick code snippet for handling “parent read ids”:

```bash
for barcode in $(seq 0 3)
do
    awk '{if(NR%4==1) {print $0}}' barcode_${barcode}.fastq | sed -n -e 's/.*parent\_read\_id=//p' | awk '{print $1}'  | sort -u > read_id_${barcode}.txt
    cat read_id_${barcode}.txt | slow5tools get reads.blow5 -o barcode_${barcode}.blow5
done
```

## Custom splitting

From slow5tools v1.3.0 onwards, you can split BLOW5 files using a custom TSV file. For example, to split by channel number:

```
slow5tools skim reads.blow5 | cut -f1,9 > split.tsv
slow5tools split reads.blow5 -d blow5_dir -x split.tsv --demux-rid '#read_id' --demux-code channel_number
```
