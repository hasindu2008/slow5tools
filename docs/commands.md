# Commands and Options

## COMMANDS

* `fast5toslow5` or `f2s`:<br/>
         Convert FAST5 files to SLOW5/BLOW5 format.
* `slow5tofast5` or `s2f`:<br/>
         Convert SLOW5/BLOW5 files to FAST5 format.
* `view`:<br/>
         View the contents of a SLOW5/BLOW5 file or convert between different SLOW5/BLOW5 formats and compressions.
* `index`:<br/>
         Create an index for a SLOW5/BLOW5 file.
* `merge`:<br/>
         Merge multiple SLOW5/BLOW5 files to a single file.
* `cat`:<br/>
		 Quickly concatenate SLOW5/BLOW5 files of same read group.
* `split`:<br/>
         Split a single a SLOW5/BLOW5 file into multiple separate files.
* `get`:<br/>
         Retrieve records for specified read IDs from a SLOW5/BLOW5 file.
* `stats`:<br/>
         Prints summary statistics describing a SLOW5/BLOW5 file.
* `skim`:<br/>
         Skims through a SLOW5/BLOW5 file and prints signal metadata.
* `quickcheck`:<br/>
         Quickly checks if a SLOW5/BLOW5 file is intact.
* `degrade`:<br/>
         Irreversibly degrade and convert a SLOW5/BLOW5 file.



### fast5toslow5 (or f2s)

```
slow5tools fast5toslow5 [OPTIONS] fast5_dir1 -d output_dir
slow5tools fast5toslow5 [OPTIONS] fast5_dir1 fast5_dir2 ... -d output_dir
slow5tools fast5toslow5 [OPTIONS] file1.fast5 file2.fast5 ... -d output_dir
slow5tools fast5toslow5 [OPTIONS] file.fast5 -o output.blow5
slow5tools fast5toslow5 [OPTIONS] file.fast5 -o output.slow5
```

Converts FAST5 files to SLOW5/BLOW5 format.
The input can be a single FAST5 file, a list of FAST5 files, a directory containing multiple FAST5 files, or a list of directories. If a directory is provided, the tool recursively searches within for FAST5 files (.fast5 extension) and converts them to SLOW5/BLOW5.
For each multi-FAST5 file in the input directories, a SLOW5/BLOW5 file with the same file name will be created inside the output directory (specified with `-d`).
If single-FAST5 files are provided as input, a single SLOW5/BLOW5 file will be created for each process used during conversion (specified with `-p`).

Note: it is not recommended to run f2s on a mixture of both multi-FAST5 and single-FAST5 files in a single command.

*  `--to format_type`:<br/>
   Specifies the format of output files. `format_type` can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: blow5].
*  `-d, --out-dir STR`:<br/>
   Specifies name/location of the output directory (required option unless converting only one FAST5 file). If a name is provided, a directory will be created under the current working directory. Alternatively, a valid relative or absolute path can be provided. To prevent data overwriting, the program will terminate with error if the directory name already exists and is non-empty.
*  `-o, --output FILE`:<br/>
   When only one FAST5 file is being converted, `-o` specifies a single FILE to which output data is written [default value: stdout]. Incompatible with `-d` and can automatically detect the output format from the file extension.
*  `-c, --compress compression_type`:<br/>
   Specifies the compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed binary; `zlib` for zlib-based (also known as gzip or DEFLATE) compression; or `zstd` for Z-standard-based compression [default value: zlib]. This option is only valid for BLOW5. `zstd` will only function if slow5tools has been built with zstd support which is turned off by default.
*  `-s, --sig-compress compression_type`:<br/>
   Specifies the raw signal compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed raw signal, `svb-zd` to compress the raw signal using StreamVByte zig-zag delta and `ex-zd` (from slow5tools v1.3.0) for exception coding [default value: svb-zd]. ex-zd offers a better compression ratio to svb-zd. This option is introduced from slow5tools v0.3.0 onwards. Note that record compression (-c option above) is still applied on top of the compressed signal. Signal compression with svb-zd and record compression with zstd is similar to ONT's vbz. zstd+svb-zd offers slightly smaller file size and slightly better performance compared to the default zlib+svb-zd, however, will be less portable.
