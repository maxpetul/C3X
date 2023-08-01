
import re

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
    # Regular expression pattern to match member type and name
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
        # Calculate the size of the member
        member_size = compute_member_size(struct_dict, enum_dict, member_type, array_size)
        return member_name, member_type, member_size
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

    # If member is a pointer, its size is the size of a void pointer
    if '*' in member_type:
        type_size = fundamental_type_sizes["void*"]
    elif member_type in fundamental_type_sizes:
        type_size = fundamental_type_sizes[member_type]
    elif member_type in struct_dict:
        type_size = compute_struct_size(struct_dict, enum_dict, member_type)
    elif member_type in enum_dict:
        type_size = fundamental_type_sizes["int"]
    elif member_type == "enum": # TODO: Actual parsing of non-typedef'd enums
        type_size = fundamental_type_sizes["int"]
    else:
        raise Exception(f"Struct member type \"{member_type}\" not recognized")

    return type_size * array_size

def align(size, alignment):
    rem = size % alignment
    if rem == 0:
        return size
    else:
        return size + alignment - rem
    
def compute_struct_size(struct_dict, enum_dict, name):
    struct_size = 0
    strictest_member_alignment = 1
    for member in struct_dict[name]:
        if extract_func_ptr_info(member) is not None:
            member_size = 4
        else:
            _, _, member_size = extract_member_info(struct_dict, enum_dict, member)
        member_alignment = min(4, member_size) # Assume we don't have any alignment requirements stricter than 4 bytes.
        strictest_member_alignment = max(strictest_member_alignment, member_alignment)
        struct_size = align(struct_size, member_alignment) + member_size
    return align(struct_size, strictest_member_alignment)

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

def prepend_dummy_structs(content, dummy_structs):
    dummy_defs = ""
    for name, size in dummy_structs.items():
        dummy_defs += f"struct {name} {{ char dummy[{size}]; }};\n"
    return dummy_defs + content

# TODO: Fill in actual sizes
opaque_win_structs = {
    "RTL_CRITICAL_SECTION": 4,
    "RECT": 4,
    "BITMAPINFO": 4,
    "HRGN": 4,
    "HBITMAP": 4,
    "HDC": 4,
}

with open("../Civ3Conquests.h", "r") as f:
    header = f.read()
    header = remove_c_comments(header)
    header = prepend_dummy_structs(header, opaque_win_structs)
ss = extract_structs(header)
es = extract_enums(header)
nonvtable_structs = {name: ss[name] for name in ss.keys() if not "vtable" in name}
s_sizes = {name: compute_struct_size(ss, es, name) for name in ss.keys()}
