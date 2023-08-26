# Bash One-liners with slow5tools


## Extracting information for eye-balling and inspecting

**Note that these commands are not efficient to be run routinely on gigantic datasets, instead are only for quickly eyeballing and inspecting relatively smaller datasets.**

```
# print slow5 header:
slow5tools view file.blow5 | grep '^[#@]'
slow5tools skim --hdr file.blow5 # available from slow5tools v0.7.0

# print read records without the header:
slow5tools view file.blow5 | grep -v '^[#@]'

# print the first 10 read records (without the header):
slow5tools view file.blow5 | grep -v '^[#@]' | head -10

# print the list of read IDs:
slow5tools view file.blow5 | grep -v '^[#@]' | awk '{print $1}'
slow5tools skim --rid file.blow5 # available from slow5tools v0.7.0

# print all data columns (including the data type and column name), except the raw signal (column 8):
slow5tools view file.blow5 | sed -n '/#char*/,$p' | cut -f 1-7,9-

# print the raw-signal (column 8) for the read-id r1:
slow5tools get --to slow5 file.blow5 "r1" | grep -v '^[#@]' | awk '{print $8}'

# get statistics of raw signal length (column 7):
slow5tools view file.blow5 | grep -v '^[#@]' | datamash mean 7 median 7 sstdev 7 min 7 max 7 sum 7

# extract the signal samples 100-200 (1-indexed) for the read-id r1:
slow5tools get --to slow5 file.blow5 "r1" | grep -v '^[#@]' | awk '{print $8}' | cut -d, -f 100-200

# print the axiliary data field 'median_before' (note: slow5tools skim is available from v0.7.0)
slow5tools skim file.blow5 | awk -v c="median_before" 'NR==1{for (i=1; i<=NF; i++) if ($i==c){p=i; break};} {print $p}'

# get the mean and standard deviation of the axiliary data field 'median_before'
slow5tools skim file.blow5 | awk -v c="median_before" 'NR==1{for (i=1; i<=NF; i++) if ($i==c){p=i; break};} {print $p}' | tail -n+2 |  datamash mean 1 sstdev 1

# count how many reads come from each 'channel_number'
slow5tools skim file.blow5 | awk -v c="channel_number" 'NR==1{for (i=1; i<=NF; i++) if ($i==c){p=i; break};} {print $p}' | tail -n+2 |  sort | uniq -c
```

## Operatings on multiple files in parallel

```
# merge every 10 files together in INPUT_DIR and save to OUTPUT_DIR:
find INPUT_DIR/ -name '*.blow5' | parallel -I% --max-args 10 slow5tools merge % -o OUTPUT_DIR/{#}.blow5

# Get the sum of read counts in all BLOW5 files in a directory:
find INPUT_DIR/ -name '*.blow5' | parallel -I% --max-args 1 slow5tools stats % | grep "number of records" | awk 'BEGIN {count=0;} {count=count+$NF} END {print count;}'

```
