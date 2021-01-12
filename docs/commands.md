# Commands and Options

## COMMANDS 

* `fast2slow`:               
         Converts from FAST5 to SLOW5/BLOW5.
* `slow2fast`:    
         Converts from SLOW5/BLOW5 to FAST5.
* `index`:           
         Indexes a SLOW5/BLOW5 file.
* `view`:          
         SLOW5<->BLOW5 conversion.   
* `merge`:          
         Merge multiple SLOW5/BLOW5 files to a single file.
* `split`:          
         Splits a SLOW5/BLOW5 file.
* `get`:          
         Get records for specified read IDs.         
* `stats`:
         Generates Statistics from a SLOW5/BLOW5 file.

*  `--version`:
    Print the version number to the standard out. 

## OPTIONS

### fast2slow

`slow5tools fast2slow [OPTIONS] fast5_dir1/file1.fast5 ... >  out.slow5`

Recursively searches for FAST5 files (.fast5 extension) in specified directories and converts them to SLOW5/BLOW5 format. FAST5 files also can be provided directly as arguments instead of directories.

*  `-s, --slow5`:
   Outputs in text-based SLOW5 format.
*  `-b, --blow5 compression_type`:
   Outputs in BLOW5 format. `compression_type` can be `bin` for uncompressed binary, `gzip` or gzip-based compression.
*  `-c INT`, `--compress INT`:
   Outputs compressed BLOW5 at compression level specified by INT (compression levels 1 to 9 as in gzip). This option is in-efective if `-s` is specified or `-b bin`.
*  `-h, --help`:                           
   Prints the help to the standard out.
*  `-i FILE`, `--index FILE`
   Generates SLOW5/BLOW5 index.
*  `-o FILE`, `--output FILE`:
   Outputs converted contents to FILE [default value: stdout]
*  `-p, --iop INT`:
    Number of I/O processes [default value: 8]. Increasing the number of I/O processes makes conversion significantly faster, especially on HPC with RAID systems (multiple disks) where this can be as high as 64. 
*   `--lossy`:
    Discard useless information in FAST5.
*  `--no-merge DIR`:
    Convert each FAST5 file to a separate SLOW5/BLOW5 and write to the directory specified by DIR. `-o` is ineffective with this option.
*  `--no-recursion`:
    Do not recursively search for FAST5 files in specified directories.   
*  `--tmp-prefix` STR:
    Write temporary files to STR.nnnn.blow5 [default value: ./tmp]
*  `--verbose INT`:
    Verbosity level for the log messages [default value: 0].



### slow2fast

`slow5tools slow2fast [OPTIONS] file1.slow5/file1.blow5/dir1 ... -o fast5_dir`

*  `-h`, `--help`:                           
   Prints the help to the standard out.
*  `K`, `--batchsize`: 
   Number of reads to write into one FAST5 file [default value: 4000]
*  `-p, --iop INT`:
   Number of I/O processes [default value: 8]. Increasing the number of I/O processes makes conversion significantly faster, especially on HPC with RAID systems (multiple disks) where this can be as high as 64.  
*   `-o DIR`
   Output directory where the FAST5 files will be written.      
*  `--verbose INT`:
    Verbosity level for the log messages [default value: 0].

    
### merge

`slow5tools merge [OPTIONS] file1.slow5 ...`

Merges multiple SLOW5/BLOW5 files into one SLOW5/BLOW5 file. If multiple samples are detected, the header and the *read_group* field will be modified accordingly.
*  `-s, --slow5`:
   Outputs in text-based SLOW5 format.
*  `-b, --blow5 compression_type`:
   Outputs in BLOW5 format. `compression_type` can be `bin` for uncompressed binary, `gzip` or gzip-based compression.
*  `-c INT`, `--compress INT`:
   Outputs compressed BLOW5 at compression level specified by INT (compression levels 1 to 9 as in gzip). This option is in-efective if `-s` is specified or `-b bin`.
*  `-o FILE`, `--output FILE`:
   Outputs converted contents to FILE [default value: stdout]

### Split

Split a SLOW5/BLOW5 file into multiple SLOW5/BLOW5 files. Useful for parallelising accross array jobs / distributed systems.
*  `-s, --slow5`:
   Outputs in text-based SLOW5 format.
*  `-b, --blow5 compression_type`:
   Outputs in BLOW5 format. `compression_type` can be `bin` for uncompressed binary, `gzip` or gzip-based compression.
*  `-c INT`, `--compress INT`:
   Outputs compressed BLOW5 at compression level specified by INT (compression levels 1 to 9 as in gzip). This option is in-efective if `-s` is specified or `-b bin`.
*  `-o DIR`, `--output DIR`:
   Output directory where the split files will be written.  
*  `K`, `--batchsize`: 
         

- based on the read group
- based on list containing readID and filename pairs
- max number of reads per file basis

    
    
