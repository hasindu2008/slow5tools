# Frequently Asked Questions

**Q1:** When trying to split a slow5 file I get "The file XX contains multiple read groups. You must first separate read groups using -g". How can I split a slow5 file with multiple read groups such that one file contains 4000 reads?

For example, if you had a XX blow5 file with two read groups. You first need to create two separate files using `split`, where each new file only contains one read group.

`slow5tools split -g XX -d out_dir1`

The `out_dir1` should now have two files. Now to create slow5s such that each file contains 4000 reads and to store them in `out_dir2`,

`slow5tools split -r 4000 out_dir1 -d out_dir2`

The number of slow5 files in `out_dir2` depends on how many number of reads were present in each single group file, but each will now contain 4000 reads, except the last file, which will contain whatever is left over for that read group


**Q2:** `slow5tools get` or `int slow5_get(const char *read_id, slow5_rec_t **read, slow5_file_t *s5p)` fails to fetch a record.

This is the normal behaviour if the slow5 file does not have the querying record. If you are certain that the record should not be missing, delete the slow5 index file and try again. If the slow5 file was replaced with a different slow5 file but has the same name, the old index should be deleted.

**Q3:** The fast5 file is compressed with VBZ but the required plugin is not loaded when trying to convert fast5 to slow5

This is due to the *vbz* compression used in the latest fast5 files. You need to setup the *vbz* plugin for HDF5. We have provided a helper script in *slow5tools* which you can invoke as `scripts/install-vbz.sh`. This script attempts to determine your operating system and the architecture, downloads and extracts the plugin (.so file) to *$HOME/.local/hdf5/lib/plugin*. You must export the path to the plugin by invoking `export HDF5_PLUGIN_PATH=$HOME/.local/hdf5/lib/plugin` . Note that the exported environmental variable will not persist across different shells. For persistence, you must add `export HDF5_PLUGIN_PATH=$HOME/.local/hdf5/lib/plugin` to your *~/.bashrc* or */etc/environment*. Make sure you logout and login back for any changes to *~/.bashrc* or */etc/environment* take effect.

Our helper script currently supports Linux on x86_64 and aarch64 architectures only. If your system is of a different operating system and/or architecture (or if this helper script fails) you have to download and manually install the plugin from [here](https://github.com/nanoporetech/vbz_compression/releases).
See [this post](https://github.com/nanoporetech/vbz_compression/issues/5) for troubleshooting a manual installation.

**Q4:** Are there any small example datasets that can be used for testing slow5tools?

A tiny subset (~20K reads) of the original NA12878 PromethION dataset used for benchmaking in the [SLOW5 paper](https://www.nature.com/articles/s41587-021-01147-4) is available [here](https://slow5.page.link/na12878_prom_subsub).

The original NA12878 dataset is available on [SRA](https://www.ncbi.nlm.nih.gov/sra?linkname=bioproject_sra_all&from_uid=744329) and the table below summarises the links:

| Description                                          | SRA run Data access                                                                                        | Direct download link | MD5sum |
|------------------------------------------------------|------------------------------------------------------------------------------------------------------------|----------------------|---|
| ~500K reads subset (FAST5 format)                    | [SRR15058164](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR15058164&display=data-access) | [subsample.tar.gz](https://slow5.page.link/na12878_prom_sub)                     | 591ec7d1a2c6d13f7183171be8d31fba |
| ~500K reads subset (BLOW5 format)                    | [SRR22186403](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR22186403&display=data-access) |     [subsample_slow5.tar](https://slow5.page.link/na12878_prom_sub_slow5)                 | 6cdbe02c3844960bb13cf94b9c3173bb |
| ~9M reads complete PromethION dataset (FAST5 format) | [SRR15058166](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR15058166&display=data-access) | [fast5.tar.gz](https://slow5.page.link/na12878_prom)                     | 0adbd2956a54528e92dd8fe6d42d2fce |
| ~9M reads complete PromethION dataset (BLOW5 format) | [SRR22186402](https://trace.ncbi.nlm.nih.gov/Traces/?view=run_browser&acc=SRR22186402&display=data-access) | [slow5.tar](https://slow5.page.link/na12878_prom_slow5)                         | 3b23f706add38612445cd4f5204ae8b5 |

The direct link to the ~500K reads subset is available [here](https://slow5.page.link/na12878_prom_sub).
The direct link to the complete PromethION dataset (~9M reads) is [here](https://slow5.page.link/na12878_prom).

**Q5:** How can I make SLOW5 to FAST5 conversion fast?

If your SLOW5 to FAST5 conversion takes ages, it is likely that you called `slow5tools s2f` on a single SLOW5 file which will only use one processor. For parallelising the conversion, first split the SLOW5 file into multiple files using `slow5tools split -r` (you may have to use `slow5tools split -g` first if you BLOW5 file has multiple read groups). Then call `slow5tools s2f` with `-p <num_processes>` so that multiple CPU processors can be used for parallel conversion.

**Q6:** When I convert from FAST5 to BLOW5 and then reconvert to FAST5, why are the new FAST5 files are different in size to original FAST5 files?

This is normal behaviour and be assured that difference in sizes does not mean that data has been lost. Please see the issues [#70](https://github.com/hasindu2008/slow5tools/issues/70), [#76](https://github.com/hasindu2008/slow5tools/issues/76) and [#58](https://github.com/hasindu2008/slow5tools/issues/58).

**Q7** Can I upload my BLOW5 files to public archives like SRA?

Yes you can. Follow the same method you use to upload unbasecalled FAST5 files. Simply create a tar ball containing your BLOW5 file/files (optionally .blow5.idx files as well), select ONT_NATIVE under the SRA submission format and upload as you would usually do. When the loading fails (happens for unbasecalled FAST5 as well - and now ONT does not store basecalls on FAST5 anyway), email the SRA helpdesk and they will complete your submission.
