# Bash One-liners with slow5tools


```
# print read records without the header:
slow5tools view file.blow5 | grep -v '^[#@]'

# print the list of read IDs:
slow5tools view file.blow5 | grep -v '^[#@]' | awk '{print $1}'

# print the raw-signal for the read-id r1
slow5tools view file.blow5 | awk '{if ($1="r1") {print $8; exit;}}'
```
