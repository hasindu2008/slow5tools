# Commands and Options

## COMMANDS

* `f2s`:
         Converts from FAST5 to SLOW5/BLOW5.
* `merge`:
         Merge multiple SLOW5/BLOW5 files to a single file.
* `index`:
         Indexes a SLOW5/BLOW5 file.
* `view`:
         SLOW5<->BLOW5 conversion.
* `split`:
         Splits a SLOW5/BLOW5 file.
* `s2f`:
         Converts from SLOW5/BLOW5 to FAST5.
* `get`:
         Get records for specified read IDs.
* `stats`:
         Generates Statistics from a SLOW5/BLOW5 file.



### f2s

`slow5tools f2s [OPTIONS] fast5_dir1 fast5_dir2 ... -d output_dir`

Recursively searches for FAST5 files (.fast5 extension) in specified directories and converts them to SLOW5/BLOW5 format. Do not mix multi-FAST5 and single-FAST5 files in a single command (is this correct Hiruna?). For each multi-FAST5 file in provided input directories, a SLOW5/BLOW5 file with the same file name will be created inside rge directory specified by `-d`. If single-FAST5 files are provided as input, a SLOW5/BLOW5 file will be created one for each process (specified by -p below).
What happens if we provide a.fast5 and b.fast5 instead of fast5_dir1 and fast5_dir2 hiruna?

*  `-t STR , --to STR`:
   Output in the format specified in STR which can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: BLOW5].
*  `-c, --compress compression_type`:
   Outputs the compression method for BLOW5 output. `compression_type` can be `none` for uncompressed binary, `gzip` for gzip-based compression. This option is only effective with -t blow5 [default value: gzip]..
*  `-d STR, --out-dir STR`:
   The output directory name/location. If a name is provided, a directory will be created under the current working directory. Alternatively, a relative or absolute path can be provided, as long as the immediate parent directory exists (e.g., if /path/to/foo is given, /path/to should already exist).  For prevent overwriting your data, the program will terminate with error if the provided directory name already exists and is non-empty.
<!--
*  `-c INT`, `--compress INT`:
   Outputs compressed BLOW5 at compression level specified by INT (compression levels 1 to 9 as in gzip). This option is in-efective if `-s` is specified or `-b bin`.
-->
*  `-h, --help`:
   Prints the help to the standard out.
<!--
*  `-i FILE`, `--index FILE`
   Generates SLOW5/BLOW5 index.
*  `-o FILE`, `--output FILE`:
   Outputs converted contents to FILE [default value: stdout]
-->
*  `-p, --iop INT`:
    Number of I/O processes [default value: 8]. Increasing the number of I/O processes makes conversion significantly faster, especially on HPC with RAID systems (multiple disks) where this can be as high as 64.
*   `-l`,`--lossy`:
    Discard auxilliary field information in FAST5.
<!--
*  `--no-merge DIR`:
    Convert each FAST5 file to a separate SLOW5/BLOW5 and write to the directory specified by DIR. `-o` is ineffective with this option.
*  `--no-recursion`:
    Do not recursively search for FAST5 files in specified directories.
-->



### merge

`slow5tools merge [OPTIONS] dir1/file1.slow5 dir2/file2.slow5 ...`

Merges multiple SLOW5/BLOW5 files into one SLOW5/BLOW5 file. If multiple samples are detected, the header and the *read_group* field will be modified accordingly.
*  `-s, --slow5`:
   Outputs in text-based SLOW5 format.
*  `-b, --blow5 compression_type`:
   Outputs in BLOW5 format. `compression_type` can be `bin` for uncompressed binary, `gzip` or gzip-based compression.
*  `-c INT`, `--compress INT`:
   Outputs compressed BLOW5 at compression level specified by INT (compression levels 1 to 9 as in gzip). This option is in-efective if `-s` is specified or `-b bin`.
*  `-o FILE`, `--output FILE`:
   Outputs converted contents to FILE [default value: stdout]
*  `--tmp-prefix` STR:
    Write temporary files to the directory specified by STR [default value: ./slow5_timestamp_pid]. Same conditions as for `-d` applies.

### s2f

`slow5tools slow2fast [OPTIONS] file1.slow5/file1.blow5/dir1 ... -o fast5_dir`

*  `-h`, `--help`:
   Prints the help to the standard out.
*  `n`, `--num-reads`:
   Number of reads to write into one FAST5 file [default value: 4000]
*  `-p, --iop INT`:
   Number of I/O processes [default value: 8]. Increasing the number of I/O processes makes conversion significantly faster, especially on HPC with RAID systems (multiple disks) where this can be as high as 64.
*   `-o DIR`
   Output directory where the FAST5 files will be written.
*  `--verbose INT`:
    Verbosity level for the log messages [default value: 0].


### split

`slow5tools split [OPTIONS] file1.slow5/file1.blow5 -o out_dir`

Split a SLOW5/BLOW5 file into multiple SLOW5/BLOW5 files. Useful for parallelising accross array jobs / distributed systems.

*  `-s, --slow5`:
   Outputs in text-based SLOW5 format.
*  `-b, --blow5 compression_type`:
   Outputs in BLOW5 format. `compression_type` can be `bin` for uncompressed binary, `gzip` or gzip-based compression.
*  `-c INT`, `--compress INT`:
   Outputs compressed BLOW5 at compression level specified by INT (compression levels 1 to 9 as in gzip). This option is in-efective if `-s` is specified or `-b bin`.
*  `-o DIR`, `--output DIR`:
   Output directory where the split files will be written.
*  `n, --num-reads INT`:
   Split such that n reads are put onto a single SLOW5/BLOW5 file (based on order they appear in the original file)
*  `r, --read-groups`:
   Split such that each read group goes into a different file
*  `l, --list FILE`:
   Split as per the mappings given in file containing a list of readID and filename pairs.
*  `-p, --iop INT`:
   Number of I/O processes [default value: 8]. Increasing the number of I/O processes makes conversion significantly faster, especially on HPC with RAID systems (multiple disks)



### index

`slow5tools index [OPTIONS] file1.slow5/file1.blow5`

Generates an index for a SLOW5/BLOW5 file.
* `-t, --threads INT`:
   Number of threads

### get

`slow5tools get [OPTIONS] file1.slow5/file1.blow5 < readids.txt`
`slow5tools get [OPTIONS] file1.slow5/file1.blow5 readid1 ....`

* `-t, --threads INT`:
   Number of threads
* `-K, --batchsize`
   The batch size

Get records for specified read IDs.


### stats

Get statistics of a SLOW5/BLOW5 file
Should print if SLOW5 or BLOW5
The compression technique and compression level if applicable
Number of read groups
Total number of reads
Number of reads from each group


## GLOBAL OPTIONS

*  `-V, --version`:
    Print the version number to the standard out.
*  `-v INT, --verbose INT`:
    Set the verbosity level (0-7) [default value: 3]. 0-off, 1-errors, 2-warnings, 3-information, 4-verbose, 5-gossip, 6-debug, 7-trace.
*  `-h, --help`:
    Prints the help to the standard out.
