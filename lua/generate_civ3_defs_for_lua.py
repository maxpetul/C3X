
import re

aliases = {}

def remove_c_comments(source):
    # The regular expression pattern to match C-style comments
    pattern = r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"'
    regex = re.compile(pattern, re.DOTALL | re.MULTILINE)

    # The function to replace the matched patterns
    def replacer(match):
        # If the match is not a comment (i.e., it is a string), return it as is
        if match.group(0).startswith('/') or match.group(0).startswith('*'):
            return ""  # if it's a comment, replace it with nothing
        else:
            return match.group(0)  # else return the match

    return regex.sub(replacer, source)

def convert_to_camel_case(identifier, capitalize_first=False):
    parts = identifier.split("_")
    parts[0] = (parts[0][0].capitalize() if capitalize_first else parts[0][0].lower()) + parts[0][1:]
    for i in range(1, len(parts)):
        parts[i] = parts[i][0].capitalize() + parts[i][1:] # Capitalize first letter
    return "".join(parts)
assert(all(convert_to_camel_case(x, True ) == "FooBar" for x in ["foo_bar", "fooBar", "FooBar", "Foo_Bar"]))
assert(all(convert_to_camel_case(x, False) == "fooBar" for x in ["foo_bar", "fooBar", "FooBar", "Foo_Bar"]))
assert(convert_to_camel_case("Player_ID", False) == "playerID")

def extract_structs(source):
    struct_pattern = r'struct\s+(\w+)\s*\{([^}]*)\}\s*;'
    regex = re.compile(struct_pattern, re.MULTILINE | re.DOTALL)

    struct_declarations = regex.findall(source)

    struct_dict = {}
    for struct_name, struct_body in struct_declarations:
        members = [member.strip() for member in struct_body.split(';') if member.strip()]
        struct_dict[struct_name] = members
    return struct_dict

def extract_enums(source):
    tr = {}

    enum_pattern = r'enum\s+(\w+)\s*\{([^}]*)\}\s*;'
    decls = re.compile(enum_pattern, re.MULTILINE | re.DOTALL).findall(source)
    for name, body in decls:
        defs = [d.strip() for d in body.split(",") if d.strip()]
        tr[name] = defs

    typedef_enum_pattern = r'typedef\s+enum\s+(\w+)\s*\{([^}]*)\}\s*(\w+);'
    decls = re.compile(typedef_enum_pattern, re.MULTILINE | re.DOTALL).findall(source)
    for _, body, name in decls:
        defs = [d.strip() for d in body.split(",") if d.strip()]
        tr[name] = defs

    return tr

def extract_member_info(struct_dict, enum_dict, member):
    unsigned = member.startswith("unsigned")

    # Regular expression pattern to match member type and name.
    pattern = r'(\b[\w]+\b\s*\**?)\s*(\b\w+\b)(\[\d+\])?'
    regex = re.compile(pattern)

    match = regex.match(member.strip(';'))
    if match is not None:
        # First group is the type
        member_type = match.group(1).strip()
        # Second group is the name
        member_name = match.group(2).strip()
        # Third group is the array size (if exists)
        array_size = match.group(3)

        # Ugly fix for unsigned + char/short/int. The regex above won't work b/c it matches the integer type as the variable name. To fix, drop
        # "unsigned" from the type, extract info, then tack it back on.
        if unsigned and member_name in ["char", "short", "int"]:
            n, t, s, a = extract_member_info(struct_dict, enum_dict, member.removeprefix("unsigned ").strip())
            return n, "unsigned " + t, s, a

        # Calculate the size of the member
        member_size, member_alignment = compute_member_size(struct_dict, enum_dict, member_type, array_size)

        return member_name, member_type, member_size, member_alignment

    # TODO: Real handling of func pointers
    elif any([x in member for x in ["__fastcall", "__thiscall", "__cdecl", "__stdcall"]]):
        return "func_ptr", "void *", 4, 4

    else:
        raise Exception(f"Cannot extract info for struct member \"{member}\"")

