#!/bin/bash
#$ -cwd
#$ -V
#$ -N benchmark
#$ -S /bin/bash
#$ -b y
#$ -pe smp 40
#$ -l mem_requested=8G
#$ -l h_vmem=8G
#$ -l tmp_requested=200G
#$ -l h=zeta-*
#$ -o stdout.log
#$ -e stderr.log


set -x 

DATA_LOC=/directflow/KCCGGenometechTemp/projects/hasgam/slow5-minion/GZFN211103/

cp $DATA_LOC/disk_performance_evaluation/*.sh $DATA_LOC/reads.fq $TMPDIR/
cp ~/scratch/hg38noAlt/hg38noAlt.fa  $TMPDIR/

cp -r $DATA_LOC/fast5 $TMPDIR/fast5
cp $DATA_LOC/reads.fastt $TMPDIR/

cd $TMPDIR

./run_bench.sh

# cp -r fast5_iop_experiment_multifast5 $DATA_LOC/disk_performance_evaluation/
# cp -r prep $DATA_LOC/disk_performance_evaluation/fast5_iop_experiment_multifast5/

# cp -r fast5_iot_experiment_multifast5 $DATA_LOC/disk_performance_evaluation/
# cp -r prep $DATA_LOC/disk_performance_evaluation/fast5_iot_experiment_multifast5/

cp -r fastt_experiment $DATA_LOC/disk_performance_evaluation/
cp -r prep $DATA_LOC/disk_performance_evaluation/fastt_experiment/

cp *.txt $DATA_LOC/disk_performance_evaluation/