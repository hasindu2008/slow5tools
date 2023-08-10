# Datasets directly in S/BLOW5 format

## NA12878 R9.4.1 PromethION

The NA12878 R9.4.1 PromethION dataset sequenced for the [SLOW5 paper](https://www.nature.com/articles/s41587-021-01147-4) is available on [SRA](https://www.ncbi.nlm.nih.gov/sra/?term=SRS9414678) and links are given below:

| <sub>Description</sub>                                          | <sub>SRA run Data access</sub>                                                                                         | <sub>Direct download link (md5sum)</sub>  |
|------------------------------------------------------|------------------------------------------------------------------------------------------------------------|----------------------|
| <sub>~20K reads subsubset (BLOW5, FAST5, FASTQ, BAM)</sub>                  |                                                -                                                                      | <sub>[NA12878_prom_subsubsample.tar.gz](https://slow5.page.link/na12878_prom_subsub)</sub> <sub>(`f64074151d25d6e35c73f668d4146032`)</sub> |
| <sub>~500K reads subset (BLOW5 format)</sub>                    | <sub>[SRR22186403](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR22186403&display=data-access)</sub> | <sub>[subsample_slow5.tar](https://slow5.page.link/na12878_prom_sub_slow5)</sub> <sub>(`6cdbe02c3844960bb13cf94b9c3173bb`)</sub> |
| <sub>~9M reads complete PromethION dataset (BLOW5 format)</sub> | <sub>[SRR22186402](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR22186402&display=data-access)</sub> | <sub>[na12878_prom_merged.blow5](https://slow5.page.link/na12878_prom_slow5) (`7e1a5900aff10e2cf1b97b8d3c6ecd1e`), [na12878_prom_merged.blow5.idx](https://slow5.page.link/na12878_prom_slow5_idx) (`a78919e8ac8639788942dbc3f1a2451a`) </sub>                          |
| <sub>~500K reads subset (FAST5 format)</sub>                    | <sub>[SRR15058164](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR15058164&display=data-access)</sub> | <sub>[subsample.tar.gz](https://slow5.page.link/na12878_prom_sub)</sub> <sub>(`591ec7d1a2c6d13f7183171be8d31fba`)</sub> |
| <sub>~9M reads complete PromethION dataset (FAST5 format)</sub> | <sub>[SRR15058166](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR15058166&display=data-access)</sub> | <sub>[fast5.tar.gz](https://slow5.page.link/na12878_prom)</sub>   <sub>(`0adbd2956a54528e92dd8fe6d42d2fce`)</sub> |

## NA24385 R10.4.1 LSK114 PromethION

An NA24385 R10.4.1 LSK114 dataset sequenced on a PromethION is available on [SRA](https://www.ncbi.nlm.nih.gov/sra/?term=SRS16575602)  and given below are the links:

| <sub>Description</sub>                                          | <sub>SRA run Data access</sub>                                                                                         | <sub>Direct download link (md5sum)</sub>  |
|------------------------------------------------------|------------------------------------------------------------------------------------------------------------|----------------------|
| <sub>~20K reads subsubset (BLOW5 format)</sub>                  |  |     <sub>[hg2_prom_lsk114_subsubsample.tar](https://slow5.page.link/hg2_prom_subsub)</sub> <sub>(`4d338e1cffd6dbf562cc55d9fcca040c`)</sub> |
| <sub>~500K reads subset (BLOW5 format)</sub>                    | <sub>[SRR23215365](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23215365&display=data-access)</sub> |     <sub>[hg2_subsample_slow5.tar](https://slow5.page.link/hg2_prom_sub_slow5)</sub> <sub>(`65386e1da1d82b892677ad5614e8d84d`)</sub> |
| <sub>~15M reads complete PromethION dataset (BLOW5 format)</sub> | <sub>[SRR23215366](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23215366&display=data-access)</sub> | <sub> [PGXX22394_reads.blow5](https://slow5.page.link/hg2_prom_slow5) (`3498b595ac7c79a3d2dce47454095610`), [PGXX22394_reads.blow5.idx](https://slow5.page.link/hg2_prom_slow5_idx) (`1e11735c10cf63edc4a7114f010cc472`)</sub>*                         |

*This dataset is hosted in the [gtgseq AWS bucket](https://aws.amazon.com/marketplace/pp/prodview-rve772jpfevtw) granted by the AWS open data sponsorship programme, for which the documentation available under the [gtgseq GitHub repository](https://github.com/GenTechGp/gtgseq).

## NA12878 R10.4.1 LSK114 PromethION

An NA12878 R10.4.1 LSK114 dataset sequenced on a PromethION is available at the links below:

| <sub>Description</sub>                                          | <sub>SRA run Data access</sub>                                                                                         | <sub>Direct download link (md5sum)</sub>  |
|------------------------------------------------------|------------------------------------------------------------------------------------------------------------|----------------------|
| <sub>~11M reads complete PromethION dataset (BLOW5 format)</sub> | <sub>-</sub> | <sub> [PGXXHX230142_reads.blow5](https://slow5.page.link/na12878_prom2_slow5) (`24266f6dabb8d679f7f520be6aa22694`), [PGXXHX230142_reads.blow5.idx](https://slow5.page.link/na12878_prom2_slow5_idx) (`a5659f829b9410616391427b2526b853`) </sub>*                         |


*This dataset is hosted in the [gtgseq AWS bucket](https://aws.amazon.com/marketplace/pp/prodview-rve772jpfevtw) granted by the AWS open data sponsorship programme, for which the documentation available under the [gtgseq GitHub repository](https://github.com/GenTechGp/gtgseq).

## MinION selective sequencing datasets

MinION datsets sequenced with readfish selective sequencing for [Comprehensive genetic diagnosis of tandem repeat expansion disorders with programmable targeted nanopore sequencing](https://www.science.org/doi/10.1126/sciadv.abm5386) are available on [SRA](https://trace.ncbi.nlm.nih.gov/Traces/?view=study&acc=SRP349335).
- tar files without "_reads" at the end (e.g., GBXM047265.tar) are BLOW5 data
- tar files with _reads at the end (e.g., GBXM047265_reads.tar) are FAST5 data

## Converted public datasets

Following public datasets from others have been converted to BLOW5 format. Relatively smaller datasets (hundreds of GBs) are directly available for download. Larger datasets (terabytes) have been uploaded to [SRA](https://www.ncbi.nlm.nih.gov/bioproject/PRJNA932454) and are available for cloud delivery. Alternatively, these converted BLOW5 files are currently stored locally in a archive storage at Garvan Institute, if anyone is interested contact.

1. [SP1 SARS-CoV-2 dataset](https://community.artic.network/t/links-to-raw-fast5-fastq-data-for-artic-protocol/17):
- [SP1-raw-mapped.blow5](https://slow5.page.link/SP1-raw-mapped) (md5sum: `d87c60f70bf8646ee56bcee2795e7535`)
- [SP1-raw-mapped.blow5.idx](https://slow5.page.link/SP1-raw-mapped-idx) (md5sum: `c79ef9280be63fad7c07e4352402ce7a`)

2. Some of the [Zymo Mock community](https://github.com/LomanLab/mockcommunity) data:
- [Zymo-GridION-EVEN-BB-SN.blow5](https://slow5.page.link/Zymo-GridION-EVEN-BB-SN) (md5sum: `d7c894164aef398907adc6c034dd3049`)
- [Zymo-GridION-EVEN-BB-SN.blow5.idx](https://slow5.page.link/Zymo-GridION-EVEN-BB-SN-idx) (md5sum: `d7d5feae1107c6d4517ebb416dc02683`)

3. All raw nanopore data from [Telomere-to-telomere consortium CHM13 project](https://github.com/marbl/CHM13)
- BLOW5 files available from [SRR23371619](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23371619&display=data-access). file name: `CHM13_T2T_ONT_blow5.tar` (md5sum: `04f9d1c6ea2d11ccfc131c8244f059d3`).

4. All [nanopore-wgs-consortium](https://github.com/nanopore-wgs-consortium/NA12878) datasets:
- BLOW5 files for the DNA dataset available from [SRR23513620](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23513620&display=data-access). filename: `na12878_DNA_blow5.tar` (md5sum: `2d02a7706d00572dcd9fcfa96e0357f4`)
- BLOW5 files for the direct-RNA dataset available from [SRR23513624](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23513624&display=data-access). filename: `na12878_directRNA_blow5.tar` (md5sum: `282e305f2b6a72d28980a8d5c803d54e`. Also available for direct download from [na12878_rna_merged.blow5](https://slow5.page.link/na12878_rna) (md5sum: `36bc164e9d885838245073f6cd2ecd79`), [na12878_rna_merged.blow5.idx](https://slow5.page.link/na12878_rna_idx) (md5sum: `82f96208ac2f42574abe0cf5a3954602`)
- BLOW5 files for the cDNA-RNA dataset available from [SRR23513622](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR23513622&display=data-access). filename: `na12878_cDNA_blow5.tar` (md5sum: `cba2ce651d8c33528e594a9e45ff6515`)



