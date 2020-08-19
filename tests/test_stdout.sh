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

USAGE="Usage: "$SLOW5TOOLS_PATH" [OPTION]... [COMMAND] [ARG]..."
VERSION=""$SLOW5TOOLS_PATH" 0.0"
HELP_SMALL="Try '"$SLOW5TOOLS_PATH" --help' for more information."
HELP_LARGE=""$USAGE"
Tools for using slow5 files.

COMMANDS:
    f2s - convert fast5 file(s) to slow5
    s2f - convert slow5 file(s) to fast5

ARGS:
    Try '"$SLOW5TOOLS_PATH" [COMMAND] --help' for more information.

OPTIONS:
    -d, --debug
        Output debug information.

    -h, --help
        Display this message and exit.

    -v, --verbose
        Explain what is being done.

    -V, --version
        Output version information and exit."

CMD_F2S="f2s"
F2S_PATH=""$SLOW5TOOLS_PATH" "$CMD_F2S""
F2S_USAGE="Usage: "$SLOW5TOOLS_PATH" "$CMD_F2S" [OPTION]... [FAST5_FILE/DIR]..."
F2S_HELP_SMALL="Try '"$SLOW5TOOLS_PATH" "$CMD_F2S" --help' for more information."
F2S_HELP_LARGE=""$F2S_USAGE"
Convert fast5 file(s) to slow5.

OPTIONS:
    -d, --max-depth=[NUM]
        Set the maximum depth to search directories for fast5 files.
        NUM must be a non-negative integer.
        Default: No maximum depth.

        E.g. NUM=1: Read the files within a specified directory but
        not those within subdirectories.

    -h, --help
        Display this message and exit.

    -o, --output=[SLOW5_FILE]
        Output slow5 contents to SLOW5_FILE.
        Default: Stdout."


# Good input

declare -a good_cmds
declare -a good_act
declare -a good_exp

good_cmds=( 
""$SLOW5TOOLS_PATH" -h"
""$SLOW5TOOLS_PATH" --help"
""$SLOW5TOOLS_PATH" -V"
""$SLOW5TOOLS_PATH" --version"
""$SLOW5TOOLS_PATH" f2s -h"
""$SLOW5TOOLS_PATH" f2s --help"
""$SLOW5TOOLS_PATH" -v f2s --help"
""$SLOW5TOOLS_PATH" -d 2>&1 1>&2"
""$SLOW5TOOLS_PATH" -d f2s -h 2>&1"
)

good_act=( 
"$("$SLOW5TOOLS_PATH" -h)"
"$("$SLOW5TOOLS_PATH" --help)"
"$("$SLOW5TOOLS_PATH" -V)"
"$("$SLOW5TOOLS_PATH" --version)"
"$("$SLOW5TOOLS_PATH" f2s -h)"
"$("$SLOW5TOOLS_PATH" f2s --help)"
"$("$SLOW5TOOLS_PATH" -v f2s --help)"
"$("$SLOW5TOOLS_PATH" -dV 2>&1)"
"$("$SLOW5TOOLS_PATH" -d f2s -h 2>&1)"
)

good_exp=(
"$HELP_LARGE"
"$HELP_LARGE"
"$VERSION"
"$VERSION"
"$F2S_HELP_LARGE"
"$F2S_HELP_LARGE"
"$F2S_HELP_LARGE"
"$SLOW5TOOLS_PATH [main.c:main:124]: \033[1;35margv=[\""$SLOW5TOOLS_PATH"\", \"-dV\"]\033[0m
$VERSION"
"$SLOW5TOOLS_PATH [main.c:main:124]: \033[1;35margv=[\"$SLOW5TOOLS_PATH\", \"-d\", \"f2s\", \"-h\"]\033[0m
$F2S_PATH [f2s.c:f2s_main:40]: \033[1;35margv=[\"$F2S_PATH\", \"-h\"]\033[0m
$F2S_HELP_LARGE"
)

# Bad input 

declare -a bad_cmds
declare -a bad_act
declare -a bad_exp

bad_cmds=(
""$SLOW5TOOLS_PATH" -v 2>&1 1>/dev/null" # Just stderr
""$SLOW5TOOLS_PATH" --verbose 2>&1 1>/dev/null"
""$SLOW5TOOLS_PATH" -v 2>/dev/null"
""$SLOW5TOOLS_PATH" --verbose 2>/dev/null"
""$SLOW5TOOLS_PATH" f2s 2>&1 1>/dev/null"
""$SLOW5TOOLS_PATH" f2s 2>/dev/null"
""$SLOW5TOOLS_PATH" -z 2>&1"
)

bad_act=( 
"$("$SLOW5TOOLS_PATH" -v 2>&1 1>&2)"
"$("$SLOW5TOOLS_PATH" --verbose 2>&1 1>&2)"
"$("$SLOW5TOOLS_PATH" -v 2>/dev/null)"
"$("$SLOW5TOOLS_PATH" --verbose 2>/dev/null)"
"$("$SLOW5TOOLS_PATH" f2s 2>&1 1>&2)"
"$("$SLOW5TOOLS_PATH" f2s 2>/dev/null)"
"$("$SLOW5TOOLS_PATH" -z 2>&1)"
)

bad_exp=(
"$SLOW5TOOLS_PATH: missing command
$HELP_SMALL"
"$SLOW5TOOLS_PATH: missing command
$HELP_SMALL"
""
""
"$F2S_PATH: missing fast5 files or directories
$F2S_HELP_SMALL"
""
"$SLOW5TOOLS_PATH: invalid option -- 'z'
$HELP_SMALL"
)


# Processing expected vs actual output

declare -i good_passed=0

if [ "${#good_act[@]}" != "${#good_exp[@]}" ] || [ "${#good_act[@]}" != "${#good_cmds[@]}" ]; then
    echo "good test: act("${#good_act[@]}") and exp("${#good_exp[@]}") and cmd("${#good_cmds[@]}") differ in length"
    exit 1
fi

for i in $(seq 0 $(( ${#good_act[@]}-1 )) ); do
    cmd="${good_cmds[$i]}"
    act="${good_act[$i]}"
    exp="${good_exp[$i]}"

    if diff <(echo -e "$act") <(echo -e "$exp") 2>&1 1>/dev/null; then
        good_passed+=1
    else
        echo -e "good test $(( i + 1 )): failed
--CMD----------------------------
$cmd
--ACT----------------------------
$act
--EXP----------------------------
$exp
---------------------------------
"
    fi
done

declare -i bad_passed=0

if [ "${#bad_act[@]}" != "${#bad_exp[@]}" ] || [ "${#bad_act[@]}" != "${#bad_cmds[@]}" ]; then
    echo "bad test: act("${#bad_act[@]}") and exp("${#bad_exp[@]}") and cmd("${#bad_cmds[@]}") differ in length"
    exit 1
fi

for i in $(seq 0 $(( ${#bad_act[@]}-1 )) ); do
    cmd="${bad_cmds[$i]}"
    act="${bad_act[$i]}"
    exp="${bad_exp[$i]}"
    
    if diff <(echo -e "$act") <(echo -e "$exp") 2>&1 1>/dev/null; then
        bad_passed+=1 
    else
        echo -e "bad test $(( i + 1 )): failed
--CMD----------------------------
$cmd
--ACT----------------------------
$act
--EXP----------------------------
$exp
---------------------------------
"
    fi
done


# Outcome

echo "good test: "$good_passed"/"${#good_exp[@]}" passed"
echo "bad test: "$bad_passed"/"${#bad_exp[@]}" passed"
