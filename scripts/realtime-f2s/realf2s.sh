#!/bin/bash
#================================================================
# HEADER
#================================================================
#% SYNOPSIS
#+    ${SCRIPT_NAME} -m [directory] [options ...]
#%
#% DESCRIPTION
#%    Runs realtime FAST5 to SLOW5 conversion of sequenced genomes
#%    given input directory.
#%
#% OPTIONS
#%
#%    -h, --help                                    Print help message
#%    -i, --info                                    Print script information
#%    -m [directory]                                Monitor a specific directory
#%    -r                                            Resumes from last processing position
#%    -t [time]                                     Timeout format in seconds (default 3600 s)
#%
#% ADVANCED OPTIONS
#%
#%    -n                                            Specify non-realtime analysis
#%    -d [filename]                                 Specify location of temporary file (default: monitor_dir/realtime_f2s_attempted_list.log)
#%    -l [filename]                                 Specify log filename for logs (default: monitor_dir/realtime_f2s.log)
#%    -f [file]                                     Specify location of files that failed to convert (default: monitor_dir/realtime_f2s_failed_list.log)
#%    -s [file]                                     Specify script for processing files (default: script_location/pipeline.sh)
#%    -y, --yes                                     Say yes to 'Are you sure?' message in advance
#%
#% EXAMPLES
#%    convert
#%        ${SCRIPT_NAME} -m [directory]
#%    resume convert
#%        ${SCRIPT_NAME} -m [directory] -r
#%    non realtime
#%        ${SCRIPT_NAME} -m [directory] -n
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
usagefull() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#[%+]" | sed -e "s/^#[%+-]//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }
scriptinfo() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#-" | sed -e "s/^#-//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }

    #== Default variables ==#

# Default script to be copied and run on the worker nodes
PIPELINE_SCRIPT="$SCRIPT_PATH/pipeline.sh"

# Set options by default
resuming=false
realtime=true
say_yes=false

# Default timeout of 1 hour
TIME_INACTIVE=3600

# Assume necessary options not set
monitor_dir_specified=false
MONITOR_PARENT_DIR=

## Handle flags
while getopts "ihnrym:l:t:d:f:" o; do
    case "${o}" in
        m)
            MONITOR_PARENT_DIR=${OPTARG}
            monitor_dir_specified=true
            ;;
		l)
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
            TMP_FILE_PATH="${OPTARG}"
            ;;
        t)
            TIME_INACTIVE="${OPTARG}"
            ;;
        s)
            PIPELINE_SCRIPT=${OPTARG}
            ;;
        f)
            FAILED_LIST=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))



# If either format or monitor option not set
if ! ($monitor_dir_specified); then
    if ! $monitor_dir_specified; then echo "[realf2s.sh] No monitor directory specified!"; fi
	usage
	exit 1
fi

# set the temporary file and log file
[ -z ${TMP_FILE_PATH} ] && TMP_FILE_PATH=${MONITOR_PARENT_DIR}/realtime_f2s_attempted_list.log
echo "[realf2s.sh] Temporary file location that saves the state $TMP_FILE_PATH"
[ -z ${FAILED_LIST} ] && FAILED_LIST=${MONITOR_PARENT_DIR}/realtime_f2s_failed_list.log
echo "[realf2s.sh] Any fast5 files that failed conversions will be written to $FAILED_LIST"
[ -z ${LOG} ] && LOG=${MONITOR_PARENT_DIR}/realtime_f2s.log
echo "[realf2s.sh] Master log file location ${LOG}"
MONITOR_TRACE=${MONITOR_PARENT_DIR}/realtime_f2s_monitor_trace.log              #trace of the monitor for debugging
echo "[realf2s.sh] Monitor trace log ${MONITOR_TRACE}"
START_END_TRACE=${MONITOR_PARENT_DIR}/realtime_f2s_start_end_trace.log          #trace for debugging
echo "[realf2s.sh] Start end trace log ${START_END_TRACE}"
MONITOR_TEMP=${MONITOR_PARENT_DIR}/realtime_f2s_monitor_temp                    #used internally to communicate with the monitor
echo "[realf2s.sh] Idle time with no fast5 files to end the program ${TIME_INACTIVE} seconds"
test -d ${MONITOR_PARENT_DIR} || { echo "[realf2s.sh] Monitor directory does not exist!"; exit 1; }
[ -z ${SLOW5TOOLS} ] && export SLOW5TOOLS=slow5tools



${SLOW5TOOLS} --version &> /dev/null || { echo "[realf2s.sh] slow5tools not found! Either put slow5tools under path or set SLOW5TOOLS variable, e.g.,export SLOW5TOOLS=/path/to/slow5tools"; exit 1;}
which inotifywait &> /dev/null || { echo "[realf2s.sh] inotifywait not found! On ubuntu: sudo apt install inotify-tools"; exit 1; }

#== Begin Run ==#

