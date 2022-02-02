#Real-time FAST5 to SLOW5 conversion

This can be used on your computer where you are doing the sequencing acquisition, for instance, the laptop connected to the MinION and running MinKNOW.


## Limitations
- Only works for Linux at the moment (Does not work on WSL).
- Does not retain the directory structure for fast5

## Pre-requisites
- inotify-tools:  `sudo apt install inotify-tools`


## Running

Assume your sequencing data directory is `/data` and you are sequencing a sample called 'abc123' on to '/data/abc123'

./run.sh -m /data/abc123
