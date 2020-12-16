"""
Print out which fields of FAST5 files are static or variable for the same experiment
Given h5dump output of any number of FAST5 files

Usage: python3 $0 [h5dump_FAST5_output...]
"""

from sys import argv

attrs = {}
var = []
const = []

for fname in argv[1:]:
    f = open(fname)

    for line in f:
        line = line.split()

        if line[0] == "ATTRIBUTE" or line[0] == "DATASET":
            curr_attr = line[1][1:-1]

            # Create empty set for attribute if not already there
            if curr_attr not in attrs:
                attrs[curr_attr] = set()

        # Data follows a (0)
        elif line[0] == "(0):":
            # Store attribute's data
            data = " ".join(line[1:])
            attrs[curr_attr].add(data)

for prop in attrs:
    if len(attrs[prop]) == 1:
        const.append(prop)
    else:
        var.append(prop)

# Print properties which are constant and variable
print("Constant:")
for prop in const:
    print(prop)

print("\nVariable:")
for prop in var:
    print(prop)
