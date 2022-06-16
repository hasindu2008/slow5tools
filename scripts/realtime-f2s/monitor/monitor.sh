#!/bin/bash
#================================================================
# HEADER
#================================================================
#% SYNOPSIS
#+    ${SCRIPT_NAME} [options ...] [directories ...]
#%
#% DESCRIPTION
#%    Monitor script that outputs absolute file path
#%    once it is created in the monitored director(y/ies).
#%
#% OPTIONS
#%    -e, --existing                                Print pre-existing files in monitor director(y/ies)
#%    -f, --flag                                    Print flag of -1 if exited due to completion
#%    -h, --help                                    Print help message
#%    -i, --info                                    Print script information
#%    -n [num]                                      Exit after given number of files
#%    -d [tempfile]                                 Temporary file for monitor
#%    -t [seconds],
#%        default -t 10800                           Default timeout of 3 hours
#%
#% EXAMPLES
#%    exit after 10 new files
#%        ${SCRIPT_NAME} -n 10 [directory]
#%    exit after 30 mins of no new files from either directory
#%        ${SCRIPT_NAME} -t 1800 [dir_1] [dir_2]
#%
#================================================================
#- IMPLEMENTATION
#-    authors         Sasha JENNER (jenner.sasha@gmail.com)
#-    license         MIT
#-
#-    Copyright (c) 2020 Sasha Jenner
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

: ${1?$(usage)} # Require 1 arg else give usage message



    #== Default variables ==#

TEMP_FILE=temp # Temporary file

monitor_dirs=() # Declare empty list of directories to monitor
timeout=false # No timeout enabled by default
flag=false # No flag on exit enabled by default
existing=false # Existing files not outputed by default


## Handle flags
while getopts "ehift:n:d:" o; do
    case "${o}" in
        e)
            existing=true
            ;;
        h)
            usagefull
            exit 0
            ;;
        i)
            scriptinfo
            exit 0
            ;;
        f)
            flag=true
            ;;
        n)
            NO_FILES=${OPTARG}
            ;;
        t)
            timeout=true
            TIME_INACTIVE=${OPTARG}
            ;;
        d)
            TEMP_FILE="${OPTARG}"
            ;;
        *)
            echo "[monitor.sh] Incorrect or no timeout format specified"
            usagefull
            exit 1
            ;;
    esac
done
shift $((OPTIND-1))

# Iterating through the parameter directories
for dir in $@; do
    # Append the absolute path of each directory to array `monitor_dirs`
    monitor_dirs+=( $dir )
done

    #== Begin ==#

if $existing; then # If existing files option set
# Output the absolute path of all existing fast5 and fastq files
    find ${monitor_dirs[@]} | grep '\\.fast5'
fi

reset_timer() {
    echo 0 > $TEMP_FILE # Send flag to reset timer
}

exit_safely() { # Function to use on exit
    rm $TEMP_FILE # Remove the temporary file

    if $flag; then # If the flag option is enabled
        echo -1
    fi

    >&2 echo "[monitor.sh] exiting"

    # (todo : kill background while loop?)
}

touch $TEMP_FILE # Create the temporary file

trap exit_safely EXIT # Catch exit of script with function



i=0 # Initialise file counter
## Set up monitoring of all input directory indefinitely for a file being written or moved to them
(
    while read path action file; do

        if $timeout && echo $file | grep -q '\.fast5$'; then # If timeout option set and file is fast5
            reset_timer # Reset the timer
        fi
        echo "$path$file" # Output the absolute file path

        ((i++)) # Increment file counter
        if [ "$NO_FILES" = "$i" ]; then # Exit after specified number of files found
            echo -1 > $TEMP_FILE # Send flag to main process

            while : # Pause the script in while loop
            do
                sleep 1
            done

        fi

    done < <(inotifywait -r -m ${monitor_dirs[@]} -e close_write -e moved_to) # Pass output to while loop
) & # Push to the background



if $timeout; then # If timeout option set
    reset_timer # Reset the timer
fi

while $timeout; do

    # If 0 flag in temporary file
    if [ "$(cat $TEMP_FILE)" = "0" ]; then # Reset the timer
        SECONDS=0
        echo > $TEMP_FILE # Empty contents of temp file
    fi

    time_elapsed=$SECONDS
    # If there has been no files created in a specified period of time exit program
    # or -1 flag has been called by background process
    if (( $(echo "$time_elapsed > $TIME_INACTIVE" | bc -l) )) || [ "$(cat $TEMP_FILE)" = "-1" ]; then
        exit 0
    fi

    if [ "$REALF2S_AUTO" = "1" ]; then
        SAMPLE=${monitor_dirs[0]}
        NUM_RESTARTS=$(find ${SAMPLE}/*/* -maxdepth 0 -type d | wc -l)
        NUM_SUMS=$(find $SAMPLE -type f -name "final_summary*.txt" -mmin +60 | wc -l)
        if [ $NUM_SUMS -eq $NUM_RESTARTS ]; then
            exit 0
        fi
    fi
    sleep 1

done

while : ; do # While true
    # If -1 flag in temporary file
    if [ "$(cat $TEMP_FILE)" = "-1" ]; then
        exit 0
    fi
done
