# Commands and Options

## COMMANDS

* `f2s`:
         Convert FAST5 files to SLOW5/BLOW5 format.
* `s2f`:
         Convert SLOW5/BLOW5 files to FAST5 format.
* `view`:
         View the contents of a SLOW5/BLOW5 file or convert between different SLOW5/BLOW5 formats (ASCII, binary, compressed).
* `index`:
         Create an index for a SLOW5/BLOW5 file.        
* `merge`:
         Merge multiple SLOW5/BLOW5 files to a single file.
* `split`:
         Split a single a SLOW5/BLOW5 file into multiple separate files.
* `get`:
         Retrieve records for specified read IDs from a SLOW5/BLOW5 file.
* `stats`:
         Generate summary statistics describing a SLOW5/BLOW5 file.



### f2s

`slow5tools f2s [OPTIONS] file.fast5 -o file.blow5 -p 1`

`slow5tools f2s [OPTIONS] fast5_dir1/file1.fast5 fast5_dir2/file2.fast5 ... -d output_dir`

Use this tool to convert FAST5 files to SLOW5/BLOW5 format.
The tool recursively searches the specified input directories for FAST5 files (.fast5 extension) and converts them to SLOW5/BLOW5.
For each multi-FAST5 file in the input directories, a SLOW5/BLOW5 file with the same file name will be created inside the output directory (specified with `-d`).
If single-FAST5 files are provided as input, a single SLOW5/BLOW5 file will be created for each process used during conversion (specified with `-p`).
It is not recommended to run f2s on a mixture of both multi-FAST5 and single-FAST5 files in a single command.

*  `-b STR , --to format_type`:
   Specifies the format of output files. `format_type` can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: blow5].
*  `-c, --compress compression_type`:
   Specifies the compression method used for BLOW5 output. `compression_type` can be `none` for uncompressed binary or `gzip` for gzip-based compression [default value: gzip]. Assumes `-b blow5`.
*  `-d STR, --out-dir STR`:
   Specifies name/location of the output directory. If a name is provided, a directory will be created under the current working directory. Alternatively, a valid relative or absolute path can be provided. To prevent data overwriting, the program will terminate with error if the directory name already exists and is non-empty.
<!--
*  `-c INT`, `--compress INT`:
   Specifies the compression level of BLOW5 output files (compression levels 1 to 9 as in gzip). Assumes `-b blow5` and `-c gzip`.
-->
*  `-h, --help`:
   Prints the help menu.
<!--
*  `-i FILE`, `--index FILE`
   Create a SLOW5/BLOW5 index for each output file. Assumes `-b blow5`.
-->
*  `-o FILE`, `--output FILE`:
   Outputs converted contents to FILE [default value: stdout]. Incompatible with `-d` and must be used with `-p 1` 
*  `-p, --iop INT`:
    Specifies the number of I/O processes to use during conversion [default value: 8]. Increasing the number of I/O processes makes f2s significantly faster, especially on HPC with RAID systems (multiple disks), where up to 64 processes can be used.
*   `-l`,`--lossy`:
    Discard information in auxilliary fields during FAST5 to SLOW5 conversion. This information is generally not required for downstream analysis and will be discarded by default to reduce filesize.
<!--
*  `--no-merge DIR`:
    Convert each FAST5 file to a separate SLOW5/BLOW5 file and write to the directory specified by DIR. `-o` is ineffective with this option.
*  `--no-recursion`:
    Do not recursively search for FAST5 files in specified directories.
-->
* `-a, --allow`:
   Allow run id mismatches in a multi-fast5 file or inside a directory of single-fast5 files, i.e., FAST5 files from different samples can be converted in a single command.
*  `-h`, `--help`:
   Prints the help to the standard out.

### merge

`slow5tools merge [OPTIONS] dir1/file1.slow5 dir2/file2.slow5 ...`

Merges multiple SLOW5/BLOW5 files into one SLOW5/BLOW5 file. If multiple samples (different run ids) are detected, the header and the *read_group* field will be modified accordingly.

*  `-b STR , --to STR`:
   Output in the format specified in STR which can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: blow5].
*  `-c, --compress compression_type`:
   Outputs the compression method for BLOW5 output. `compression_type` can be `none` for uncompressed binary, `gzip` for gzip-based compression. This option is only effective with -t blow5 [default value: gzip].
*  `-o FILE`, `--output FILE`:
   Outputs converted contents to FILE [default value: stdout]
*  `--tmp-prefix` STR:
    Write temporary files to the directory specified by STR [default value: ./slow5_timestamp_pid]. If a name is provided, a directory will be created under the current working directory. Alternatively, a relative or absolute path can be provided, as long as the immediate parent directory exists (e.g., if /path/to/foo is given, /path/to should already exist).  For prevent overwriting your data, the program will terminate with error if the provided directory name already exists and is non-empty.
* `-t, --threads INT`:
   Number of threads
*   `-l`,`--lossy`:
   Discard auxilliary field information in FAST5.
*  `-h`, `--help`:
   Prints the help to the standard out.
   

### index

`slow5tools index [OPTIONS] file1.slow5/file1.blow5`

Generates an index for a SLOW5/BLOW5 file.

*  `-h`, `--help`:
   Prints the help to the standard out.

