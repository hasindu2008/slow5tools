#!/bin/bash

SLOW5TOOLS=../../slow5tools

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

    ${SLOW5TOOLS} f2s -p1 $FILE -o $SLOW5_FILEPATH 2> $LOG_FILEPATH

done
