
# Reads in Civ3Conquests.h from the parent directory, converts it for C++, and writes it back out (into this directory) as Civ3Conquests.hpp

from pathlib import Path

def process_line(L):
    L = L.replace("this", "_this");

    s = L.split("__fastcall")
    if len(s) == 1:
        return s[0]
    elif len(s) == 2:
        return s[0] + "__thiscall" + s[1].replace("__,", "")
    else:
        raise Exception("Invalid line: " + L)

with open(Path.cwd().parent.joinpath("Civ3Conquests.h"), mode="r") as h:
    with open("Civ3Conquests.hpp", mode="w") as hpp:
        hpp.writelines([process_line(L) for L in h.readlines()])
