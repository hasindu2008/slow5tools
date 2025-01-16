# Frequently Asked Questions

**Q1:** When trying to split a slow5 file I get "The file XX contains multiple read groups. You must first separate read groups using -g". How can I split a slow5 file with multiple read groups such that one file contains 4000 reads?

For example, if you had a XX blow5 file with two read groups. You first need to create two separate files using `split`, where each new file only contains one read group.

`slow5tools split -g XX -d out_dir1`

The `out_dir1` should now have two files. Now to create slow5s such that each file contains 4000 reads and to store them in `out_dir2`,

`slow5tools split -r 4000 out_dir1 -d out_dir2`

The number of slow5 files in `out_dir2` depends on how many number of reads were present in each single group file, but each will now contain 4000 reads, except the last file, which will contain whatever is left over for that read group


**Q2:** `slow5tools get` or `int slow5_get(const char *read_id, slow5_rec_t **read, slow5_file_t *s5p)` fails to fetch a record.

This is the normal behaviour if the slow5 file does not have the querying record. If slow5tools warns that the slow5 index is older than the slow5 file, delete the index and try again. If the slow5 file was replaced with a different slow5 file but has the same name, the old index should be deleted. Also, it could be that the read ID you are querying has a different parent read ID. You can check if this is the case by inspecting the basecalled FASTQ file (look for the tag called *parent_read_id*).

```bash
grep <read_id> reads.fastq | sed -n -e 's/.*parent\_read\_id=//p' | awk '{print $1}'
```


**Q3:** The fast5 file is compressed with VBZ but the required plugin is not loaded when trying to convert fast5 to slow5

This is due to the *vbz* compression used in the latest fast5 files. You need to setup the *vbz* plugin for HDF5. We have provided a helper script in *slow5tools* which you can invoke as `scripts/install-vbz.sh`. This script attempts to determine your operating system and the architecture, downloads and extracts the plugin (.so file) to *$HOME/.local/hdf5/lib/plugin*. You must export the path to the plugin by invoking `export HDF5_PLUGIN_PATH=$HOME/.local/hdf5/lib/plugin` . Note that the exported environmental variable will not persist across different shells. For persistence, you must add `export HDF5_PLUGIN_PATH=$HOME/.local/hdf5/lib/plugin` to your *~/.bashrc* or */etc/environment*. Make sure you logout and login back for any changes to *~/.bashrc* or */etc/environment* take effect.

