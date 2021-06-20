# Example workflows

## To convert a FAST5 dataset to a single BLOW5 file and then convert back to FAST5

```
slow5tools f2s fast5_dir -d blow5_dir  -p 8
slow5tools merge blow5_dir -o file.blow5 -t8
rm -rf  blow5_dir
slow5tools split file.blow5 -d blow5_dir -r 4000
slow5tools s2f blow5_dir -d fast5  -p 8
```
