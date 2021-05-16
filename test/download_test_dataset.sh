#!/bin/bash
# Download a test dataset
Usage="download_dataset.sh [path to download directory] [download link (optional)]"

if [[ "$#" -lt 1 ]]; then
	echo "Usage: $Usage"
	exit
fi

download_dir=$1
link="https://github.com/samtools/htslib/releases/download/1.9/htslib-1.9.tar.bz2"

if [[ "$#" -eq 2 ]]; then
	link=$2
fi

mkdir -p $download_dir
tar_path=$download_dir/data.tgz
wget -O $tar_path $link || rm -rf $tar_path
echo "Extracting. Please wait."
tar -xf $tar_path -C $download_dir || rm -rf $tar_path
rm -f $tar_path
echo "Extraction successful. Please check $download_dir."