*  `-p, --iop INT`:<br/>
    Specifies the number of I/O processes to use during conversion [default value: 8]. Increasing the number of I/O processes makes f2s significantly faster, especially on HPC with RAID systems (multiple disks) where a large value number of processes can be used (e.g., `-p 64`).
*  `--lossless STR`:<br/>
    Retain information in auxiliary fields during FAST5 to SLOW5 conversion. STR can be either `true` or `false`. [default value: true]. This information is generally not required for downstream analysis and can be optionally discarded to reduce filesize. *IMPORTANT: Generated files are only to be used for intermediate analysis and NOT for archiving. You will not be able to convert lossy files back to FAST5*.
* `-a, --allow`:<br/>
   By default f2s will not accept an individual multi-fast5 file or an individual single-fast5 directory containing multiple unique run IDs. When `-a` is specified f2s will allow multiple unique run IDs in an individual multi-fast5 file or single-fast5 directory. In this case, the header of all SLOW5/BLOW5 output files will be determined based on the first occurrence of run ID seen by f2s. This can be used to convert FAST5 files from different samples in a single command if the user does not further require the original run IDs.
*  `--retain`:<br/>
	Retain the same directory structure in the converted output as the input (experimental).
*  `-h, --help`:<br/>
   Prints the help menu.

### merge

```
slow5tools merge [OPTIONS] file1.blow5 file2.blow5  -o output.blow5
slow5tools merge [OPTIONS] blow5_dir1  -o output.blow5
slow5tools merge [OPTIONS] blow5_dir1 blow5_dir2  -o output.blow5
```

Merges multiple SLOW5/BLOW5 files to a single file.
The input can be a list of SLOW5/BLOW5 files, a directory containing multiple SLOW5/BLOW5 files, or a list of directories. If a directory is provided, the tool recursively searches within for SLOW5/BLOW5 files (.slow5/blow5 extension) and merges their contents.
If multiple samples (different run ids) are detected, the header and the *read_group* field will be modified accordingly, with each run id assigned a separate *read_group*.

*  `--to format_type`:<br/>
   Specifies the format of output files. `format_type` can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: blow5].
*  `-o, --output FILE`:<br/>
      Outputs merged data to FILE [default value: stdout]. This can auto detect the output format from the file extension.
*  `-c, --compress compression_type`:<br/>
   Specifies the compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed binary; `zlib` for zlib-based (also known as gzip or DEFLATE) compression; or `zstd` for Z-standard-based compression [default value: zlib]. This option is only valid for BLOW5. `zstd` will only function if slow5tools has been built with zstd support which is turned off by default.
*  `-s, --sig-compress compression_type`:<br/>
   Specifies the raw signal compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed raw signal, `svb-zd` to compress the raw signal using StreamVByte zig-zag delta and `ex-zd` (from slow5tools v1.3.0) for exception coding [default value: svb-zd]. ex-zd offers a better compression ratio to svb-zd. This option is introduced from slow5tools v0.3.0 onwards. Note that record compression (-c option above) is still applied on top of the compressed signal. Signal compression with svb-zd and record compression with zstd is similar to ONT's vbz. zstd+svb-zd offers slightly smaller file size and slightly better performance compared to the default zlib+svb-zd, however, will be less portable.
* `-t, --threads INT`:<br/>
   Number of threads [default value: 8].
* `-K, --batchsize INT`:<br/>
  The batch size. This is the number of records on the memory at once [default value: 4096]. An increased batch size improves multi-threaded performance at cost of higher RAM.
*   `--lossless STR`:<br/>
    Retain information in auxiliary fields during file merging [default value: true]. This information is generally not required for downstream analysis can be optionally discarded to reduce file size. *IMPORTANT: Generated files are only to be used for intermediate analysis and NOT for archiving. You will not be able to convert lossy files back to FAST5*.
* `-a, --allow`:<br/>
    Allow merging despite attribute differences in the same run_id.
*  `-h, --help`:<br/>
   Prints the help menu.

### cat

```
slow5tools cat file1.blow5 file2.blow5  -o output.blow5
slow5tools cat blow5_dir1 -o output.blow5
```

