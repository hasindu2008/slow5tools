# Frequently Asked Questions

**Question:** When trying to split a slow5 file I get "The file XX contains multiple read groups. You must first separate read groups using -g". How can I split a slow5 file with multiple read groups such that one file contains 4000 reads?

**Answer:** 

For example, if you had a XX blow5 file with two read groups. You first need to create two separate files using `split`, where each new file only contains one read group.

`slow5tools split -g XX -o out_dir1`

The `out_dir1` should now have two files. Now to create slow5s such that each file contains 4000 reads and to store them in `out_dir2`,

`slow5tools split -r 4000 out_dir1 -o out_dir2`

The number of slow5 files in `out_dir2` depends on how many number of reads were present in each single group file, but each will now contain 4000 reads, except the last file, which will contain whatever is left over for that read group

##
**Q2:** `slow5tools get` or `int slow5_get(const char *read_id, slow5_rec_t **read, slow5_file_t *s5p)` fails to fetch a record. 

**Answer:** 

This is the normal behaviour if the slow5 file does not have the querying record. If you are certain that the record should not be missing, delete the slow5 index file and try again. If the slow5 file was replaced with a different slow5 file but has the same name, the old index should be deleted.
