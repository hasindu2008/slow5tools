# Frequently Asked Questions

#### When trying to split a slow5 file I get "The file XX contains multiple read groups. You must first separate read groups using -g". How can I split a slow5 file with multiple read groups such that one file contains 4000 reads?

Imagine having a XX blow5 file with two read groups. You then first have to create two separate files one with a single group.
 
`slow5tools split -g XX -o out_dir1`

The `out_dir1` should now have two files. To create slow5s such that one contains only 4000 reads and to store them in `out_dir2`,

`slow5tools split -r 4000 out_dir1 -o out_dir2`

The number of files in `out_dir2` depends on how many number of reads were present in each single group file.