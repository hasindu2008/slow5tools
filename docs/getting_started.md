# slow5tools

Slow5tools is a simple toolkit for converting (FAST5 <-> SLOW5), compressing, viewing, indexing and manipulating data in SLOW5 format.

## About SLOW5 format

SLOW5 is a new file format for signal data from Oxford Nanopore Technologies (ONT) devices. SLOW5 was developed to overcome inherent limitations in the standard FAST5 data format that prevent efficient, scalable analysis and cause many headaches for developers.

SLOW5 is a simple tab-separated values (TSV) file encoding metadata and time-series signal data for one nanopore read per line, with global metadata stored in a file header. Parallel file access is facilitated by an accompanying index file, also in TSV format, that specifies the position of each read (in Bytes) within the main SLOW5 file. SLOW5 can be encoded in human-readable ASCII format, or a more compact and efficient binary format (BLOW5) - this is analogous to the seminal SAM/BAM format for storing DNA sequence alignments. The BLOW5 binary format can be compressed using standard gzip compression, thereby minimising the data storage footprint while still permitting efficient parallel access.

Detailed benchmarking experiments have shown that SLOW5 format is up to X-fold faster and X% smaller than FAST5 [SLOW5 paper].

## Quick start

If you are a Linux user and want to quickly try slow5tools out, download the compiled binaries from the [latest release](https://github.com/hasindu2008/slow5tools/releases). For example:
```sh
VERSION=v0.0-alpha
wget "https://github.com/hasindu2008/slow5tools/releases/download/$VERSION/slow5tools-$VERSION-binaries.tar.gz" && tar xvf slow5tools-$VERSION-binaries.tar.gz && cd slow5tools-$VERSION/
./slow5tools_x86_64_linux
```
Binaries should work on most Linux distributions and the only dependency is `zlib` which is available by default on most distros.

## Building

### Building a release


Users are recommended to build from the  [latest release](https://github.com/hasindu2008/slow5tools/releases) tar ball. Quick example for Ubuntu :
```sh
sudo apt-get install libhdf5-dev zlib1g-dev   #install HDF5 and zlib development libraries
VERSION=v0.0-alpha
wget "https://github.com/hasindu2008/slow5tools/releases/download/$VERSION/slow5tools-$VERSION-release.tar.gz" && tar xvf slow5tools-$VERSION-release.tar.gz && cd slow5tools-$VERSION/
./configure
make
```
The commands to install hdf5 (and zlib) __development libraries__ on some popular distributions :
```sh
On Debian/Ubuntu : sudo apt-get install libhdf5-dev zlib1g-dev
On Fedora/CentOS : sudo dnf/yum install hdf5-devel zlib-devel
On Arch Linux: sudo pacman -S hdf5
On OS X : brew install hdf5
```

If you skip `./configure` hdf5 will be compiled locally. It is a good option if you cannot install hdf5 library system wide. However, building hdf5 takes ages.


### Building from GitHub


Building from the Github repository additionally requires `autoreconf` which can be installed on Ubuntu using `sudo apt-get install autoconf automake`. To build from GitHub:

```
sudo apt-get install libhdf5-dev zlib1g-dev autoconf automake  #install HDF5 and zlib development libraries and autotools
git clone --recursive https://github.com/hasindu2008/slow5tools
cd slow5tools
autoreconf
./configure
make
```

If you want to locally build HDF5 (takes ages) and build slow5tools against that:
```
git clone --recursive https://github.com/hasindu2008/slow5tools
cd slow5tools
autoreconf
scripts/install-hdf5.sh         # download and compiles HDF5 in the current folder
./configure --enable-localhdf5
make
```



## Usage

Visit the [man page](https://hasindu2008.github.io/slow5tools/commands.html) for all the commands and options.

### Examples

```sh
#convert a directory of fast5 files into .blow5 (compression enabled) using 8 I/O processes
slow5tools f2s fast5_dir -d blow5_dir -p 8
#convert a single fast5 file into a blow5 file(compression enabled)
slow5tools f2s file.fast5 -o file.blow5 -p 1
#merge all blow5 files in a directory into a single blow5 file using 8 threads
slow5tools merge blow5_dir -o file.blow5 -t8

#Convert a BLOW5 file into SLOW5 ASCII
slow5tools view file.blow5 --to slow5 -o file.slow5
#convert a SLOW5 file to BLOW5
slow5tools view file.slow5 --to blow5 -o file.blow5

#index a slow5/blow5 file
slow5tools index file.blow5

#extract records from a slow5/blow5 file corresponding to given read ids
slow5tools get file.blow5 readid1 readid2

#split a blow5 file into separate blow5 files based on the read groups
slow5tools split file.blow5 -d blow5_dir -r
#split a blow5 file (single read group) into separate blow5 files such that there are 4000 reads in one file
slow5tools split file.blow5 -d blow5_dir -r 4000

#convert a directory of blow5 files to fast5 using 8 I/O processes
slow5tools s2f blow5_dir -d fast5  -p 8

```

Visit [here](https://hasindu2008.github.io/slow5tools/workflows.html) for example workflows.

## Acknowledgement
slow5tools uses [klib](https://github.com/attractivechaos/klib). Some code snippets have been taken from [Minimap2](https://github.com/lh3/minimap2) and [Samtools](http://samtools.sourceforge.net/).
