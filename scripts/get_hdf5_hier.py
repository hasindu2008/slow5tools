"""
Get the hierarchy of a hdf5 file given h5dump file
Built for fast5 files

Usage: python3 $0 [h5dump_file]
       python3 $0 <(h5dump [fast5_file])
"""

from sys import argv

f = open(argv[1])
tab = ""
bracket_pos = 0
group_pos = []

for line in f:
    line = line.split()

    if line[0] == "GROUP" and line[1] != '"/"':
        print(f"{tab}{line[1][1:-1]}:")
        tab += "\t"
        group_pos.append(bracket_pos)

    elif line[0] == "ATTRIBUTE" or line[0] == "DATASET":
        print(f"{tab}{line[1][1:-1]}: ", end="")

    # Data follows a (0)
    elif line[0] == "(0):":
        print(f"{' '.join(line[1:])}")

    elif line[0] == "HARDLINK":
        print(f"{tab}--> {line[1][1:-1]}")

    # Closing marker
    if "{" in line:
        bracket_pos += 1

    if "}" in line:
        bracket_pos -= 1

        # Check if group is finished
        if len(group_pos) != 0 and bracket_pos == group_pos[-1]:
            group_pos = group_pos[:-1]
            tab = tab[:-1]