Quickly concatenate SLOW5/BLOW5 files of same type (same header, extension, compression).
Note: This subtool is is much faster than merge, but performs minimal input validation. Use with caution.

*  `-o, --output FILE`:<br/>
      Outputs concatenated data to FILE [default value: stdout].
*  `-h, --help`:<br/>
   Prints the help menu.


### index

`slow5tools index file1.blow5`

Creates an index for a SLOW5/BLOW5 file.
Input file can be in SLOW5 ASCII or SLOW5 binary (BLOW5) and can be compressed or uncompressed.

*  `-h`, `--help`:<br/>
   Prints the help menu.

### view

View the contents of a SLOW5/BLOW5 file.
This tool is also used to convert between ASCII SLOW5 and binary BLOW5 formats, or between compressed and uncompressed BLOW5 files.

`slow5tools view [OPTIONS] file.blow5`

*  `--to format_type`:<br/>
   Specifies the format of output files. `format_type` can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: slow5].
*  `-o FILE`, `--output FILE`:<br/>
   Outputs data to FILE [default value: stdout]. This can auto detect the output format from the file extension.
*  `-c, --compress compression_type`:<br/>
   Specifies the compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed binary; `zlib` for zlib-based (also known as gzip or DEFLATE) compression; or `zstd` for Z-standard-based compression [default value: zlib]. This option is only valid for BLOW5. `zstd` will only function if slow5tools has been built with zstd support which is turned off by default.
*  `-s, --sig-compress compression_type`:<br/>
   Specifies the raw signal compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed raw signal, `svb-zd` to compress the raw signal using StreamVByte zig-zag delta and `ex-zd` (from slow5tools v1.3.0) for exception coding [default value: svb-zd]. ex-zd offers a better compression ratio to svb-zd. This option is introduced from slow5tools v0.3.0 onwards. Note that record compression (-c option above) is still applied on top of the compressed signal. Signal compression with svb-zd and record compression with zstd is similar to ONT's vbz. zstd+svb-zd offers slightly smaller file size and slightly better performance compared to the default zlib+svb-zd, however, will be less portable.
* `-t, --threads INT`:<br/>
   Number of threads [default value: 8].
* `-K, --batchsize`:<br/>
   The batch size. This is the number of records on the memory at once [default value: 4096]. An increased batch size improves multi-threaded performance at cost of higher RAM.
*  `--from format_type`:<br/>
   Specifies the format of input files. `format_type` can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: autodetected based on the file extension otherwise].
*  `-h`, `--help`:<br/>
   Prints the help menu.


### get

Retrieves records for specified read IDs from a SLOW5/BLOW5 file.

```
slow5tools get [OPTIONS] file1.blow5 readid1 readid2 ....
slow5tools get [OPTIONS] file1.blow5 --list readids.txt
```

*  `--to format_type`:<br/>
    Specifies the format of output files. `format_type` can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: blow5].
*  `-o FILE`, `--output FILE`:<br/>
    Outputs data to FILE [default value: stdout]. This can auto detect the output format from the file extension.
*  `-c, --compress compression_type`:<br/>
    Specifies the compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed binary; `zlib` for zlib-based (also known as gzip or DEFLATE) compression; or `zstd` for Z-standard-based compression [default value: zlib]. This option is only valid for BLOW5. `zstd` will only function if slow5tools has been built with zstd support which is turned off by default.
*  `-s, --sig-compress compression_type`:<br/>
   Specifies the raw signal compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed raw signal, `svb-zd` to compress the raw signal using StreamVByte zig-zag delta and `ex-zd` (from slow5tools v1.3.0) for exception coding [default value: svb-zd]. ex-zd offers a better compression ratio to svb-zd. This option is introduced from slow5tools v0.3.0 onwards. Note that record compression (-c option above) is still applied on top of the compressed signal. Signal compression with svb-zd and record compression with zstd is similar to ONT's vbz. zstd+svb-zd offers slightly smaller file size and slightly better performance compared to the default zlib+svb-zd, however, will be less portable.
* `-t, --threads INT`:<br/>
    Number of threads [default value: 8].
* `-K, --batchsize`:<br/>
    The batch size. This is the number of records on the memory at once [default value: 4096]. An increased batch size improves multi-threaded performance at cost of higher RAM.
