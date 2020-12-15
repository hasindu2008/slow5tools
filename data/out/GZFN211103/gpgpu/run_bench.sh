#!/bin/bash

# @author: Hasindu Gamaarachchi (hasindu@unsw.edu.au)

###############################################################################

#some changeable definitions

#reference fasta
MINIMAP=minimap2
SAMTOOLS=samtools
F5C=f5c
F5C_FASTT=~/hasindu2008.git/f5c-fastt/f5c
F5C_IOP=~/hasindu2008.git/f5c-iop/f5c

#reference fasta
REF=../ref.fa
#reference index for minimap2
REFIDX=../ref.fa

clean_file_system_cache() {
	clean_fscache
	#sync
	#echo 3 | tee /proc/sys/vm/drop_caches
}

FASTQ=reads.fq
BAM=reads.bam
SAM=reads.sam
METH=reads.tsv
FASTT=reads.fastt

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
	/usr/bin/time -v $F5C index -d ../fast5/ $FASTQ --iop 32 -t 32 2> $folder/nanindex.log

	#minimap
	/usr/bin/time -v $MINIMAP -x map-ont -a -t$num_threads --secondary=no $REFIDX $FASTQ > $SAM 2> $folder/minimap.log


	#sorting
	/usr/bin/time -v $SAMTOOLS sort -@$num_threads $SAM > $BAM 2> $folder/sort.log
	/usr/bin/time -v $SAMTOOLS index $BAM 2> $folder/samindex.log

	#convert
	multi_to_single_fast5 -i ../fast5/ -s fast5/ --recursive -t32

	#fastt
	/usr/bin/time -v $F5C_FASTT fastt fast5/ > $FASTT 2> $folder/fastt.log
	/usr/bin/time -v $F5C_FASTT fastt -i $FASTT 2> $folder/fastt_index.log
	

}


launch_f5c_fast5_iot_experiment_multifast5 (){
	
	folder=fast5_iot_experiment_multifast5/
	test -d  $folder && rm -r $folder
	mkdir $folder	
	print_versions > $folder/log.txt
	
	num_threads=40
	iot=40
	
	#mock
	/usr/bin/time -v $F5C_FASTT call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $iot > $METH 2> $folder/methlog_mock.log
	
	for iot in 32 16 8 4 2 1 
	do
	
		clean_fscache
		echo "doing for $iot iots"
		
		folder=fast5_iot_experiment_multifast5/$iot
		mkdir $folder
		
		mkdir $folder/collectl
		collectl --all  -f $folder/collectl &
		collectlpid=$!

		#methylation
		/usr/bin/time -v $F5C_FASTT call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $iot > $METH 2> $folder/methlog_$iot.log
	
		clean_fscache
		kill $collectlpid
		
	done
}


extract_f5c_fast5_iot_experiment_multifast5 (){
	
	echo -e "I/O threads\tBAM access\tFASTA access\tFAST5 access\tProcessing"
	for iot in 1 2 4 8 16 32
	do
		echo -en "$iot\t"
		folder=fast5_iot_experiment_multifast5/$iot
		grep "bam load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' |   tr '\n' '\t'
		grep "fasta load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "fast5 load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "Data processing time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		echo ""
	done
}


launch_f5c_fast5_iop_experiment_multifast5 (){
	
	folder=fast5_iop_experiment_multifast5/
	test -d  $folder && rm -r $folder
	mkdir $folder	
	print_versions > $folder/log.txt
	
	num_threads=40
	iop=40
	
	#mock
	/usr/bin/time -v $F5C_IOP call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iop $iop > $METH 2> $folder/methlog_mock.log
	
	for iop in 32 16 8 4 2 1
	do
	
		clean_fscache
		echo "doing for $iop iops"
		
		folder=fast5_iop_experiment_multifast5/$iop
		mkdir $folder
		
		mkdir $folder/collectl
		collectl --all  -f $folder/collectl &
		collectlpid=$!

		if [ $iop -eq 1 ]
		then
			/usr/bin/time -v $F5C_FASTT call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iot $iop > $METH 2> $folder/methlog_$iop.log
			#this should be taken from iot mode instead (here I/O model is different)
		else
			#methylation
			/usr/bin/time -v $F5C_IOP call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM -K4096 -B100M --iop $iop > $METH 2> $folder/methlog_$iop.log
		fi
		
		clean_fscache
		kill $collectlpid
		
	done
}