def extract_func_ptr_info(member):
    # Regular expression pattern to match function pointers
    pattern = r'(\b[\w]+\b\s*\*?\s*)\s*\(__\w+\s*\*\s*(\b\w+\b)\)\s*\(([\w\s,\*]*)\)'
    regex = re.compile(pattern)

    match = regex.match(member.strip(';'))
    if match is not None:
        # First group is the return type
        return_type = match.group(1).strip()
        # Second group is the name
        func_ptr_name = match.group(2).strip()
        # Third group is the list of arguments
        arg_list = match.group(3).strip()

        # Capture calling convention from member string
        calling_convention = re.search(r'\(__(\w+)', member).group(1)
        
        func_ptr_type = f'{return_type} ({calling_convention} *) ({arg_list})'
        is_func_ptr = True

        return func_ptr_name, func_ptr_type, is_func_ptr
    else:
        return None

def compute_member_size(struct_dict, enum_dict, member_type, array_size="1"):
    fundamental_type_sizes = {
        'byte': 1,
        'char': 1, 'unsigned char': 1,
        'short': 2, 'unsigned short': 2,
        'int': 4, 'unsigned int': 4, 'unsigned': 4,
        'float': 4,
        'void*': 4,  # Assuming a 32-bit system
    }

    if array_size:
        # Remove brackets and convert to int
        array_size = int(array_size.strip('[]'))
    else:
        array_size = 1  # For non-array types, size multiplier is 1

    struct_alignment = None
    # If member is a pointer, its size is the size of a void pointer
    if '*' in member_type:
        type_size = fundamental_type_sizes["void*"]
    elif member_type in fundamental_type_sizes:
        type_size = fundamental_type_sizes[member_type]
    elif member_type in struct_dict:
        type_size, _, struct_alignment = compute_struct_layout(struct_dict, enum_dict, member_type)
    elif member_type in enum_dict:
        type_size = fundamental_type_sizes["int"]
    elif member_type == "enum": # TODO: Actual parsing of non-typedef'd enums
        type_size = fundamental_type_sizes["int"]
    else:
        raise Exception(f"Struct member type \"{member_type}\" not recognized")

    alignment = type_size if struct_alignment is None else struct_alignment
    return type_size * array_size, alignment

def align(size, alignment):
    rem = size % alignment
    if rem == 0:
        return size
    else:
        return size + alignment - rem
    
def compute_struct_layout(struct_dict, enum_dict, name):
    offsets = []
    struct_size = 0
    strictest_member_alignment = 1
    for member in struct_dict[name]:
        if extract_func_ptr_info(member) is not None:
            member_size = 4
            member_alignment = 4
        else:
            _, _, member_size, member_alignment = extract_member_info(struct_dict, enum_dict, member)
        strictest_member_alignment = max(strictest_member_alignment, member_alignment)
        offsets.append(align(struct_size, member_alignment))
        struct_size = offsets[-1] + member_size
    return align(struct_size, strictest_member_alignment), offsets, strictest_member_alignment

# Modifies the struct_dict to inline one struct type into another
# For example, inline(ss, "Tile_Body", "Tile") will inline Tile_Body into the Tile struct
def inline(struct_dict, struct_type_to_inline, target_struct_type):
    inlined = []
    for m in struct_dict[target_struct_type]:
        m = m.removeprefix("struct ")
        type_name = m.split(" ")[0]
        if type_name == struct_type_to_inline:
            inlined.extend(struct_dict[type_name])
        else:
            inlined.append(m)
    struct_dict[target_struct_type] = inlined

def print_member_offsets(struct_dict, enum_dict, name):
    _, offsets, _ = compute_struct_layout(struct_dict, enum_dict, name)
    for mem_name, offset in zip(struct_dict[name], offsets):
        print(f"{offset}\t{mem_name}")

