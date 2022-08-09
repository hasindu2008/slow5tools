# slow5tools

Slow5tools is a simple toolkit for converting (FAST5 <-> SLOW5), compressing, viewing, indexing and manipulating data in SLOW5 format.

**About SLOW5 format:**<br/>
SLOW5 is a new file format for storing signal data from Oxford Nanopore Technologies (ONT) devices. SLOW5 was developed to overcome inherent limitations in the standard FAST5 signal data format that prevent efficient, scalable analysis and cause many headaches for developers. SLOW5 can be encoded in human-readable ASCII format, or a more compact and efficient binary format (BLOW5) - this is analogous to the seminal SAM/BAM format for storing DNA sequence alignments. The BLOW5 binary format supports  *zlib* (DEFLATE) compression, or other compression methods (see [notes](https://github.com/hasindu2008/slow5tools#notes)), thereby minimising the data storage footprint while still permitting efficient parallel access. Detailed benchmarking experiments have shown that SLOW5 format is an order of magnitude faster and significantly smaller than FAST5.

[![GitHub Downloads](https://img.shields.io/github/downloads/hasindu2008/slow5tools/total?logo=GitHub)](https://github.com/hasindu2008/slow5tools/releases)
[![BioConda Install](https://img.shields.io/conda/dn/bioconda/slow5tools.svg?style=flag&label=BioConda%20install)](https://anaconda.org/bioconda/slow5tools)
[![CI](https://github.com/hasindu2008/slow5tools/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/hasindu2008/slow5tools/actions/workflows/c-cpp.yml)

Full documentation: https://hasindu2008.github.io/slow5tools<br/>
Pre-print: https://www.biorxiv.org/content/10.1101/2021.06.29.450255v1<br/>
Publication: https://www.nature.com/articles/s41587-021-01147-4<br/>
SLOW5 specification: https://hasindu2008.github.io/slow5specs<br/>

## Quick start

If you are a Linux user on x86_64 architecture and want to quickly try slow5tools out, download the compiled binaries from the [latest release](https://github.com/hasindu2008/slow5tools/releases). For example:
```sh
VERSION=v0.6.0
wget "https://github.com/hasindu2008/slow5tools/releases/download/$VERSION/slow5tools-$VERSION-x86_64-linux-binaries.tar.gz" && tar xvf slow5tools-$VERSION-x86_64-linux-binaries.tar.gz && cd slow5tools-$VERSION/
./slow5tools
```
Binaries should work on most Linux distributions as the only dependency is `zlib` which is available by default on most distributions. For compiled binaries to work, your processor must support SSSE3 instructions or higher (processors after 2007 have these) and your operating system must have GLIBC 2.17 or higher (Linux distributions from 2014 onwards typically have this).

You can also use conda to install *slow5tools* as `conda install slow5tools -c bioconda -c conda-forge`.

## Building

### Building a release

Users are recommended to build from the  [latest release](https://github.com/hasindu2008/slow5tools/releases) tar ball.

Quick example for Ubuntu :

```sh
sudo apt-get install libhdf5-dev zlib1g-dev   #install HDF5 and zlib development libraries
VERSION=v0.6.0
wget "https://github.com/hasindu2008/slow5tools/releases/download/$VERSION/slow5tools-$VERSION-release.tar.gz" && tar xvf slow5tools-$VERSION-release.tar.gz && cd slow5tools-$VERSION/
./configure
make
```
The commands to install hdf5 (and zlib) __development libraries__ on some popular distributions :
```sh
On Debian/Ubuntu : sudo apt-get install libhdf5-dev zlib1g-dev
On Fedora/CentOS : sudo dnf/yum install hdf5-devel zlib-devel
On Arch Linux: sudo pacman -S hdf5
On OS X : brew install hdf5 zlib
```


### Building from GitHub

**WARNING: Building from GitHub is meant for advanced users to test latest features. For production purposes, use the latest release version that is throughly tested.**

Building from the Github repository additionally requires `autoreconf` which can be installed on Ubuntu using `sudo apt-get install autoconf automake` (`brew install autoconf automake` on macOS).

To build from GitHub:

```
sudo apt-get install libhdf5-dev zlib1g-dev autoconf automake  #install HDF5 and zlib development libraries and autotools
git clone --recursive https://github.com/hasindu2008/slow5tools
cd slow5tools
autoreconf	# autoreconf --install for macos
./configure
make
```

### Other building options

- If you only want to manipulate S/BLOW5 files, you can disable FAST5/HDF5 for even easier compilation. Call `./configure --disable-hdf5 && make` or completely bypass the configure step and just call `make disable_hdf5=1`.

- You can optionally enable [*zstd* compression](https://facebook.github.io/zstd) support when building *slow5lib* by invoking `make zstd=1`. This requires __zstd 1.3 or higher development libraries__ installed on your system (*libzstd1-dev* package for *apt*, *libzstd-devel* for *yum/dnf* and *zstd* for *homebrew*). SLOW5 files compressed with *zstd* offer smaller file size and better performance compared to the default *zlib*. However, *zlib* runtime library is available by default on almost all distributions unlike *zstd* and thus files compressed with *zlib* will be more 'portable' (also see [notes](https://github.com/hasindu2008/slow5tools#notes)).

- *slow5tools* from version 0.3.0 onwards by default requires vector instructions (SSSE3 or higher for Intel/AMD and neon for ARM). If your processor is an ancient processor with no such vector instructions, invoke make as `make no_simd=1`.

- If you cannot install the hdf5 library system wide you can locally build HDF5 (takes ages) and build slow5tools against that:

    ```
    scripts/install-hdf5.sh         # download and compiles HDF5 in the current folder
    ./configure --enable-localhdf5
    make
    ```

    Similarly, to locally build *zstd* and link against that:

    ```
    scripts/install-zstd.sh        # download and compiles zstd in the current folder
    ./configure --enable-localzstd
    make			# don't run make zstd=1. libzstd.a is statically linked this time.
    ```

- On Mac M1 or in any system if `./configure` cannot find the hdf5 libraries installed through the package manager, you can specify the location as *LDFLAGS=-L/path/to/shared/lib/ CPPFLAGS=-I/path/to/headers/*. For example on Mac M1:
	```
	./configure LDFLAGS=-L/opt/homebrew/lib/ CPPFLAGS=-I/opt/homebrew/include/
	make
	```

- You can build a docker image as follows.
	```
	git clone https://github.com/hasindu2008/slow5tools && cd slow5tools
	docker build .
	docker run -v /path/to/local/data/data/:/data/ -it :image_id  ./slow5tools
	```

## Usage

Visit the [man page](https://hasindu2008.github.io/slow5tools/commands.html) for all the commands and options. See [here](https://hasindu2008.github.io/slow5tools/oneliners.html) for example bash one-liners with slow5tools. A guide on using BLOW5 for archiving and steps to verify if data integrity is preserved is [here](https://hasindu2008.github.io/slow5tools/archive.html). A script for performing real-time FAST5 to BLOW5 conversion during sequencing is provided [here](https://github.com/hasindu2008/slow5tools/tree/master/scripts/realtime-f2s).

### Examples

```sh
# convert a directory of fast5 files into BLOW5 files (default compression: zlib+svb-zd)
slow5tools f2s fast5_dir -d blow5_dir
# convert a single fast5 file into a SLOW5 ASCII
slow5tools f2s file.fast5 -o file.slow5
# convert a directory of fast5 files into BLOW5 files with zstd+svb-zd compression (similar to ONT's vbz compression)
slow5tools f2s fast5_dir -d blow5_dir -c zstd -s svb-zd

# concatenate all BLOW5 fils in a directory into a single BLOW5 file (works only if all the BLOW5 files have the same header, otherwise use merge)
slow5tools cat blow5_dir -o file.blow5

# merge all BLOW5 files in a directory into a single BLOW5 file (default compression: zlib+svb-zd)
slow5tools merge blow5_dir -o file.blow5
# merge all BLOW5 files in a directory into a single BLOW5 file with zstd+svb-zd compression (similar to ONT's vbz compression)
slow5tools merge blow5_dir -o file.blow5 -c zstd -s svb-zd

# to view a BLOW5 file in SLOW5 ASCII on standard out
slow5tools view file.blow5
# Convert a BLOW5 file into SLOW5 ASCII
slow5tools view file.blow5 -o file.slow5
# convert a SLOW5 file to BLOW5 (default compression)
slow5tools view file.slow5 -o file.blow5

# index a slow5/blow5 file
slow5tools index file.blow5

# extract records from a slow5/blow5 file corresponding to given read ids
slow5tools get file.blow5 readid1 readid2 -o output.slow5

# split a BLOW5 file into separate BLOW5 files based on the read groups
slow5tools split file.blow5 -d blow5_dir -g
# split a BLOW5 file (single read group) into separate BLOW5 files such that there are 4000 reads in one file
slow5tools split file.blow5 -d blow5_dir -r 4000

# convert a directory of blow5 files to fast5
slow5tools s2f blow5_dir -d fast5

```

Visit [here](https://hasindu2008.github.io/slow5tools/workflows.html) for example workflows.


### Troubleshooting/Questions

Visit the [frequently asked questions](https://hasindu2008.github.io/slow5tools/faq.html) or open an [issue](https://github.com/hasindu2008/slow5tools/issues).


### Upcoming features and optimisations

Following are some features and optimisations in our todo list which will be implemented based on the need. If anyone is interested please request [here](https://github.com/hasindu2008/slow5tools/issues). Contributions are welcome.

- pipelining input, processing and output in *merge, get, etc.* (improved runtime upto 2X, please find the implementation [here](https://github.com/hasindu2008/slow5tools/tree/interleave_merge))
- reading from stdin for *view*
- binary releases for ARM64 processors on Linux
- binary releases for MacOS
- any other features that are potentially useful to many

### Notes

*slow5lib* from version 0.3.0 onwards has built in [StreamVByte](https://github.com/lemire/streamvbyte) compression support to enable even smaller file sizes, which is applied to the raw signal by default when producing BLOW5 files.  *zlib* compression is then applied by default to each record. If *zstd* is used instead of *zlib* on top of *StreamVByte*, it is similar to ONT's latest [vbz](https://github.com/nanoporetech/vbz_compression) compression. BLOW5 files compressed with *zstd+StreamVByte* are still significantly smaller than vbz compressed FAST5 files.

## Acknowledgement
slow5tools uses [klib](https://github.com/attractivechaos/klib). Some code snippets have been taken from [Minimap2](https://github.com/lh3/minimap2) and [Samtools](http://samtools.sourceforge.net/).