extract_f5c_fast5_iop_experiment_multifast5 (){
	
	echo -e "I/O processes\tBAM access\tFASTA access\tFAST5 access\tProcessing"
	for iop in 1 2 4 8 16 32
	do
		echo -en "$iop\t"
		folder=fast5_iop_experiment_multifast5/$iop
		grep "bam load time" $folder/methlog_$iop.log | awk '{print $(NF-1)/3600}' |   tr '\n' '\t'
		grep "fasta load time" $folder/methlog_$iop.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "fast5 load time" $folder/methlog_$iop.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "Data processing time" $folder/methlog_$iop.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		echo ""
	done
}


compile_code_noasync (){


	test -d f5c-fastt-async && rm -rf f5c-fastt-async
	cp -r ~/hasindu2008.git/f5c-fastt f5c-fastt-async
	cd f5c-fastt-async
	
	#undefine ASYNC
	sed -i "s/#define ASYNC .*//" src/ftidx.h
	
	make clean
	make -j8

	cd ..
	
}

compile_code_async (){

	iot=$1

	test -d f5c-fastt-async && rm -rf f5c-fastt-async
	cp -r ~/hasindu2008.git/f5c-fastt f5c-fastt-async
	cd f5c-fastt-async
	
	#define async
	sed -i "s/\/\/#define ASYNC .*/#define ASYNC 1/" src/ftidx.h
 	sed -i "s/#define BGFS_HFILE .*//" src/ftidx.c
	sed -i "s/\/\/#define UN_BUFFERED .*/#define UN_BUFFERED 1/" src/ftidx.c

	#change number of threads
	sed -i "s/#define AIO_FD_N .*/#define AIO_FD_N $iot/" src/ftidx.c
	sed -i "s/ainit.aio_threads=64/ainit.aio_threads=$iot;/" src/f5c.c
	
	make clean
	make -j8

	cd ..
	
	
}

launch_f5c_fastt_experiment (){

	
	folder=fastt_experiment/
	test -d  $folder && rm -r $folder
	mkdir $folder	
	print_versions > $folder/log.txt
	
	F5C_FASTT_ASYNC=f5c-fastt-async/f5c
	num_threads=40
	iot=40
	
	compile_code_async $iot
	

	#mock
	/usr/bin/time -v $F5C_FASTT_ASYNC call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM --fastt $FASTT -K4096 -B100M > $METH 2> $folder/methlog
	
	for iot in 32 16 8 4 2 1
	do
		
		if [ $iot -eq 1 ]
		then
			compile_code_noasync
		else
			compile_code_async $iot
		fi
		
		clean_fscache
		echo "doing for $iot iots"
		
		folder=fastt_experiment/$iot
		mkdir $folder
		
		mkdir $folder/collectl
		collectl --all  -f $folder/collectl &
		collectlpid=$!


		/usr/bin/time -v $F5C_FASTT_ASYNC call-methylation -t $num_threads -r  $FASTQ -g $REF -b $BAM --fastt $FASTT -K4096 -B100M > $METH 2> $folder/methlog_$iot.log

	
		clean_fscache
		kill $collectlpid
		
	done

}


extract_f5c_fastt_experiment (){
	
	echo -e "I/O threads\tBAM access\tFASTA access\tFAST5 access\tProcessing"
	for iot in 1 2 4 8 16 32
	do
		echo -en "$iot\t"
		folder=fastt_experiment/$iot
		grep "bam load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' |   tr '\n' '\t'
		grep "fasta load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "fast5 load time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		grep "Data processing time" $folder/methlog_$iot.log | awk '{print $(NF-1)/3600}' | tr '\n' '\t'
		echo ""
	done
}



#prep
#launch_f5c_fast5_iop_experiment_multifast5
#extract_f5c_fast5_iop_experiment_multifast5 > iop.txt
#launch_f5c_fast5_iot_experiment_multifast5 
#extract_f5c_fast5_iot_experiment_multifast5 > iot.txt
launch_f5c_fastt_experiment
extract_f5c_fastt_experiment > fastt.txt


#prep

