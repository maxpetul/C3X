
# Reads in Civ3Conquests.h from the parent directory, converts it for C++, and writes it back out (into this directory) as Civ3Conquests.hpp

from pathlib import Path
import csv

def process_line(L):
    L = L.replace("this", "_this");

    s = L.split("__fastcall")
    if len(s) == 1:
        return s[0]
    elif len(s) == 2:
        return s[0] + "__thiscall" + s[1].replace("__,", "")
    else:
        raise Exception("Invalid line: " + L)

# Returns C/C++ source lines creating #defines for all objects defined in civ_prog_objects.csv. The addresses are taken from an array, and
# additionally one array is created (as a global constant) for each binary version.
def make_addr_table_lines():
    tr = []

    with open(Path.cwd().parent.joinpath("civ_prog_objects.csv"), mode="r") as cpo_file:
        reader = csv.reader(cpo_file)
        objects = [x for x in list(reader) if len(x) > 0 and (x[0].strip() == "define" or x[0].strip() == "inlead")]

        for n in range(len(objects)):
            ctype = ",".join(objects[n][5:]).strip(" \"\t") # types contain commas and so get split into separate columns by the CSV reader
            name  = objects[n][4].strip(" \"\t")

            # Undo the __fastcall standin for __thiscall
            if "__fastcall" in ctype:
                ctype = ctype.replace("this", "_this").replace("__fastcall", "__thiscall").replace("int edx,", "")

            tr.append(f"#define {name} (({ctype})bin_addrs[{n}])")

        def make_addr_table(index, bin_name):
            addrs = []
            addrs.append(f"int const {bin_name}_addrs[{len(objects)}] = " + "{")
            for ob in objects:
                addrs.append(f"\t{ob[index].strip()},")
            addrs.append("};")
            return addrs

        tr.extend(make_addr_table(1, "gog"))
        tr.extend(make_addr_table(2, "steam"))
        tr.extend(make_addr_table(3, "pcg"))

    return tr

with open("Civ3Conquests.hpp", mode="w") as hpp: # Output file
    # Copy over contents of header file, processing each line
    with open(Path.cwd().parent.joinpath("Civ3Conquests.h"), mode="r") as h:
        hpp.writelines([process_line(L) for L in h.readlines()])

    hpp.write("\n\n\n\n\n")

    # Add address tables
    hpp.writelines("\n".join(make_addr_table_lines()))
