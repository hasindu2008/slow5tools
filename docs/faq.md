# Frequently Asked Questions

**Question:** When trying to split a slow5 file I get "The file XX contains multiple read groups. You must first separate read groups using -g". How can I split a slow5 file with multiple read groups such that one file contains 4000 reads?

**Answer:**

For example, if you had a XX blow5 file with two read groups. You first need to create two separate files using `split`, where each new file only contains one read group.

`slow5tools split -g XX -d out_dir1`

The `out_dir1` should now have two files. Now to create slow5s such that each file contains 4000 reads and to store them in `out_dir2`,

`slow5tools split -r 4000 out_dir1 -d out_dir2`

The number of slow5 files in `out_dir2` depends on how many number of reads were present in each single group file, but each will now contain 4000 reads, except the last file, which will contain whatever is left over for that read group

##
**Q2:** `slow5tools get` or `int slow5_get(const char *read_id, slow5_rec_t **read, slow5_file_t *s5p)` fails to fetch a record.

**Answer:**

This is the normal behaviour if the slow5 file does not have the querying record. If you are certain that the record should not be missing, delete the slow5 index file and try again. If the slow5 file was replaced with a different slow5 file but has the same name, the old index should be deleted.

##
**Q3:** The fast5 file is compressed with VBZ but the required plugin is not loaded when trying to convert fast5 to slow5

**Answer:**
This is due to the *vbz* compression used in the latest fast5 files. You need to setup the *vbz* plugin for HDF5. We have provided a helper script in *slow5tools* which you can invoke as `scripts/install-vbz.sh`. This script attempts to determine your operating system and the architecture, downloads and extract the plugin (.so file) to *$HOME/.local/hdf5/lib/plugin*. You must export the path to the plugin by invoking `export HDF5_PLUGIN_PATH=$HOME/.local/hdf5/lib/plugin` . Note that the exported environmental variable will not persist across different shells. For persistence, you must add `export HDF5_PLUGIN_PATH=$HOME/.local/hdf5/lib/plugin` to your *~/.bashrc* or */etc/environment*.

Our helper script currently supports Linux on x86_64 and aarch64 architectures only. If your system is of a different operating system and/or architecture (or if this helper script fails) you have to download and manually install the plugin from [here](https://github.com/nanoporetech/vbz_compression/releases).
See [this post](https://github.com/nanoporetech/vbz_compression/issues/5) for troubleshooting a manual installation.
