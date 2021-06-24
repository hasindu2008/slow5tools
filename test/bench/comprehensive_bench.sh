#!/bin/bash

# @author: Hasindu Gamaarachchi (hasindu@unsw.edu.au)

###############################################################################

#some changeable definitions
set -x

#reference fasta
MINIMAP=~/installs/minimap2-2.17/minimap2
SAMTOOLS=~/installs/samtools-1.9/samtools
F5C=~/hasindu2008.git/f5c/f5c
F5C_FAST5=~/hasindu2008.git/f5c-fastt/f5c
F5C_SLOW5=~/hasindu2008.git/f5c-slow5/f5c


#reference fasta
REF=~/store/genome/human_genome/hg38noAlt.fa
#reference index for minimap2
REFIDX=~/store/genome/human_genome/hg38noAlt.fa

THREAD_LIST="32 24 16 8 4"
THREAD_LIST2="32 16 8 4 2 1"

clean_file_system_cache() {
	clean_fscache
	#sync
	#echo 3 | tee /proc/sys/vm/drop_caches
}

FAST5DIR=fast5
FASTQ=reads.fastq
BAM=reads.bam
SAM=reads.sam
METH=reads.tsv
SLOW5_PREFIX=reads

print_versions (){
	echo "Number of threads : $num_threads"
	echo ""
	echo "minimap2 version"
	echo ""
	$MINIMAP --version
	echo "samtools version"
	$SAMTOOLS --version
	echo ""
	echo "f5c versions"
	$F5C --version
}


prep () {

	folder=prep
	num_threads=32

	test -d  $folder && rm -r $folder
	mkdir $folder
	print_versions > $folder/log.txt

	#index
	/usr/bin/time -v $F5C index -d $FAST5DIR/ $FASTQ --iop 32 -t 32 2> $folder/nanindex.log

	#minimap
	/usr/bin/time -v $MINIMAP -x map-ont -a -t$num_threads --secondary=no $REFIDX $FASTQ > $SAM 2> $folder/minimap.log


	#sorting
	/usr/bin/time -v $SAMTOOLS sort -@$num_threads $SAM > $BAM 2> $folder/sort.log
	/usr/bin/time -v $SAMTOOLS index $BAM 2> $folder/samindex.log


}



launch_f5c_fast5_iot_with_varied_threads () {

	rootdir=f5c_fast5_iot_with_varied_threads
	mkdir $rootdir

	for num in $THREAD_LIST
	do
		clean_fscache
		num_threads=$num
		folder=$rootdir/$num_threads

		test -d  $folder && rm -r $folder
		mkdir $folder
		print_versions > $folder/log.txt

		mkdir $folder/collectl
		collectl --all  -f $folder/collectl &
		collectlpid=$!

		core=$(echo "$num_threads-1" | bc)
		#methylation
		taskset -c 0-$core  /usr/bin/time -v $F5C_FAST5 call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $num_threads > $METH 2> $folder/methlog


		clean_fscache

		kill $collectlpid
	done

}




launch_f5c_slow5_with_varied_threads () {

	rootdir=$1
	SLOW5=$2
	#rootdir=f5c_slow5_with_varied_threads
	mkdir $rootdir

	for num in $THREAD_LIST
	do
		clean_fscache
		num_threads=$num
		folder=$rootdir/$num_threads

		test -d  $folder && rm -r $folder
		mkdir $folder
		print_versions > $folder/log.txt

		mkdir $folder/collectl
		collectl --all  -f $folder/collectl &
		collectlpid=$!

		core=$(echo "$num_threads-1" | bc)
		#methylation
		taskset -c 0-$core /usr/bin/time -v $F5C_SLOW5 call-methylation --fastt $SLOW5 -t $num_threads --iot $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M  > $METH 2> $folder/methlog

		clean_fscache

		kill $collectlpid
	done

}


#############################################################################

launch_f5c_fast5_iot_experiment (){

	folder=fast5_iot_experiment/
	test -d  $folder && rm -r $folder
	mkdir $folder
	print_versions > $folder/log.txt

	num_threads=32
	iot=32

	#mock
	/usr/bin/time -v $F5C_FAST5 call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $iot > $METH 2> $folder/methlog_mock.log

	for iot in $THREAD_LIST2
	do

		clean_fscache
		echo "doing for $iot iots"

		folder=fast5_iot_experiment/$iot
		mkdir $folder

		mkdir $folder/collectl
		collectl --all  -f $folder/collectl &
		collectlpid=$!

		core=$(echo "$num_threads-1" | bc)
		#methylation
		taskset -c 0-$core /usr/bin/time -v $F5C_FAST5 call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $iot > $METH 2> $folder/methlog_$iot.log

		clean_fscache
		kill $collectlpid

	done
}



