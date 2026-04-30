#Script to read in type info from Civ3Conquests.h
#@author 
#@category _NEW_
#@keybinding 
#@menupath 
#@toolbar 

# IMPORTANT NOTE if you're using this script for the first time in a new project. You need to copy certain
# Windows types (HBITMAP, etc.) from Ghidra's VS category into the Civ3Conquests.exe category otherwise
# getDataTypes won't be able to find them. Specifically, you need to copy over the subcategories "wingdi.h"
# and "WinDef.h". I took them from windows_vs12_32 but it probably works with other VS versions too.

import re

from ghidra.program.model.data import DataTypeManager, CategoryPath, ArrayDataType, PointerDataType, StructureDataType

decl         = re.compile (r"(\w+) (\w+);")
array_decl   = re.compile (r"(\w+) (\w+)\[(\d+)\];")
ptr_decl     = re.compile (r"(\w+) \*(\w+);")
ptr_ptr_decl = re.compile (r"(\w+) \*\*(\w+);")
arr_ptr_decl = re.compile (r"(\w+) \*(\w+)\[(\d+)\];")

int_type = getDataTypes ("int")[0]
int_array_type = ArrayDataType (int_type, 5, 4) # DataType, NumElements, ElementLength
int_ptr_type = PointerDataType (int_type)
generic_ptr_type = PointerDataType ()

# If try_get_type is set to true, returns None instead of raising an exception on failure.
known_types = {}
def get_type (name, try_get_type=False):
    if name in known_types:
        return known_types[name]
    else:
        types = getDataTypes (name)
        L = len (types)
        if L == 1:
            known_types[name] = types[0]
            return types[0]
        elif try_get_type:
            return None
        elif L > 1:
            raise Exception ("Ambiguous type name: \"" + name + "\"")
        else:
            raise Exception ("Unknown type name: \"" + name + "\"")

def get_pointer_type (name):
    if name != "void":
        return PointerDataType (get_type (name))
    else:
        return PointerDataType ()

def trim_struct_name (name):
    if name.startswith ("struct_"):
        return name[7:]
    elif name.startswith ("class_"):
        return name[6:]
    else:
        return name

# print (dir (currentProgram.getDataTypeManager ()))
# print (dir (currentProgram.getDataTypeManager ().getRootCategory ()))
# print (currentProgram.getDataTypeManager ().getRootCategory ().getCategories ())
# print ("===")
# print (currentProgram.getDataTypeManager ().getDataTypes ("HBITMAP"))
# x = get_type ("HBITMAP")
dtm = currentProgram.getDataTypeManager ()
# print (cat)
# for n in range (dtm.getCategoryCount ()):
#     print (currentProgram.getDataTypeManager ().getCategory (n))
# print (currentProgram.getDataTypeManager ().getDataType ("windows_vs12_32/", "HBITMAP"))


# dtm = currentProgram.getDataTypeManager ()
# transaction = dtm.startTransaction ("Test categories")
# cat = dtm.createCategory (CategoryPath ("/TestCreateCategory"))
# ss = StructureDataType ("TestRecursiveStruct", 0)
# pss = PointerDataType (ss)
# ss.add (pss, "node", "recursive reference")
# cat.addDataType (ss, None)
# dtm.endTransaction (transaction, True)
# raise Exception ("stop here")

# print (Array)
# print (len (getDataTypes ("Difficulty[5]")))
# print (len (getDataTypes ("Difficulty *")))



# diff = getDataTypes ("Difficulty")[0]
# print (dir (diff))
# newcomp = diff.add (generic_ptr_type, "testing_add_void_ptr_comp", "Hopefully this is a void *.")
# print (dir (newcomp))
# raise Exception ("done now")


filepath = askString ("Enter path to file containing declarations", "Filepath:")
# cat_name = askString ("Enter name for destination category", "Category name:")