def prepend_dummy_structs(content, dummy_structs):
    dummy_defs = ""
    for name, size in dummy_structs.items():
        dummy_defs += f"struct {name} {{ char dummy[{size}]; }};\n"
    return dummy_defs + content

opaque_win_structs = {
    "RTL_CRITICAL_SECTION": 24,
    "RECT": 16,
    "BITMAPINFO": 44,
    "HRGN": 4,
    "HBITMAP": 4,
    "HDC": 4,
}

# "defines" is a dictionary mapping struct names to lists. Each list specifies which members to include in the exported defs, any others will be left
# opaque.
def generate_civ3_defs_for_lua(struct_dict, proced_struct_dict, enum_dict, defines):
    from collections import OrderedDict

    ss = struct_dict
    pss = proced_struct_dict
    es = enum_dict

    # Takes a struct type name returns a list of struct type names that are contained in the struct, possibly indirectly.
    # Example: "Tile" -> ["Base", "Tile_Body"]
    def gather_deps(s_name):
        tr = []
        for m in pss[s_name]: # For each member of s_name
            member_name, member_type, _, _ = m
            is_exported = (s_name in defines) and (member_name in defines[s_name])
            if (member_type in ss) and is_exported and not (member_type in tr): # If that member is a struct, is to be exported, and not already in the list of deps
                tr.extend(gather_deps(member_type)) # Recursively add its deps to the list
                tr.append(member_type) # Add the member type itself to the list
        return tr

    # Reorder & extend "defines" so all the structs are preceded by their dependencies
    ordered_defines = OrderedDict()
    for s_name in defines.keys():
        for dep_name in gather_deps(s_name):
            if not dep_name in ordered_defines:
                ordered_defines[dep_name] = []
        ordered_defines[s_name] = defines[s_name]

    def export_struct_name(n): return convert_to_camel_case(aliases.get(n, n), True)
    def export_member_name(n): return convert_to_camel_case(aliases.get(n, n), False)

    tr = ""
    for struct_name, included_members in ordered_defines.items():
        tr += f"typedef struct s_{export_struct_name(struct_name)}\n{{\n"

        struct_size, offsets, _ = compute_struct_layout(ss, es, struct_name)
        opaque_counter = 0
        running_size = 0
        for m, offset in zip(pss[struct_name], offsets):
            member_name, member_type, member_size, member_alignment = m
            if member_name in included_members:
                current_offset = align(running_size, member_alignment)
                if current_offset < offset: # If we need padding
                    tr += f"\tbyte _opaque_{opaque_counter}[{offset - current_offset}];\n"
                    opaque_counter += 1
                tr += f"\t{member_type} {export_member_name(member_name)};\n";
                running_size = offset + member_size

        # Insert padding at end of struct
        if running_size < struct_size:
            tr += f"\tbyte _opaque_{opaque_counter}[{struct_size - running_size}];\n"

        tr += f"}} {export_struct_name(struct_name)};\n\n"

    return tr;

with open("../Civ3Conquests.h", "r") as f:
    header = f.read()
    header = remove_c_comments(header)
    header = prepend_dummy_structs(header, opaque_win_structs)
ss = extract_structs(header)
es = extract_enums(header)

# Inline bodies into main structs
inline(ss, "Tile_Body", "Tile")

nonvtable_structs = {name: ss[name] for name in ss.keys() if not "vtable" in name}

# Process struct dict
pss = {}
for name, members in ss.items():
    proced_members = [extract_member_info(ss, es, m) for m in members]
    pss[name] = proced_members


# Generates C code that can be added to injected_code.c to check that all the sizes we've computed match the real sizes
# for name in ss.keys():
#     size = compute_struct_layout(ss, es, name)[0]
#     print(f"num_correct += {size} == sizeof({name});")
#     print(f"if ({size} != sizeof({name}))")
#     print(f'\tMessageBoxA (NULL, "{name}", NULL, MB_ICONINFORMATION);')
# print(f"total_count = {len(ss)};")