* `-l, --list FILE`:<br/>
    List of read ids provided as a single-column text file with one read id per line.
* `--index FILE`:<br/>
    Path to a custom slow5 index (experimental). Useful if your index file is located somewhere other than in the same directory as the input S/BLOW5 file.
*  `-h`, `--help`:<br/>
    Prints the help menu.


### split

```
slow5tools split [OPTIONS] file1.blow5 -d out_dir
slow5tools split [OPTIONS] blow5_dir1 -d out_dir
```

Splits a single a SLOW5/BLOW5 file into multiple separate files.
This tool is useful for parallelising across array jobs / distributed systems.

*  `--to format_type`:<br/>
    Specifies the format of output files. `format_type` can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: blow5].
*  `-d, --out-dir STR`:<br/>
    Output directory where the split files will be written. If a name is provided, a directory will be created under the current working directory. Alternatively, a valid relative or absolute path can be provided. To prevent data overwriting, the program will terminate with error if the directory name already exists and is non-empty.
*  `-c, --compress compression_type`:<br/>
    Specifies the compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed binary; `zlib` for zlib-based (also known as gzip or DEFLATE) compression; or `zstd` for Z-standard-based compression [default value: zlib]. This option is only valid for BLOW5. `zstd` will only function if slow5tools has been built with zstd support which is turned off by default.
*  `-s, --sig-compress compression_type`:<br/>
   Specifies the raw signal compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed raw signal, `svb-zd` to compress the raw signal using StreamVByte zig-zag delta and `ex-zd` (from slow5tools v1.3.0) for exception coding [default value: svb-zd]. ex-zd offers a better compression ratio to svb-zd. This option is introduced from slow5tools v0.3.0 onwards. Note that record compression (-c option above) is still applied on top of the compressed signal. Signal compression with svb-zd and record compression with zstd is similar to ONT's vbz. zstd+svb-zd offers slightly smaller file size and slightly better performance compared to the default zlib+svb-zd, however, will be less portable.
*  `-g, --groups`:<br/>
    Split the data into separate files for each read group (usually run id / sample name). The number of output files will equal the number of read groups in the input file.
*  `-r, --reads INT`:<br/>
    Split the data into files containing N reads (where N = INT). Cannot be used together with `-f` or `-g`. Note: this option works only for SLOW5/BLOW5 files with a single read group but you can run with `-g` split read groups into separate files and subsequently split each file with `-r`.
*  `-f, --files INT`:<br/>
    Split the data into n files (where n = INT) in which all files have equal numbers of reads. Cannot be used together with `-r` or `-g`. Note: this option works only for SLOW5/BLOW5 files with a single read group but you can run with `-g` split read groups into separate files and subsequently split each file with `-n`.
*  `-x, --demux TSV_PATH`:<br/>
    Split the data into separate files according to a TSV file. The TSV file has a header and at least two columns: the read IDs and the categories. Each row maps one read ID to one category, although one read ID can be mapped to multiple categories using multiple rows. The default read IDs column name is 'parent_read_id', but can be changed using the option `--demux-rid`. The default categories column name is 'barcode_arrangement', but can be changed using the option `--demux-code`. For demultiplexing barcodes, give the path to Nanopore's `sequencing_summary.txt` file or buttery-eel's `barcode_summary.txt` file and use the default column names.
*  `--demux-code STR`:<br/>
    The categories column header in the demux TSV file [default value: 'barcode_arrangement']. Use with option `--demux`.
*  `--demux-rid STR`:<br/>
    The read IDs column header in the demux TSV file [default value: 'parent_read_id']. Use with option `--demux`.
*  `-u, --demux-uniq STR`:<br/>
    Multi-category reads are only recorded once and are placed in a new category named STR. Use with option `--demux`.
*   `--lossless STR`:<br/>
    Retain information in auxilliary fields during file merging [default value: true]. This information is generally not required for downstream analysis can be optionally discarded to reduce filesize. *IMPORTANT: Generated files are only to be used for intermediate analysis and NOT for archiving. You will not be able to convert lossy files back to FAST5*.
*  `-t, --threads INT`:<br/>
   Number of threads [default value: 8].
