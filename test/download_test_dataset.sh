#!/bin/bash
# Download test datasets

download (){
	mkdir -p $download_dir
	tar_path=$download_dir/data.tgz
	wget -O $tar_path $link || rm -f $tar_path
	echo "Extracting. Please wait."
	tar -xf $tar_path -C $download_dir || rm -f $tar_path
	rm -f $tar_path
	echo "Extraction successful. Please check $download_dir."
}

download_dir=test/
# test -d $download_dir/NA12878_prom_subsubsample && rm -r $download_dir/NA12878_prom_subsubsample
# link="https://slow5.page.link/na12878_prom_subsub"
# download

test -d $download_dir/fast5_soup && rm -r $download_dir/fast5_soup
link="https://slow5.page.link/fast5-soup"
download

