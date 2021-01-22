// open slow5 file
// get read entry from id

slow5_file *fp = slow5_open("eg.slow5", "r");

slow5_read *read_entry1 = slow5_get("read_laks0293408", fp);
slow5_read *read_entry2 = slow5_get("read_aslkdj20398", fp);
slow5_read_print(read_entry2); // slow5_read_fprint(read_entry2, stdout);

slow5_close(fp);
slow5_read_free(read_entry1);
slow5_read_free(read_entry2);

// open slow5 file
// get read entries from ids

slow5_file *fp = slow5_open("eg.slow5", "r");

char *reads[] = { "readx", "ready", "readz" };
uint64_t n = 3;
slow5_read **read_entries = slow5_get_multi(reads, n, fp); // slow5_read **read_entries = slow5_get(reads, n, fp);

slow5_close(fp);
slow5_read_free(read_entry1);
slow5_read_free(read_entry2);

// merge both two slow5 files

slow5_file *fp1 = slow5_open("eg1.slow5", "r");
slow5_file *fp2 = slow5_open("eg2.slow5", "r");

slow5_file *fpout = slow5_open("egout.slow5", "w");
slow5_merge(fp1, fp2, fpout);

slow5_close(fp1);
slow5_close(fp2);
slow5_close(fpout);

// merge both two slow5 files. write to stdout

slow5_file *fp1 = slow5_open("eg1.slow5", "r");
slow5_file *fp2 = slow5_open("eg2.slow5", "r");

slow5_file *fpout = slow5_init_fp(stdout, FORMAT_SLOW5);
slow5_merge(fp1, fp2, fpout);

fclose(fp1);
fclose(fp2);

// merge both two slow5 files. write to fd

slow5_file *fp1 = slow5_open("eg1.slow5", "r");
slow5_file *fp2 = slow5_open("eg2.slow5", "r");

slow5_file *fpout = slow5_init_fd(1, FORMAT_SLOW5);
slow5_merge(fp1, fp2, fpout);

fclose(fp1);
fclose(fp2);

// convert slow5 to blow5 file

slow5_file *fp1 = slow5_open("eg1.slow5", "r");
slow5_file *fp2 = slow5_open("eg1.blow5", "w");

slow5_write(fp1, fp2);

slow5_close(fp1);
slow5_close(fp2);

// convert slow5 to two blow5 files with different compression

slow5_file *fp1 = slow5_open("eg1.slow5", "r");
slow5_file *fp2 = slow5_open_with("eg1.blow5", "w+", COMPRESS_NONE);
slow5_file *fp3 = slow5_init_fp(stdout, "w", FORMAT_BINARY | COMPRESS_GZIP);

slow5_write(fp1, fp2);
slow5_write(fp1, fp3); // slow5_write(fp2, fp3); should work too

slow5_close(fp1);
slow5_close(fp2);
slow5_free(fp3);

// convert fast5 to slow5 file

slow5_file *out = slow5_open("new.blow5", "w");

fast5_to_slow5("fast5_files/", out);

slow5_close(out);

// convert slow5 to fast5 files

slow5_file *in = slow5_open("in.slow5", "r");

slow5_to_fast5(in, "fast5_files/");

slow5_close(in);

// split a slow5 file by read group

slow5_file *in = slow5_open("in.slow5", "r");

slow5_split(in, "slow5_files/");

slow5_close(in);

// index a slow5 file

slow5_file *in = slow5_open("eg.slow5", "r");
slow5_index(in);
slow5_close(in);