launch_f5c_slow5_experiment (){

	folder_root=$1
	SLOW5=$2
	#folder=slow5_experiment/
	test -d  $folder_root && rm -r $folder_root
	mkdir $folder_root
	print_versions > $folder_root/log.txt

	num_threads=32
	iot=32

	#mock
	/usr/bin/time -v $F5C_SLOW5 call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $iot --fastt $SLOW5 > $METH 2> $folder_root/methlog_mock.log

	for iot in $THREAD_LIST2
	do

		clean_fscache
		echo "doing for $iot iots"

		folder=$folder_root/$iot
		mkdir $folder

		mkdir $folder/collectl
		collectl --all  -f $folder/collectl &
		collectlpid=$!

		core=$(echo "$num_threads-1" | bc)

		#methylation

		taskset -c 0-$core /usr/bin/time -v $F5C_SLOW5 call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $iot --fastt $SLOW5 > $METH 2> $folder/methlog_$iot.log


		clean_fscache
		kill $collectlpid

	done
}


########################################################

extract_f5c_varied_threads (){

	echo -e "threads\tBAM access\tFASTA access\tFAST5 access\tProcessing\texecutiontime\tCPUusage"
	for iot in 4 8 16 24 32
	do
		echo -en "$iot\t"
		folder=$1/$iot
		grep "bam load time" $folder/methlog | awk '{print $(NF-1)/3600}' |   tr '\n' '\t'
		grep "fasta load time" $folder/methlog | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "fast5 load time" $folder/methlog | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "Data processing time" $folder/methlog | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "Elapsed (wall clock) time (h:mm:ss or m:ss):" $folder/methlog | cut -d ' ' -f 8 |tr ':' \\t |  awk '{if(NF==1) print ($1/3600); else{ if(NF==2) print(($1*60)+$2)/3600; else print(($1*3600)+($2*60)+$3)/3600}}' | tr '\n' '\t'
		grep "Percent of CPU this job got" $folder/methlog | awk '{print $(NF)}' | tr '\n' '\t'
		echo ""
	done
}


extract_f5c_fast5_iot_experiment (){

	echo -e "I/O threads\tBAM access\tFASTA access\tFAST5 access\tProcessing"
	for iot in 1 2 4 8 16 32
	do
		echo -en "$iot\t"
		folder=fast5_iot_experiment/$iot
		grep "bam load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' |   tr '\n' '\t'
		grep "fasta load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "fast5 load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "Data processing time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		echo ""
	done
}

extract_f5c_slow5_experiment (){

	folder_r=$1

	echo -e "I/O threads\tBAM access\tFASTA access\tSLOW5 access\tProcessing"
	for iot in 1 2 4 8 16 32
	do
		echo -en "$iot\t"
		folder=$folder_r/$iot
		grep "bam load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' |   tr '\n' '\t'
		grep "fasta load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "fast5 load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "Data processing time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		echo ""
	done
}


prep

launch_f5c_fast5_iot_with_varied_threads
extract_f5c_varied_threads f5c_fast5_iot_with_varied_threads > f5c_fast5_iot_with_varied_threads.txt

launch_f5c_slow5_with_varied_threads f5c_slow5_with_varied_threads $SLOW5_PREFIX".slow5"
extract_f5c_varied_threads f5c_slow5_with_varied_threads > f5c_slow5_with_varied_threads.txt

launch_f5c_slow5_with_varied_threads f5c_blow5_with_varied_threads $SLOW5_PREFIX".blow5"
extract_f5c_varied_threads f5c_blow5_with_varied_threads > f5c_blow5_with_varied_threads.txt

launch_f5c_slow5_with_varied_threads f5c_blow5_gz_with_varied_threads $SLOW5_PREFIX"_gz.blow5"
extract_f5c_varied_threads f5c_blow5_gz_with_varied_threads > f5c_blow5_gz_with_varied_threads.txt

launch_f5c_fast5_iot_experiment
extract_f5c_fast5_iot_experiment > iot.txt

launch_f5c_slow5_experiment slow5_experiment $SLOW5_PREFIX".slow5"
extract_f5c_slow5_experiment slow5_experiment > slow5.txt

launch_f5c_slow5_experiment blow5_experiment $SLOW5_PREFIX".blow5"
extract_f5c_slow5_experiment blow5_experiment > blow5.txt

launch_f5c_slow5_experiment blow5_gz_experiment $SLOW5_PREFIX"_gz.blow5"
extract_f5c_slow5_experiment blow5_gz_experiment > blow5_gz.txt