*  `-h, --help`:<br/>
    Prints the help menu.


### slow5tofast5 (or s2f)

```
slow5tools slow5tofast5 [OPTIONS] file1.blow5 -o output.fast5
slow5tools slow5tofast5 [OPTIONS] blow5_dir1 -d fast5_dir
slow5tools slow5tofast5 [OPTIONS] file1.blow5 file2.blow5 ... -d fast5_dir
slow5tools slow5tofast5 [OPTIONS] blow5_dir1 blow5_dir2 ... -d fast5_dir
```

Converts SLOW5/BLOW5 files to FAST5 format.
The input can be a list of SLOW5/BLOW5 files, a directory containing multiple SLOW5/BLOW5 files, or a list of directories. If a directory is provided, the tool recursively searches within for SLOW5/BLOW5 files (.slow5/blow5 extension) and converts them to FAST5.
Note: Before converting a SLOW5 file having multiple read groups, split the file into groups using `split`.

*   `-d, --out-dir STR`:<br/>
    Output directory where the FAST5 files will be written. If a name is provided, a directory will be created under the current working directory. Alternatively, a valid relative or absolute path can be provided. To prevent data overwriting, the program will terminate with error if the directory name already exists and is non-empty.
*  `-o FILE`, `--output FILE`:<br/>
    Outputs data to FILE and FILE must have .fast5 extension.
*  `-p, --iop INT`:<br/>
    Specifies the number of I/O processes to use during conversion [default value: 8]. Increasing the number of I/O processes makes f2s significantly faster, especially on HPC with RAID systems (multiple disks) where a large value number of processes can be used (e.g., `-p 64`).
*  `-h, --help`:<br/>
   Prints the help menu.


### stats

`slow5tools stats file1.slow5/file1.blow5`
`slow5tools stats`

Prints summary statistics describing a SLOW5/BLOW5 file such as:

- if the file is SLOW5 or BLOW5
- compression method if applicable
- number of read groups
- total number of reads

If no argument is given, details about slow5tools is printed.

### quickcheck

Performs a quick check if a SLOW5/BLOW5 file is intact: checks if the file begins with a valid header (SLOW5 or BLOW5), attempt to decode the first SLOW5 record and then seeks to the end of the file and checks if proper EOF exists (BLOW5 only).
If the file is intact, the commands exits with 0. Otherwise it exits with a non-zero error code.

### skim

Skims through components in a SLOW5/BLOW5 file requested by user (using options) and prints to standard out. If no options are provided, all the SLOW5 fields except the raw signal will be printed to standard out. enum data types are printed as strings. This subprogramme is available form slow5tools v0.7.0 onwards.

```
slow5tools skim [OPTIONS] file.blow5
```

* `-t, --threads INT`:<br/>
    Number of threads [default value: 8].
* `-K, --batchsize INT`:<br/>
    The batch size. This is the number of records on the memory at once [default value: 4096]. An increased batch size improves multi-threaded performance at cost of higher RAM.
* `--hdr`:<br/>
    print the header only.
* `--rid`:<br/>
    print the list of read ids only.
*  `-h`, `--help`:
    Prints the help menu.


### degrade

Irreversibly degrade and convert a SLOW5/BLOW5 file.
This tool is equivalent to `view` except that it first degrades the data using the
chosen lossy compression algorithm.

`slow5tools degrade [OPTIONS] file.blow5`

See below for documentation on `degrade`-specific options. For documentation on all other options see the `view` subtool.

*  `-s, --sig-compress compression_type`:<br/>
   Specifies the raw signal compression method used for BLOW5 output. Note: the default value is ex-zd which differs in `view`.
*  `-b, --bits INT`:<br/>
   The number of least significant bits to zero then round for each raw signal data point [default value: "auto" (autodetected based on the file header and data)].


## GLOBAL OPTIONS

*  `-V, --version`:<br/>
    Print the slow5tools version number.
*  `-v INT, --verbose INT`:<br/>
    Set the verbosity level (0-6) [default value: 4]. 0-off, 1-errors, 2-warnings, 3-information, 4-verbose, 5-debug, 6-trace.
*  `-h, --help`:<br/>
    Prints the help menu.
*  `--cite`:<br/>
    Prints the citation information.