### view

View a SLOW5 file. Can be used to convert back and forth between ASCII and binary and also with and without compression.

`./slow5tools view [OPTIONS] file.slow5/file.blow5`

*  `-f STR, --from STR`        
      Specify input file format. STR can be slow5 or blow5 [Default: autodetected based on the file extension otherwise].
*  `-b STR, --to STR`          
      Specify output file format. STR can be slow5 or blow5. [Default: slow5]     
*  `-c, --compress compression_type`:
      Outputs the compression method for BLOW5 output. `compression_type` can be `none` for uncompressed binary, `gzip` for gzip-based compression. This option is only effective 
*  `-o STR, --output STR`        
      output to file specified by STR. [default: stdout]
*  `-h`, `--help`:
   Prints the help to the standard out.

*TODO: if '-' is specified as the file, read from stdin*
*TODO: view specified field only*
*TODO: view header only*

### get

`slow5tools get [OPTIONS] file1.slow5/file1.blow5 < readids.txt`
`slow5tools get [OPTIONS] file1.slow5/file1.blow5 readid1 ....`

* `-t, --threads INT`:
   Number of threads
* `-K, --batchsize`
   The batch size
*  `-h`, `--help`:
   Prints the help to the standard out.
   

Get records for specified read IDs.


### split

`slow5tools split [OPTIONS] file1.slow5/file1.blow5 -o out_dir`

Split a SLOW5/BLOW5 file into multiple SLOW5/BLOW5 files. Useful for parallelising accross array jobs / distributed systems.

*  `-b STR , --to STR`:
   Output in the format specified in STR which can be `slow5` for SLOW5 ASCII or `blow5` for SLOW5 binary (BLOW5) [default value: blow5].
*  `-c, --compress STR`:
   Outputs the compression method for BLOW5 output. `STR` can be `none` for uncompressed binary, `gzip` for gzip-based compression. This option is only effective with -t blow5 [default value: gzip].
*  `-d STR`, `--out-dir STR`:
   Output directory where the split files will be written. If a name is provided, a directory will be created under the current working directory. Alternatively, a relative or absolute path can be provided, as long as the immediate parent directory exists (e.g., if /path/to/foo is given, /path/to should already exist).  For prevent overwriting your data, the program will terminate with error if the provided directory name already exists and is non-empty.
*  `f, --files INT`:
   Split such that N (as specified in INT) number of files  are created with all having equal number of reads. Note that, this option works only for slow5 files with a single read group. Cannot be used together with -r. You can run with -r first to split each read groups into separate files and subsequently with -n on each file.
*  `r, --reads INT`:
   Split such that files are created with each having N (as specified in INT) number of reads. Note that, this option works only for slow5 files with a single read group. Cannot be used together with -r. You can run with -r first to split each read groups into separate files and subsequently with -n on each file.
*  `g, --groups`:
   Split such that each read group (sample/run id) goes into a different file. 
*   `-l`,`--lossy`:
    Discard auxilliary field information in FAST5.
*  `-p, --iop INT`:
   Number of I/O processes [default value: 8]. Increasing the number of I/O processes makes conversion significantly faster, especially on HPC with RAID systems (multiple disks)
*  `-h`, `--help`:
   Prints the help to the standard out.
<!--
*  `n, --num-reads INT`:
   Split such that n reads are put onto a single SLOW5/BLOW5 file (based on order they appear in the original file). 
*  `l, --list FILE`:
   Split as per the mappings given in file containing a list of readID and filename pairs.
-->


### s2f

`slow5tools slow2fast [OPTIONS] file1.slow5/file1.blow5/dir1 ... -o fast5_dir`

Convert SLOW5/BLOW5 into FAST5 files. One FAST5 per SLOW5/BLOW5.

*  `-h`, `--help`:
   Prints the help to the standard out.
*  `n`, `--num-reads`:
   Number of reads to write into one FAST5 file [default value: 4000]. not implemented (todo). Use split --reads option to first split reads.
*  `-p, --iop INT`:
   Number of I/O processes [default value: 8]. Increasing the number of I/O processes makes conversion significantly faster, especially on HPC with RAID systems (multiple disks) where this can be as high as 64.
*   `-d STR`, `--out-dir STR`:
   Output directory where the FAST5 files will be written. If a name is provided, a directory will be created under the current working directory. Alternatively, a relative or absolute path can be provided, as long as the immediate parent directory exists (e.g., if /path/to/foo is given, /path/to should already exist).  For prevent overwriting your data, the program will terminate with error if the provided directory name already exists and is non-empty.




### stats

`slow5tools stats [OPTIONS] file1.slow5/file1.blow5`

Get statistics of a SLOW5/BLOW5 file and prints to the stdout.

if SLOW5 or BLOW5
The compression technique and compression level if applicable
Number of read groups
Total number of reads
Number of reads from each group
and whatever is useful.


## GLOBAL OPTIONS

*  `-V, --version`:
    Print the version number to the standard out.
*  `-v INT, --verbose INT`:
    Set the verbosity level (0-7) [default value: 3]. 0-off, 1-errors, 2-warnings, 3-information, 4-verbose, 5-gossip, 6-debug, 7-trace.
*  `-h, --help`:
    Prints the help to the standard out.