# Open file, split into lines, strip them, and discard empty and comment ones
lines = []
with open (filepath, "r") as f:
    for L in f.read ().split ("\n"):
        L = L.strip ()
        if (len (L) > 0) and not L.startswith ("//"):
            lines.append (L)
    
# Each component is a tuple of four items: (kind, base, name, param). kind can be "simple", "array",
# "pointer", or "array of pointers". simple types only have a base type and name; pointer types
# use the 'param' to store the number of levels of indirection; arrays use 'param' to store the
# array count. Examples:
#   int x; --> ("simple", "int", "x", None)
#   char name[24]; --> ("array", "char", "name", 24)
#   Foo *f; --> ("pointer", "Foo", "f", 1)
#   Foo **f; --> ("pointer", "Foo", "f", 2)
#   short *fs[2]; --> ("array of pointers", "short", "fs", 2)
# The point of parsing into this components form before creating the DataTypes is so that we can
# sort the list of struct by dependency.
def parse_components (lines):
    tr = []
    for line in lines:
        m = re.match (decl, line)
        if m is not None:
            # comps.append ((get_type (m.group (1)), m.group (2)))
            tr.append (("simple", trim_struct_name (m.group (1)), m.group (2), None))
            continue
        m = re.match (array_decl, line)
        if m is not None:
            # t = get_type (m.group (1))
            # comps.append ((ArrayDataType (t, int (m.group (3)), t.getLength ()), m.group (2)))
            tr.append (("array", trim_struct_name (m.group (1)), m.group (2), int (m.group (3))))
            continue
        m = re.match (ptr_decl, line)
        if m is not None:
            # t = PointerDataType (get_type (m.group (1))) if m.group (1) != "void" else PointerDataType ()
            # comps.append ((t, m.group (2)))
            tr.append (("pointer", trim_struct_name (m.group (1)), m.group (2), 1))
            continue
        m = re.match (ptr_ptr_decl, line)
        if m is not None:
            tr.append (("pointer", trim_struct_name (m.group (1)), m.group (2), 2))
            continue
        m = re.match (arr_ptr_decl, line)
        if m is not None:
            tr.append (("array of pointers", trim_struct_name (m.group (1)), m.group (2), int (m.group (3))))
            continue
        raise Exception ("The following line didn't match any pattern:\n\t" + line)
    return tr

# Find all structs in the file. Each entry in this list is a tuple of (name, is_union, components)
def extract_structs (lines):
    tr = []
    n = 0
    while n < len (lines):
        is_union = lines[n].startswith ("union ")
        if lines[n].startswith ("struct ") or is_union:
            struct_name = trim_struct_name (lines[n].split (" ")[1])
            if lines[n+1] != "{": raise Exception ("Expected '{' after struct")
            n += 2
            struct_lines = []
            while lines[n] != "};":
                struct_lines.append (lines[n])
                n += 1
            tr.append ((struct_name, is_union, parse_components (struct_lines)))
        elif lines[n].startswith ("enum "):
            # ignore enums for now
            if lines[n+1] != "{": raise Exception ("Expected '{' after enum")
            n += 2
            while lines[n] != "};":
                n += 1
        else:
            raise Exception ("Invalid line (expected struct or enum):\n\t" + lines[n])
        n += 1
    return tr

structs = extract_structs (lines)

def find_unknown_types (structs):
    struct_names = {}
    tr = []
    for s in structs:
        struct_names[s[0]] = True
    for s in structs:
        struct_name, is_union, comps = s
        for c in comps:
            kind, base, comp_name, param = c
            if (base != "void") and (base not in struct_names) and (get_type (base, True) is None):
                tr.append (struct_name + "." + comp_name)
    return tr

unknown_types = find_unknown_types (structs)
if len (unknown_types) > 0:
    print ("Couldn't get types for the following struct members:\n\t" + "\n\t".join (unknown_types))
    raise Exception ("Unknown or ambiguous types")

