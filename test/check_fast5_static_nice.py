"""
Print out which fields of FAST5 files are static or variable for the same experiment
In a nice hierarchical format
Given h5dump output of any number of FAST5 files

Usage: python3 $0 [h5dump_FAST5_output...] [-c]
"""

from sys import argv
import re

# Class for a hdf5 group
class Group:
    def __init__(self, name, prev_group=None):
        self.name = name
        self.prev_group = prev_group
        self.attrs = []

    def add_attr(self, attr):
        self.attrs.append(attr)

    def print_hier(self, prefix=''):
        for attr in self.attrs:
            if isinstance(attr, Group):
                print(f"{prefix}{attr.name}:")
                attr.print_hier(prefix + '    ')
            else:
                print(f"{prefix}{attr}")

    def print_hier_if_in(self, arr, show_val=False, prefix=''):
        is_dict = type(arr) is dict

        for attr in self.attrs:
            if isinstance(attr, Group):
                print(f"{prefix}{attr.name}:")
                attr.print_hier_if_in(arr, show_val, prefix + '    ')
            else:
                if attr in arr:
                    if show_val and is_dict:
                        print(f"{prefix}{attr}: {arr[attr]}")
                    else:
                        print(f"{prefix}{attr}")

    def __str__(self):
        return str(self.attrs)
    def __repr__(self):
        return str(self.attrs)

args = argv[1:]
if "-c" in args:
    args.remove("-c")
    show_const = True
else:
    show_const = False

attrs = {}
root_group = Group("/")

var = []
const = {}

bracket_pos = 0
group_pos = []

struct_made = False
curr_group = root_group
first_read = True

for fname in args:
    f = open(fname)

    for line in f:
        line = line.split()

        # Add group to structure
        if not struct_made and line[0] == "GROUP" and line[1] != '"/"':

            # Don't repeat the structure for the 'multi' type
            if re.match('"read_.+"', line[1]):
                if first_read:
                    first_read = False
                else:
                    struct_made = True

            if not struct_made:
                group_pos.append(bracket_pos)

                new_group = Group(line[1][1:-1])
                curr_group.add_attr(new_group)

                new_group.prev_group = curr_group
                curr_group = new_group

        if line[0] == "ATTRIBUTE" or line[0] == "DATASET":
            curr_attr = line[1][1:-1]

            # Create empty set for attribute if not already there
            if curr_attr not in attrs:
                attrs[curr_attr] = set()

            # Add attribute to structure
            if not struct_made:
                curr_group.add_attr(curr_attr)

        # Data follows a (0)
        elif line[0] == "(0):":
            # Store attribute's data
            data = " ".join(line[1:])
            attrs[curr_attr].add(data)

        # Closing marker
        if not struct_made and "{" in line:
            bracket_pos += 1

        if not struct_made and "}" in line:
            bracket_pos -= 1

            # Check if group is finished
            if len(group_pos) != 0 and bracket_pos == group_pos[-1]:
                group_pos = group_pos[:-1]
                curr_group = curr_group.prev_group

    if not struct_made:
        struct_made = True

# Decide what's constant and variable
for prop in attrs:
    if len(attrs[prop]) == 1:
        const[prop] = list(attrs[prop])[0]
    else:
        var.append(prop)

# Print properties which are constant and variable
print("CONSTANT")
root_group.print_hier_if_in(const, show_const)
print("\nVARIABLE")
root_group.print_hier_if_in(var, show_const)
