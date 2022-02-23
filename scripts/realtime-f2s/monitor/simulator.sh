#!/bin/bash
#================================================================
# HEADER
#================================================================
#% SYNOPSIS
#+    ${SCRIPT_NAME} [options ...] [in_dir] [out_dir]
#%
#% DESCRIPTION
#%    Simulator of sequenced fast5 files
#%    into specified directory.
#%
#% OPTIONS
#%
#%    -h	                                    Print help message
#%    -i	                                    Print script information
#%	  -n [num]									Copy a given number of batches
#%    -r										Realistic simulation
#%	  -s [file]									Sequencing summary file
#%
#% EXAMPLES
#%    normal simulation with 30s between batches
#%        ${SCRIPT_NAME} -t 30s [in_dir] [out_dir]
#%    realtime simulation
#%        ${SCRIPT_NAME} -r [in_dir] [out_dir]
#%
#================================================================
#- IMPLEMENTATION
#-    authors         Sasha JENNER (jenner.sasha@gmail.com)
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
SCRIPT_HEADSIZE="$(head -200 "${0}" | grep -n "^# END_OF_HEADER" | cut -f1 -d:)"
SCRIPT_NAME="$(basename "${0}")"
SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )" # Scripts current path

    #== Usage functions ==#
usage() { printf "Usage: "; head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#+" | sed -e "s/^#+[ ]*//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }
usagefull() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#[%+]" | sed -e "s/^#[%+-]//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }
scriptinfo() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#-" | sed -e "s/^#-//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }


# terminate script
die() {
	echo "$1" >&2
	echo
	exit 1
}

    #== Default variables ==#

NO_BATCHES=-1 # Default value of -1 if parameter unset
TIME=10s # 10s between copying by default
REAL_SIM=false # No real simulation by default
SEQ_SUM=""

## Handle flags
while getopts "ihrn:t:s:" o; do
    case "${o}" in
        h)
            usagefull
            exit 0
            ;;

        i)
            scriptinfo
            exit 0
            ;;
        n)
            NO_BATCHES="${OPTARG}"
            ;;
        r)
            REAL_SIM=true
            ;;
        t)
            TIME="${OPTARG}"
            ;;
		s)
			SEQ_SUM="${OPTARG}"
			;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

: ${2?$(usage)} # Require 2 args else give usage message


INPUT_DIR=$1
OUTPUT_DIR=$2
F5_DIR=$INPUT_DIR

test -d $F5_DIR || die "Input directory $F5_DIR does not exist"

    #== Begin ==#


# Colour codes for printing
RED="\033[0;31m"
GREEN="\033[34m"
NORMAL="\033[0;39m"

i=0 # Initialise a file counter

## Function for copying the corresponding fast5 and fastq files to the output directory

copy_f5_files () {

	F5_FILE=$1
	filename_pathless=$(basename $F5_FILE) # Extract the filename without the path
	filename="${filename_pathless%.*}" # Extract the filename without the extension nor the path

	echo -e $GREEN"fast5: copying $i ($F5_FILE to $OUTPUT_DIR/fast5/$filename_pathless) "$NORMAL # Set font colour to green and then back to normal

	# If fast5 file copying fails
	mkdir -p $OUTPUT_DIR/fast5 && cp $F5_FILE $OUTPUT_DIR/fast5/$filename_pathless
	if [ $? -ne 0 ]; then
		echo -e $RED"- fast5: failed copy $i"$NORMAL
	else # Else copying worked
		echo -e $GREEN"+ fast5: finished copy $i"$NORMAL
	fi

	if [ $i -eq $NO_BATCHES ]; then # If the number of batches copied equals constant
		exit 0 # Exit program
	fi
}



if $REAL_SIM; then # If the realistic simulation option is set

	# Declare an associative array to hold the file with corresponding completion time
	declare -A file_time_map

	# Extract corresponding sequencing summary filename
	seq_summary_file=$SEQ_SUM

	test -z $seq_summary_file && die "Sequencing summary file required (-s option) for real simulation"
	test -e $seq_summary_file || die "File $seq_summary_file does not exist"

	# Iterate through files with .fast5 extension in the fast5 directory
	for filename_path in $(find $F5_DIR -name *.fast5); do

		filename_pathless=$(basename $filename_path) # Extract the filename without the path
		filename="${filename_pathless%.*}" # Extract the filename without the extension nor the path

		test_cmd=$(cat $seq_summary_file 2>/dev/null |
				grep "$filename_pathless\|filename" |
				wc -l)

		# If sequencing summary file is empty or filename not found
		if [ "$test_cmd" = "0" ] || [ "$test_cmd" = "1" ]; then
			echo "Could not find $filename_pathless in $seq_summary_file"
			continue # Continue to next file
		fi

		# Cat the sequencing summary txt file to awk
		# which prints the highest start_time + duration (i.e. the completion time of that file)
		end_time=$(cat $seq_summary_file 2>/dev/null |
		grep "$filename_pathless\|filename" |
		awk '
		BEGIN {
			FS="\t" # set the file separator to tabs
			# Define variables
			final_time=0
		}

		NR==1 {
			for (i = 1; i <= NF; i ++) {
				if ($i == "start_time") {
					start_time_field = i

				} else if ($i == "duration") {
					duration_field = i
				}
			}
		}

		NR > 1 {
			if ($start_time_field + $duration_field > final_time) { # If the start-time + duration is greater than the current final time
				final_time = $start_time_field + $duration_field # Update the final time
			}
		}

		END { printf final_time }') # End by printing the final time

		file_time_map["$end_time"]=$filename_path # Set a key, value combination of the end time and file
		echo "$filename_path deduced to be available at $end_time"
	done

	echo "Starting simulation"
	SECONDS=0 # Restart the timer
	ordered_list=$(for time in "${!file_time_map[@]}"; do # For each time in the keys of the associative array
			echo $time # Output the time
		done |
		sort -g # Sort the output in ascending generic numerical order (including floating point numbers))
		)

	for ordered_time in $ordered_list
	do
		while (( $(echo "$SECONDS < $ordered_time" | bc -l) )) # While the file's has not been 'completed'
		do
			: # Do nothing
		done

		filename_path=${file_time_map[$ordered_time]} # Extract file from map

		echo "file completed: ${ordered_time}s | file: $filename_path"

		((i++)) # Increment the counter
		copy_f5_files $filename_path # Copy fast5 files into output directory
	done

else ## Else iterate through the files normally
	find $F5_DIR -name *.fast5 | while read filename_path ; do
		((i++)) # Increment the counter
		copy_f5_files $filename_path # Copy fast5 and fastq files into output directory
		sleep $TIME # Pause for a given time
	done
fi