# Warn before cleaning logs
if ! $resuming && ! $say_yes; then # If not resuming

    if [ -e ${LOG} ]
        then
            while true; do
                read -p "[realf2s.sh] A previous log file exist at ${LOG}! Are you sure you want to remove them and start over (y/n)" response
                case $response in
                    [Yy]* )
                        test -e $LOG && rm $LOG # Empty log file
                        test -e $MONITOR_TRACE && rm $MONITOR_TRACE # Empty log file
                        test -e $START_END_TRACE && rm $START_END_TRACE # Empty log file
                        test -e $FAILED_LIST && rm $FAILED_LIST # Empty log file
                        break
                        ;;

                    [Nn]* )
                        exit 0
                        ;;

                    * )
                        echo "[realf2s.sh] Please answer yes or no."
                        ;;
                esac
        done
    fi

fi

# Create folders to copy the results (slow5 files logs)
# test -d $MONITOR_PARENT_DIR/slow5         || mkdir $MONITOR_PARENT_DIR/slow5            || exit 1
# echo "[realf2s.sh] SLOW5 files will be written to $MONITOR_PARENT_DIR/slow5"
# test -d $MONITOR_PARENT_DIR/slow5_logs    || mkdir $MONITOR_PARENT_DIR/slow5_logs       || exit 1
# echo "[realf2s.sh] SLOW5 f2s individual logs will be written to $MONITOR_PARENT_DIR/slow5_logs"

if ! $realtime; then # If non-realtime option set
    echo "[realf2s.sh] Non realtime conversion of all files in $MONITOR_PARENT_DIR"
    test -e $TMP_FILE_PATH && rm $TMP_FILE_PATH
    find $MONITOR_PARENT_DIR/ -name *.fast5 | "$PIPELINE_SCRIPT"  |&
    tee $LOG

else # Else assume realtime analysis is desired

    # Monitor the new file creation in fast5 folder and execute realtime f5-pipeline script
    # Close after timeout met
    if $resuming; then # If resuming option set
        echo "[realf2s.sh] resuming"
        "$SCRIPT_PATH"/monitor/monitor.sh -t $TIME_INACTIVE -f -d ${MONITOR_TEMP} $MONITOR_PARENT_DIR/  |
        "$SCRIPT_PATH"/monitor/ensure.sh -r -d $TMP_FILE_PATH -l ${MONITOR_TRACE}  |
        "$PIPELINE_SCRIPT" -d $TMP_FILE_PATH -l $START_END_TRACE |&
        tee -a $LOG
    else
        echo "[realf2s.sh] running"
        test -e $TMP_FILE_PATH && rm $TMP_FILE_PATH
        "$SCRIPT_PATH"/monitor/monitor.sh -t $TIME_INACTIVE -f -d ${MONITOR_TEMP} $MONITOR_PARENT_DIR/  |
        "$SCRIPT_PATH"/monitor/ensure.sh -d $TMP_FILE_PATH -l ${MONITOR_TRACE}  |
        "$PIPELINE_SCRIPT" -d $TMP_FILE_PATH -l $START_END_TRACE -f $FAILED_LIST |&
        tee -a $LOG
    fi
    echo "[realf2s.sh] converting left overs"
    find $MONITOR_PARENT_DIR/ -name *.fast5   |
    "$SCRIPT_PATH"/monitor/ensure.sh -r -d $TMP_FILE_PATH -l ${MONITOR_TRACE}  |
    "$PIPELINE_SCRIPT" -d $TMP_FILE_PATH -l $START_END_TRACE -f $FAILED_LIST |&
    tee -a $LOG

fi

echo "[realf2s.sh] No new fast5 files found in last ${TIME_INACTIVE} seconds."
test -e $FAILED_LIST && echo "[realf2s.sh] $(wc -l $FAILED_LIST) fast5 files failed to convert. See $FAILED_LIST for the list"
NUMFAST5=$(find $MONITOR_PARENT_DIR/ -name '*.fast5' | wc -l)
NUMBLOW5=$(find $MONITOR_PARENT_DIR/ -name '*.blow5' | wc -l)
if [ ${NUMFAST5} -ne ${NUMBLOW5} ] ; then
    echo "[realf2s.sh] In $MONITOR_PARENT_DIR, $NUMFAST5 fast5 files, but only $NUMBLOW5 blow5 files. Check the logs for any failures."
else
    echo "[realf2s.sh] In $MONITOR_PARENT_DIR, $NUMFAST5 fast5 files, $NUMBLOW5 blow5 files."
fi
FAST5_SIZE=$(find $MONITOR_PARENT_DIR/ -name '*.fast5' -printf "%s\t%p\n" | awk 'BEGIN{sum=0}{sum=sum+$1}END{print sum/(1024*1024*1024)}')
BLOW5_SIZE=$(find $MONITOR_PARENT_DIR/ -name '*.blow5' -printf "%s\t%p\n" | awk 'BEGIN{sum=0}{sum=sum+$1}END{print sum/(1024*1024*1024)}')
SAVINGS=$(echo $FAST5_SIZE - $BLOW5_SIZE | bc)
SAVINGS_PERCENT=$(echo "scale=2; $SAVINGS/$FAST5_SIZE*100" | bc)
echo "FAST5 size: $FAST5_SIZE GB"
echo "BLOW5 size: $BLOW5_SIZE GB"
echo "Savings: $SAVINGS GB ($SAVINGS_PERCENT%)"

echo "[realf2s.sh] exiting" # testing