Our helper script currently supports Linux on x86_64 and aarch64 architectures only. If your system is of a different operating system and/or architecture (or if this helper script fails) you have to download and manually install the plugin from [here](https://github.com/nanoporetech/vbz_compression/releases).
See [this post](https://github.com/nanoporetech/vbz_compression/issues/5) for troubleshooting a manual installation.

**Q4:** Are there any small example datasets that can be used for testing slow5tools?

A tiny subset (~20K reads) of the original NA12878 R9.4.1 PromethION dataset used for benchmaking in the [SLOW5 paper](https://www.nature.com/articles/s41587-021-01147-4) is available [here](https://slow5.bioinf.science/na12878_prom_subsub). A tiny subset (~20K reads) of a NA24385 R10.4.1 PromethION dataset is available [here](https://slow5.bioinf.science/hg2_prom_subsub). Links and information on complete datasets of those samples as well as additional datasets can be found [here](https://hasindu2008.github.io/slow5tools/datasets.html).

**Q5:** How can I make SLOW5 to FAST5 conversion fast?

If your SLOW5 to FAST5 conversion takes ages, it is likely that you called `slow5tools s2f` on a single SLOW5 file which will only use one processor. For parallelising the conversion, first split the SLOW5 file into multiple files using `slow5tools split -r` (you may have to use `slow5tools split -g` first if you BLOW5 file has multiple read groups). Then call `slow5tools s2f` with `-p <num_processes>` so that multiple CPU processors can be used for parallel conversion.

**Q6:** When I convert from FAST5 to BLOW5 and then reconvert to FAST5, why are the new FAST5 files are different in size to original FAST5 files?

This is normal behaviour and be assured that difference in sizes does not mean that data has been lost. Please see the issues [#70](https://github.com/hasindu2008/slow5tools/issues/70), [#76](https://github.com/hasindu2008/slow5tools/issues/76) and [#58](https://github.com/hasindu2008/slow5tools/issues/58).

**Q7** Can I upload my BLOW5 files to public archives like ERA or SRA?

Yes you can. Follow the same method you use to upload unbasecalled FAST5 files. As of July 2024, my personal preference is ENA, as the process is more streamlined.

For ENA: Create a tar.gz containing your BLOW5 file/files (optionally .blow5.idx files as well), use `OxfordNanopore_native` under the ENA submission format and upload as you would usually do. A suggested command is `tar cvf - sample/reads.blow5 sample/reads.blow5.idx | pigz -0 - > data.tar.gz`, where pigz is a parallel version of gzip and allows creating .tar.gz files with 0 compression level (no compression) which is preferred as compressing already compressed BLOW5 files is a waste of compute time for both compression and decompression. If you want simply to rely on gzip, an example command is `GZIP=-1 tar zcvf data.tar.gz sample/reads.blow5 sample/reads.blow5.idx`, but the minimum compression level allowed in gzip is 1. I submit fastq data and raw signal data as two separate ENA runs associated to the same BioSample (e.g. [ERR11768771](https://www.ebi.ac.uk/ena/browser/view/ERR11768771) and [ERR11768584](https://www.ebi.ac.uk/ena/browser/view/ERR11768584)). See this [beautiful blog article](https://naturepoker.wordpress.com/2024/07/19/nanopore-archiving-signals/) written by [@naturepoker](https://x.com/naturepoker1) that gives a step-by-step walkthrough on how to submit BLOW5 files to ENA.

For SRA: Create a tar ball containing your BLOW5 file/files (optionally .blow5.idx files as well; e.g.: `tar cvf data.tar sample/reads.blow5 sample/reads.blow5.idx`), select ONT_NATIVE under the SRA submission format and upload as you would usually do. When the loading fails or the submission seem to take forever to process (also happens for latest FAST5 as ONT does not store basecalls in FAST5 now), email the SRA helpdesk and they will complete your submission (they call it provisional loading). I submit fastq data and raw signal data as two separate SRA runs associated to the same BioSample (e.g. [SRR15058167](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR15058167&display=data-access) and [SRR2218640](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR22186402&display=data-access)) as suggested by SRA helpdesk.


**Q8** Does Oxford Nanopore Technologies officially support S/BLOW5 format?

SLOW5 is not formally supported by ONT, instead it is a community-centric format maintained by the Garvan Institute team designed to address [community requirements such as fast analysis performance, simplicty/backward compatibility and file size for long term archiving](https://hasindu2008.github.io/slow5specs/design.html), where as ONT's official formats focus on the instruments' writing performance. ONT provides freedom for the community to select what suits us best: ["What happens after that or ‘off instrument’ is much more a matter for users and using slow5 or anything else is really in their hands. We have conversion tools, and switches."]() - Clive G. Brown, CTO of ONT.

**Q9** How to compile slow5tools for Apple Silicon (e.g., Mac M1)

Without zstd support:
```
brew install hdf5 zlib
./configure LDFLAGS=-L/opt/homebrew/lib/ CPPFLAGS=-I/opt/homebrew/include/
make
```

With zstd support:
```
brew install hdf5 zlib
scripts/install-zstd.sh        # download and compiles zstd in the current folder
./configure --enable-localzstd LDFLAGS=-L/opt/homebrew/lib/ CPPFLAGS=-I/opt/homebrew/include/
make
```

**Q10** Why is there a size difference between the original FAST5 file and the file created using `s2f`?

The explanation lies in the complex structure of the HDF5 format underpinning FAST5 files. Unlike a BLOW5 file, which will always be the same size, an HDF5/FAST5 file may differ considerably depending on how it was generated. We have found that ONT’s MinKNOW software (through which almost all FAST5 files are created) creates files with large quantities of ‘unaccounted’ space, which typically inflates the file sizes by 10-20%. We don’t know why this happens, because MinKNOW is closed software, but it is highly reproducible. It is one of MinKNOW’s many mysteries. When converting from FAST5 -> BLOW5 -> FAST5, this ‘unaccounted’  space is removed by slow5tools because it serves no purpose. No data is lost; the final file is simply packed more efficiently than the original. You may also refer to [#50](https://github.com/hasindu2008/slow5tools/issues/50), [#70](https://github.com/hasindu2008/slow5tools/issues/70) and [#76](https://github.com/hasindu2008/slow5tools/issues/76) for more information.
