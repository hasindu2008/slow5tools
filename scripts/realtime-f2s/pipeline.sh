#!/bin/bash

SLOW5TOOLS=slow5tools
TMP_FILE="processed_list.log"

# terminate script
die() {
	echo "$1" >&2
	echo
	exit 1
}

## Handle flags
while getopts "d:" o; do
    case "${o}" in
        d)
            TMP_FILE=${OPTARG}
            ;;
        *)
            echo "Incorrect args"
            usagefull
            exit 1
            ;;
    esac
done
shift $((OPTIND-1))

slow5tools --version &> /dev/null || die "slow5tools not found in path. Exiting."

test -e start_end_trace.log && rm start_end_trace.log

while read FILE
do
    F5_FILEPATH=$FILE # first argument
    F5_DIR=${F5_FILEPATH%/*} # strip filename from .fast5 filepath
    PARENT_DIR=${F5_DIR%/*} # get folder one heirarchy higher
    # name of the .fast5 file (strip the path and get only the name with extension)
    F5_FILENAME=$(basename $F5_FILEPATH)
    # name of the .fast5 file without the extension
    F5_PREFIX=${F5_FILENAME%.*}

    SLOW5_FILEPATH=$PARENT_DIR/slow5/$F5_PREFIX.blow5
    LOG_FILEPATH=$PARENT_DIR/slow5_logs/$F5_PREFIX.log

    echo "Converting $FILE to $SLOW5_FILEPATH"
    START_TIME=$(date)
    ${SLOW5TOOLS} f2s -p1 $FILE -o $SLOW5_FILEPATH 2> $LOG_FILEPATH
    END_TIME=$(date)

    echo "$F5_FILEPATH" >> $TMP_FILE
    echo -e $F5_FILEPATH"\t"$SLOW5_FILEPATH"\t"$START_TIME"\t"$END_TIME >> start_end_trace.log

done
