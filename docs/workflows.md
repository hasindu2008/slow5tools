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

## Bonito

SLOW5 support for ONT's Bonito basecaller is now available as a [pull request](https://github.com/nanoporetech/bonito/pull/252) along with usage instructions and benchmarks.