# Returns list of structs sorted by dependency, i.e., the list reordered so that every struct B that's
# dependent on the definition of some struct A comes after A in the list. Assumes all structs are
# forward declared, i.e., if one refers to another through a pointer that doesn't count as a dependency.
def sort_structs_by_dependency (structs):
    tr = []
    sorted_structs = {}
    cant_satisfy_deps = lambda (sym): (sym != "void") and (sym not in sorted_structs) and (get_type (sym, True) is None)
    while True:
        sorted_any_this_pass = False
        for s in structs:
            struct_name, is_union, comps = s
            if struct_name not in sorted_structs:
                can_append = True
                for c in comps:
                    kind, base, name, param = c
                    if (kind == "simple" or kind == "array") and cant_satisfy_deps (base):
                        can_append = False
                        break
                if can_append:
                    tr.append (s)
                    sorted_structs[struct_name] = True
                    sorted_any_this_pass = True
        # if done
        if len (sorted_structs.keys ()) == len (structs): # done
            break
        # if not done and failed to sort any this pass
        elif not sorted_any_this_pass:
            raise Exception ("Some structs contain invalid (ambiguous, unknown) types or circular dependencies")
        # else proceed to next pass
    return tr

structs = sort_structs_by_dependency (structs)

# Quick and dirty code to just print the struct decls out in sorted order. Had to rewrite the sorting function
# because otherwise all dependencies are solved by the program's database (all these have already been imported
# after all).
#
# def sort_by_deps_2 (structs):
#     tr = []
#     sorted_structs = {}
#     unsorted_structs = {s[0] : True for s in structs}
#     while True:
#         sorted_any_this_pass = False
#         for s in structs:
#             struct_name, is_union, comps = s
#             if struct_name not in sorted_structs:
#                 can_append = True
#                 for c in comps:
#                     kind, base, name, param = c
#                     if (kind == "simple" or kind == "array") and base in unsorted_structs:
#                         can_append = False
#                         break
#                 if can_append:
#                     tr.append (s)
#                     sorted_structs[struct_name] = True
#                     unsorted_structs.pop (struct_name)
#                     sorted_any_this_pass = True
#         if len (tr) == len (structs):
#             break
#         elif not sorted_any_this_pass:
#             raise Exception ("Failed to sort some structs")
#     return tr
# structs = sort_by_deps_2 (structs)
# for s in structs:
#     struct_name, is_union, comps = s
#     t = "struct" if not is_union else "union"
#     print (t + " " + struct_name)
# raise Exception ("Stop here")

# Create all struct types first before filling them in so that recursive references work
struct_types = {}
for s in structs:
    struct_name, is_union, comps = s
    struct_types[struct_name] = StructureDataType (struct_name, 0)
    known_types[struct_name] = struct_types[struct_name]

# Fill in all struct components
for s in structs:
    struct_name, is_union, comps = s
    st = struct_types[struct_name]
    for c in comps:
        kind, base, comp_name, param = c
        if kind == "simple":
            st.add (get_type (base), comp_name, "")
        elif kind == "array":
            t = get_type (base)
            st.add (ArrayDataType (t, param, t.getLength ()), comp_name, "")
        elif kind == "pointer":
            p = get_pointer_type (base)
            for n in range (param - 1):
                p = PointerDataType (p)
            st.add (p, comp_name, "")
        elif kind == "array of pointers":
            p = get_pointer_type (base)
            st.add (ArrayDataType (p, param, p.getLength ()), comp_name, "")

# Add the structs to the type database
# Adding to a category doesn't work, it just dumps all the types in the main program category too.
# I can't be bothered to figure out how to make it work.
transaction = dtm.startTransaction ("Add structs from " + filepath)
# cat = dtm.createCategory (CategoryPath ("/" + cat_name))
for s in struct_types.values ():
    dtm.addDataType (s, None)
dtm.endTransaction (transaction, True)

