# Datasets directly in S/BLOW5 format

## NA12878 R9.4.1 PromethION

The NA12878 R9.4.1 PromethION dataset sequenced for the [SLOW5 paper](https://www.nature.com/articles/s41587-021-01147-4) is available on [SRA](https://www.ncbi.nlm.nih.gov/sra/?term=SRS9414678) and links are given below:

| <sub>Description</sub>                                          | <sub>SRA run Data access</sub>                                                                                         | <sub>Direct download link</sub>  | <sub>MD5sum</sub>  |
|------------------------------------------------------|------------------------------------------------------------------------------------------------------------|----------------------|---|
| <sub>~500K reads subset (BLOW5 format)</sub>                    | <sub>[SRR22186403](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR22186403&display=data-access)</sub> |     <sub>[subsample_slow5.tar](https://slow5.page.link/na12878_prom_sub_slow5)</sub>                 | <sub>6cdbe02c3844960bb13cf94b9c3173bb</sub> |
| <sub>~9M reads complete PromethION dataset (BLOW5 format)</sub> | <sub>[SRR22186402](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR22186402&display=data-access)</sub> | <sub>[slow5.tar](https://slow5.page.link/na12878_prom_slow5)</sub>                         | <sub>3b23f706add38612445cd4f5204ae8b5</sub> |
| <sub>~500K reads subset (FAST5 format)</sub>                    | <sub>[SRR15058164](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR15058164&display=data-access)</sub> | <sub>[subsample.tar.gz](https://slow5.page.link/na12878_prom_sub)</sub>                     | <sub>591ec7d1a2c6d13f7183171be8d31fba</sub> |
| <sub>~9M reads complete PromethION dataset (FAST5 format)</sub> | <sub>[SRR15058166](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR15058166&display=data-access)</sub> | <sub>[fast5.tar.gz](https://slow5.page.link/na12878_prom)</sub>                     | <sub>0adbd2956a54528e92dd8fe6d42d2fce</sub> |

## R10.4.1 LSK114 PromethION

R10.4.1 LSK114 PromethION dataset sequenced on a PromethION is availavle on [SRA](https://www.ncbi.nlm.nih.gov/sra/?term=SRS16575602)  and given below are the links:

| <sub>Description</sub>                                          | <sub>SRA run Data access</sub>                                                                                         | <sub>Direct download link</sub>  | <sub>MD5sum</sub>  |
|------------------------------------------------------|------------------------------------------------------------------------------------------------------------|----------------------|---|
| <sub>~500K reads subset (BLOW5 format)</sub>                    | <sub>[SRR23215365](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23215365&display=data-access)</sub> |     <sub>[hg2_subsample_slow5.tar](https://slow5.page.link/hg2_prom_sub_slow5)</sub>                 | <sub>65386e1da1d82b892677ad5614e8d84d</sub> |
| <sub>~15M reads complete PromethION dataset (BLOW5 format)</sub> | <sub>[SRR23215366](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23215366&display=data-access)</sub> | <sub> - </sub>                         | <sub>9aa28813714b6a9dcc32c540e2fefdc5</sub> |

## MinION selective sequencing datasets

MinION datsets sequenced with readfish selective sequencing for [Comprehensive genetic diagnosis of tandem repeat expansion disorders with programmable targeted nanopore sequencing](https://www.science.org/doi/10.1126/sciadv.abm5386) are available on [SRA](https://trace.ncbi.nlm.nih.gov/Traces/?view=study&acc=SRP349335).
- tar files without "_reads" at the end (e.g., GBXM047265.tar) are BLOW5 data
- tar files with _reads at the end (e.g., GBXM047265_reads.tar) are FAST5 data

## Converted public datasets

Following public datasets from others have been converted to BLOW5 format. Relatively smaller datasets (hundreds of GBs) are directly available for download. Larger datasets (terabytes) have been uploaded to [SRA](https://www.ncbi.nlm.nih.gov/bioproject/PRJNA932454) and are available for cloud delivery. Alternatively, these converted BLOW5 files are currently stored locally in a archive storage at Garvan Institute, if anyone is interested contact.

1. [SP1 SARS-CoV-2 dataset](https://community.artic.network/t/links-to-raw-fast5-fastq-data-for-artic-protocol/17):
- [SP1-raw-mapped.blow5](https://slow5.page.link/SP1-raw-mapped)
- [SP1-raw-mapped.blow5.idx](https://slow5.page.link/SP1-raw-mapped-idx)

2. Some of the [Zymo Mock commuinity](https://github.com/LomanLab/mockcommunity) data:
- [Zymo-GridION-EVEN-BB-SN.blow5](https://slow5.page.link/Zymo-GridION-EVEN-BB-SN)
- [Zymo-GridION-EVEN-BB-SN.blow5.idx](https://slow5.page.link/Zymo-GridION-EVEN-BB-SN-idx)

3. All raw nanopore data from [Telomere-to-telomere consortium CHM13 project](https://github.com/marbl/CHM13)
- BLOW5 files available from [SRR23371619](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23371619&display=data-access). file name: `CHM13_T2T_ONT_blow5.tar`, md5sum: `04f9d1c6ea2d11ccfc131c8244f059d3`.

4. All [nanopore-wgs-consortium](https://github.com/nanopore-wgs-consortium/NA12878) datasets:
- BLOW5 files for the DNA dataset available from [SRR23513620](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23513620&display=data-access). filename: `na12878_DNA_blow5.tar`, md5sum: `2d02a7706d00572dcd9fcfa96e0357f4`




