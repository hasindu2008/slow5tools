# slow5tools

A toolset for converting to and from SLOW5 files.
<todo>

[![Build Status](https://travis-ci.com/hasindu2008/slow5.svg?token=pN7xnsxgLrRxbAn8WLVQ&branch=master)](https://travis-ci.com/hasindu2008/slow5)
[![SLOW5 C/C++ CI](https://github.com/hasindu2008/slow5/workflows/SLOW5%20C/C++%20CI/badge.svg)](https://github.com/hasindu2008/slow5/actions?query=workflow%3A%22SLOW5+C%2FC%2B%2B+CI%22)
[![SLOW5 C/C++ CI 2](https://github.com/hasindu2008/slow5/workflows/SLOW5%20C/C++%20CI%202/badge.svg)](https://github.com/hasindu2008/slow5/actions?query=workflow%3A%22SLOW5+C%2FC%2B%2B+CI+2%22)

## Quick start

If you are a Linux user and want to quickly try out download the compiled binaries from the [latest release](https://github.com/hasindu2008/slow5/releases). For example:
```sh
VERSION=v0.2-beta
wget "https://github.com/hasindu2008/f5c/releases/download/$VERSION/slow5tools-$VERSION-binaries.tar.gz" && tar xvf slow5tools-$VERSION-binaries.tar.gz && cd slow5tools-$VERSION/
./slow5tools_x86_64_linux
```
Binaries should work on most Linux distributions and the only dependency is `zlib` which is available by default on most distros.

## Building

Users are recommended to build from the  [latest release](https://github.com/hasindu2008/slow5/releases) tar ball. Quick example for Ubuntu :
```sh
sudo apt-get install libhdf5-dev zlib1g-dev   #install HDF5 and zlib development libraries
VERSION=v0.2-beta
wget "https://github.com/hasindu2008/slow5/releases/download/$VERSION/slow5tools-$VERSION-release.tar.gz" && tar xvf slow5tools-$VERSION-release.tar.gz && cd slow5tools-$VERSION/
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

Building from the Github repository additionally requires `autoreconf` which can be installed on Ubuntu using `sudo apt-get install autoconf automake`.

## Usage

```sh
slow5tools ...
```

Visit the [man page](https://hasindu2008.github.io/slow5/docs/commands) for all the commands and options.

### Example


```sh
slow5tools ...
```

## Acknowledgement
Some code snippets have been taken from [Minimap2](https://github.com/lh3/minimap2) and [Samtools](http://samtools.sourceforge.net/).
