# Bash One-liners with slow5tools


```
# print slow5 global header:
slow5tools view file.blow5 | grep '^#'

# print slow5 data header:
slow5tools view file.blow5 | grep '^@'

# print slow5 header:
slow5tools view file.blow5 | grep '^[#@]'

# print read records without the header:
slow5tools view file.blow5 | grep -v '^[#@]'

# print the first 10 read records without the header:
slow5tools view file.blow5 | grep -v '^[#@]' | head -n 10

# print the list of read IDs:
slow5tools view file.blow5 | grep -v '^[#@]' | awk '{print $1}'

# print the raw-signal for the read-id r1
slow5tools get --to slow5 file.blow5 "r1" | grep -v '^[#@]' | awk '{print $8}'

# merge every 10 files together in INPUT_DIR and save to OUTPUT_DIR 
find -name INPUT_DIR/*.[sb]low5 | parallel -I% --max-args 10 slow5tools merge % -o OUTPUT_DIR/{#}.blow5

```
