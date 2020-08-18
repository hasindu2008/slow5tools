#!/bin/bash
# Usage $0
# Tests standard input/output of slow5tools.
# Note: #
# - Don't move this file from "slow5/tests/". 
#   Requires "slow5tools" executable in parent directory.



# Relative path to "slow5/tests/"
REL_PATH="$(dirname $0)/" 

# Path to slow5tools 
SLOW5TOOLS_PATH="$REL_PATH/../slow5tools"
# Ensure slow5tools exists
if [ ! -f $DATA_DIR ]; then
    echo "ERROR: Missing slow5tools \"$SLOW5TOOLS_PATH\""

    echo "Exiting"
    exit 1
fi

CMD_FAST5_TO_SLOW5="f2s"
USAGE_MSG="Usage: "$SLOW5TOOLS_PATH" [OPTION]... [COMMAND] [ARG]..."
VERSION_MSG=""$SLOW5TOOLS_PATH" 0.0"
HELP_SMALL_MSG="Try '"$SLOW5TOOLS_PATH" --help' for more information."
HELP_LARGE_MSG=""$USAGE_MSG"
Tools for using slow5 files.

COMMANDS:
    f2s - convert fast5 file(s) to slow5
    s2f - convert slow5 file(s) to fast5

ARGS:
    Try '"$SLOW5TOOLS_PATH" [COMMAND] --help' for more information.

OPTIONS:
    -h, --help
        Print this message and exit.

    -v, --verbose
        Output more information.

    -V, --version
        Output the current version and exit."


# Good input

declare -a good_act
declare -a good_exp

good_act=( 
"$("$SLOW5TOOLS_PATH" -h)"
"$("$SLOW5TOOLS_PATH" --help)"
"$("$SLOW5TOOLS_PATH" -V)"
"$("$SLOW5TOOLS_PATH" --version)"
)

good_exp=(
"$HELP_LARGE_MSG"
"$HELP_LARGE_MSG"
"$VERSION_MSG"
"$VERSION_MSG"
)

# Bad input 

declare -a bad_act
declare -a bad_exp

bad_act=( 
"$("$SLOW5TOOLS_PATH" -v 2>&1 1>&2)" # Just stderr
"$("$SLOW5TOOLS_PATH" --verbose 2>&1 1>&2)" # Just stderr
)

bad_exp=(
"$SLOW5TOOLS_PATH: missing command
$HELP_SMALL_MSG"
"$SLOW5TOOLS_PATH: missing command
$HELP_SMALL_MSG"
)


# Processing expected vs actual output

declare -i good_passed=0

if [ "${#good_act[@]}" != "${#good_exp[@]}" ]; then
    echo "good test: act and exp differ in length"
    exit 1
fi

for i in $(seq 0 $(( ${#good_act[@]}-1 )) ); do
    act="${good_act[$i]}"
    exp="${good_exp[$i]}"

    if [ "$act" = "$exp" ]; then
        good_passed+=1
    else
        echo "good test $(( i + 1 )): failed
    --ACT--
    $act
    --EXP--
    $exp"
    fi
done

declare -i bad_passed=0

if [ "${#bad_act[@]}" != "${#bad_exp[@]}" ]; then
    echo "bad test: act and exp differ in length"
    exit 1
fi

for i in $(seq 0 $(( ${#bad_act[@]}-1 )) ); do
    act="${bad_act[$i]}"
    exp="${bad_exp[$i]}"
    
    if [ "$act" = "$exp" ]; then 
        bad_passed+=1 
    else
        echo "bad test $(( i + 1 )): failed
    --ACT--
    $act
    --EXP--
    $exp"
    fi
done


# Outcome

echo "good test: "$good_passed"/"${#good_exp[@]}" passed"
echo "bad test: "$bad_passed"/"${#bad_exp[@]}" passed"
