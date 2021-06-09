#!/bin/bash
#================================================================
# HEADER
#================================================================
#% SYNOPSIS
#+    ${SCRIPT_NAME} -f [format] -m [directory] [options ...]
#%
#% DESCRIPTION
#%    Runs realtime (or not) analysis of sequenced genomes
#%    given input directory and its expected format.
#%
#% OPTIONS
#%
#%    -h, --help                                    Print help message
#%    -i, --info                                    Print script information
#%    -l [filename]                                 Specify log filename for logs
#%                                                  default log.txt
#%
#%    -m [directory]                                Monitor a specific directory
#%    -n                                            Specify non-realtime analysis
#%    -r                                            Resumes from last processing position
#%    -d [directory]                               Specify a directory to place results
#%        default script location
#%    -s [file]                                     Custom script for processing files on the cluster
#%        default scripts/pipeline.sh         Default script which calls minimap, f5c & samtools
#%
#%    -t [time]                                   Timeout format in seconds (default 1 hour)
#%    -y, --yes                                     Say yes to 'Are you sure?' message in advance
#%
#% EXAMPLES
#%    play and resume
#%        ${SCRIPT_NAME} -m [directory]
#%        ${SCRIPT_NAME} -m [directory] -r

#%    non realtime
#%        ${SCRIPT_NAME}  [format] -m [directory] -n
#%
#================================================================
#- IMPLEMENTATION
#-    authors         Hasindu GAMAARACHCHI (hasindu@unsw.edu.au),
#-                    Sasha JENNER (jenner.sasha@gmail.com)
#-    license         MIT
#-
#-    Copyright (c) 2019 Hasindu Gamaarachchi, 2020 Sasha Jenner
#-
#-    Permission is hereby granted, free of charge, to any person obtaining a copy
#-    of this software and associated documentation files (the "Software"), to deal
#-    in the Software without restriction, including without limitation the rights
#-    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#-    copies of the Software, and to permit persons to whom the Software is
#-    furnished to do so, subject to the following conditions:
#-
#-    The above copyright notice and this permission notice shall be included in all
#-    copies or substantial portions of the Software.
#-
#-    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#-    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#-    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#-    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#-    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#-    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#-    SOFTWARE.
#-
#================================================================
# END_OF_HEADER
#================================================================

    #== Necessary variables ==#
SCRIPT_HEADSIZE=$(head -200 ${0} | grep -n "^# END_OF_HEADER" | cut -f1 -d:)
SCRIPT_NAME="$(basename ${0})"
SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )" # Scripts current path

    #== Usage functions ==#
usage() { printf "Usage: "; head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#+" | sed -e "s/^#+[ ]*//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }
usagefull() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#[%+-]" | sed -e "s/^#[%+-]//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }
scriptinfo() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#-" | sed -e "s/^#-//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }

    #== Default variables ==#

# Default script to be copied and run on the worker nodes
PIPELINE_SCRIPT="$SCRIPT_PATH/pipeline.sh"

RESULTS_DIR_PATH="$SCRIPT_PATH" # Default location for results
RESULTS_DIR_NAME="" # Default directory name for results
LOG="$SCRIPT_PATH"/log.txt # Default log filepath

# Set options off by default
resuming=false
realtime=true
custom_log_specified=false
say_yes=false

# Default timeout of 1 hour
TIME_INACTIVE=3600

# Assume necessary options not set
format_specified=true
monitor_dir_specified=false

MONITOR_PARENT_DIR=

## Handle flags
while getopts "ihnrym:l:t:d:" o; do
    case "${o}" in
        m)
            MONITOR_PARENT_DIR=${OPTARG}
            monitor_dir_specified=true
            ;;
		l)
            custom_log_specified=true
            LOG=${OPTARG}
			;;
        h)
            usagefull
            exit 0
            ;;

        i)
            scriptinfo
            exit 0
            ;;
        n)
            realtime=false
            ;;
        r)
            resuming=true
            ;;
        y)
            say_yes=true
            ;;

        d)
            RESULTS_DIR_PATH="${OPTARG}"
            RESULTS_DIR_NAME=$(basename "$RESULTS_DIR_PATH")
            ;;
        t)
            TIME_INACTIVE="${OPTARG}"
            ;;
        s)
            PIPELINE_SCRIPT=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))



# If either format or monitor option not set
if ! ($monitor_dir_specified); then
    if ! $monitor_dir_specified; then echo "No monitor directory specified!"; fi
	usage
	exit 1
fi



    #== Begin Run ==#

# Warn before cleaning logs
if ! $resuming && ! $say_yes; then # If not resuming
    while true; do
        read -p "This may overwrite stats from a previous run. Do you wish to continue? (y/n)" response

        case $response in
            [Yy]* )
                # make clean && make || exit 1 # Freshly compile necessary programs
                rm $LOG # Empty log file
                break
                ;;

            [Nn]* )
                exit 0
                ;;

            * )
                echo "Please answer yes or no."
                ;;
        esac
    done
fi

# Create folders to copy the results (SAM files, BAM files, logs and methylation calls)
test -d $MONITOR_PARENT_DIR/slow5         || mkdir $MONITOR_PARENT_DIR/slow5            || exit 1
test -d $MONITOR_PARENT_DIR/slow5_logs        || mkdir $MONITOR_PARENT_DIR/slow5_logs           || exit 1


if ! $realtime; then # If non-realtime option set
    find $MONITOR_PARENT_DIR/fast5_pass -name *.fast5 | /usr/bin/time -v "$PIPELINE_SCRIPT"  |&
    tee $LOG

else # Else assume realtime analysis is desired

    # Monitor the new file creation in fast5 folder and execute realtime f5-pipeline script
    # Close after timeout met
    if $resuming; then # If resuming option set
        echo "resuming"
        bash "$SCRIPT_PATH"/monitor/monitor.sh -t $TIME_INACTIVE -f -e $MONITOR_PARENT_DIR/fast5_pass/ $MONITOR_PARENT_DIR/fastq_pass/ 2>> $LOG |
        bash "$SCRIPT_PATH"/monitor/ensure.sh -r --results-dir=$RESULTS_DIR_PATH 2>> $LOG |
        /usr/bin/time -v "$PIPELINE_SCRIPT"  |&
        tee -a $LOG
    else
        echo "running"
        bash "$SCRIPT_PATH"/monitor/monitor.sh -t $TIME_INACTIVE -f $MONITOR_PARENT_DIR/fast5_pass/ $MONITOR_PARENT_DIR/fastq_pass/ 2>> $LOG |
        bash "$SCRIPT_PATH"/monitor/ensure.sh 2>> $LOG |
        /usr/bin/time -v "$PIPELINE_SCRIPT"  |&
        tee -a $LOG
    fi
fi

echo "[run.sh] handling logs" # testing
cp  $LOG "$MONITOR_PARENT_DIR"/slow5_logs/slow5_master_log.log # Copy entire data folder to local f5pmaster folder

echo "[run.sh] exiting" # testing
