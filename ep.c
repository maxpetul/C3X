
#include "stdlib.h"
#include "stdio.h"

#include "tcc/libtcc/libtcc.h"
#include "lend/ld32.c"

#include "C3X.h"
#include "common.c"

#if (!defined(C3X_RUN) && !defined(C3X_INSTALL)) || (defined(C3X_RUN) && defined(C3X_INSTALL))
#error "Must #define exactly one of C3X_RUN or C3X_INSTALL"
#endif

// Large (30 MB) buffer whose job is to occupy the lower portion of this process' virtual address space. This is necessary
// for installing the mod into the Civ EXE because we need space in this area to relocate the code that is to be injected
// into the EXE.
// More specifically, the root of the problem is that all sections in a PE file must be contiguous in virtual address space.
// I have no idea why that is, but the fact remains Windows won't load any PE with discontiguous sections. So the section
// containing the injected code must be after the original sections in the PE [*], which is around 0xCD0000 IIRC. The
// challenge then is getting some memory allocated in that area. VirtualAlloc is the obvious solution but that won't work
// because it won't give us memory at low addresses, again I don't know why, it just won't. The solution I settled on is to
// create my own EXE file with a large global array that will occupy the addresses needed. This array is low_addr_buf.
// The separate EXE is necessary b/c if this file is just run with tcc -run, low_addr_buf will be placed at a much higher
// address. I think this is because the lower addresses are occuped by the batch file processor and TCC itself but I don't
// know that for sure.
// [*] Another option would be to change the PE's image base, but that would ruin all of the offsets. They could be adjusted
//     to compensate, but I think this solution with the low address buffer is easier & simpler.
char low_addr_buf[30000000];

TCCState * (LIBTCCAPI * tcc__new)              ();
void       (LIBTCCAPI * tcc__delete)           (TCCState *);
void       (LIBTCCAPI * tcc__set_options)      (TCCState *, char const *);
int        (LIBTCCAPI * tcc__compile_string)   (TCCState *, char const *);
int        (LIBTCCAPI * tcc__relocate)         (TCCState *, void *);
void *     (LIBTCCAPI * tcc__get_symbol)       (TCCState *, char const *);
int        (LIBTCCAPI * tcc__add_library_path) (TCCState *, char const *);
int        (LIBTCCAPI * tcc__add_include_path) (TCCState *, char const *);
void       (LIBTCCAPI * tcc__define_symbol)    (TCCState *, char const *, char const *);
void       (LIBTCCAPI * tcc__list_symbols)     (TCCState *, void *, void (*) (void *, char const *, void const *));

struct c3c_binary {
	char const * title;
	int file_size;
	void * addr_rdata;
	int    size_rdata;
	int addr_column; // The index of the column in civ_prog_objects.csv that contains the addresses for this binary
} * bin;

//                                TITLE         FILE_SIZE   ADDR_RDATA          SIZE_RDATA   ADDR_COLUMN
struct c3c_binary gog_binary   = {"GOG",        3417464,    (void *)0x665000,   0x1B000,     1};
struct c3c_binary steam_binary = {"Steam",      3518464,    (void *)0x682000,   0x1B000,     2};
struct c3c_binary pcg_binary   = {"PCGames.de", 3395072,    (void *)0x665000,   0x1A400,     3};

char const * standard_exe_filename = "Civ3Conquests.exe";
char const * backup_exe_filename = "Civ3Conquests-Unmodded.exe";

char *
vformat (char const * str, va_list args)
{
        size_t z = 1000;
        char * tr = malloc (z);
        while (1) {
                int actual_z = vsnprintf (tr, z, str, args);
                if ((actual_z < 0) || ((size_t)actual_z < z)) {
                        tr[z - 1] = '\0'; // In case vsnprintf fails
                        break;
                } else {
                        z = (size_t)actual_z + 1;
                        tr = realloc (tr, z);
                }
        }
        return tr;
}

char *
format (char const * str, ...)
{
        va_list args;
        va_start (args, str);
        char * tr = vformat (str, args);
        va_end (args);
        return tr;
}

char temp_str[10000];

char *
temp_format (char const * str, ...)
{
	va_list args;
	va_start (args, str);
	vsnprintf (temp_str, sizeof temp_str, str, args);
	temp_str[(sizeof temp_str) - 1] = '\0';
	return temp_str;
}

void
quit_on_error (char const * err_str)
{
	MessageBox (NULL, err_str, NULL, MB_ICONERROR);
        ExitProcess (1);
}

void
quit_on_error_at (char const * err_str, char const * func_name, char const * file_name, int line_no)
{
        char * complete_msg = format ("%s:%d (in %s):\n%s", file_name, line_no, func_name, err_str);
        quit_on_error (complete_msg);
}

#define REQUIRE(expr, err_str) do { if (! (expr)) { quit_on_error_at (err_str, __func__, __FILE__, __LINE__); } } while (0)

#define ASSERT(expr) REQUIRE (expr, format ("Assertion failed:\n%s", #expr))

#define THROW(err_str) REQUIRE (0, err_str)

char *
get_last_error_str ()
{
	DWORD err_code = GetLastError ();
	if (err_code != 0) {
		int buf_size = 1000;
		char * tr = malloc (buf_size);
		FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				err_code,
				MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
				tr,
				buf_size,
				NULL);
		return tr;
	} else
		return "No error";
}

int
str_ends_with (char const * str, char const * ending)
{
	size_t str_len = strlen (str), ending_len = strlen (ending);
	return (ending_len <= str_len) && (0 == strcmp (&str[str_len - ending_len], ending));
}

// Like strcat but allocates and returns a new buffer
char *
combine_strs (char const * a, char const * b)
{
	size_t lena = strlen (a), lenb = strlen (b);
	char * tr = malloc (lena + lenb + 1);
	strcpy (tr, a);
	strcpy (tr + lena, b);
	tr[lena + lenb] = '\0';
	return tr;
}

// A wrapper around file_to_string (defined in common.c) for ep.c. Takes separate path & filename and aborts on error
char *
ep_file_to_string (char const * path, char const * filename)
{
	char * fullpath = malloc (MAX_PATH + strlen (filename) + 2);
	sprintf (fullpath, "%s\\%s", path, filename);
	char * tr = file_to_string (fullpath);
	REQUIRE (tr != NULL, format ("Failed to open or read file \"%s\"", fullpath));
	free (fullpath);
	return tr;
}

void
buffer_to_file (void * buf, int buf_size, char const * path, char const * filename)
{
	char * fullpath = malloc (MAX_PATH + strlen (filename) + 2);
	sprintf (fullpath, "%s\\%s", path, filename);
	HANDLE file = CreateFile (fullpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	REQUIRE (file != INVALID_HANDLE_VALUE, format ("Failed to open file \"%s\" for writing:\n%s", fullpath, get_last_error_str ()));
	DWORD size_written = 0;
	int success = WriteFile (file, buf, buf_size, &size_written, NULL);
	REQUIRE (success && (size_written == buf_size), format ("Failed to write to \"%s\"", fullpath));
	CloseHandle (file);
	free (fullpath);
}

void
tcc_define_pointer (TCCState * tcc, char const * name, void * p)
{
	char s[100];
	snprintf (s, sizeof s, "((void *)0x%p)", p);
	tcc__define_symbol (tcc, name, s);
}

enum DATA_FIELD {
	DF_EXPORT_TABLE = 0,
	DF_IMPORT_TABLE,
	DF_RESOURCE_TABLE,
	DF_EXCEPTION_TABLE,
	DF_CERTIFICATE_TABLE,
	DF_BASE_RELOCATION_TABLE,
	DF_DEBUG,
	DF_ARCHITECTURE,
	DF_GLOBAL_PTR,
	DF_TLS_TABLE,
	DF_LOAD_CONFIG_TABLE,
	DF_BOUND_IMPORT,
	DF_IAT,
	DF_DELAY_IMPORT_DESCRIPTOR,
	DF_CLR_RUNTIME_HEADER,
	DF_RESERVED,

	DF_COUNT
};

#define IMAGE_SCN_CNT_CODE               0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_MEM_EXECUTE            0x20000000
#define IMAGE_SCN_MEM_READ               0x40000000
#define IMAGE_SCN_MEM_WRITE              0x80000000

struct civ_pe {
	IMAGE_DOS_HEADER * dos;
	IMAGE_FILE_HEADER * coff; // COFF header
	IMAGE_OPTIONAL_HEADER32 * opt;
	IMAGE_SECTION_HEADER * sections;
};

struct civ_pe
civ_pe_from_file_contents (byte * dat)
{
	struct civ_pe tr;
	tr.dos = (IMAGE_DOS_HEADER *)dat;
	IMAGE_NT_HEADERS32 * nt = (IMAGE_NT_HEADERS32 *)(dat + tr.dos->e_lfanew);
	tr.coff = &nt->FileHeader;
	tr.opt = &nt->OptionalHeader;
	tr.sections = (IMAGE_SECTION_HEADER *)((byte *)&nt->OptionalHeader + tr.coff->SizeOfOptionalHeader);
	return tr;
}

void
print_pe_sections (struct civ_pe const * pe)
{
	printf ("Name\tVAddr\tVSize\tRawSize\n");
	for (int n = 0; n < pe->coff->NumberOfSections; n++) {
		IMAGE_SECTION_HEADER * s = &pe->sections[n];
		printf ("%.8s\t%x\t%x\t%x\n", s->Name, s->VirtualAddress, s->Misc.VirtualSize, s->SizeOfRawData);
	}
}

// Converts a virtual address in the executable image to an offset in the file on disk
int
va_to_file_ptr (struct civ_pe const * pe, void const * virt_addr, int * out_section_index)
{
	int iva = (int)virt_addr;
	for (int n = 0; n < pe->coff->NumberOfSections; n++) {
		int section_base = pe->opt->ImageBase + pe->sections[n].VirtualAddress;
		int section_end = section_base + pe->sections[n].SizeOfRawData;
		if ((iva >= section_base) && (iva < section_end)) {
			if (out_section_index)
				*out_section_index = n;
			return pe->sections[n].PointerToRawData + iva - section_base;
		}
	}
	if (out_section_index)
		*out_section_index = -1;
	return 0;
}





#ifdef C3X_RUN
HANDLE civ_proc;
#else
struct civ_pe civ_exe;
byte * civ_exe_contents; // Buffer contains the contents of the EXE file
int civ_exe_buf_size;
int civ_exe_buf_occupied_size;
#endif



#ifdef C3X_INSTALL
byte *
get_exe_ptr (void const * virt_addr)
{
	int i_section;
	int offset = va_to_file_ptr (&civ_exe, virt_addr, &i_section);
	REQUIRE (i_section >= 0, format ("Virtual address %p does not correspond to any data in EXE file", virt_addr));
	return civ_exe_contents + offset;
}
#endif

byte *
read_prog_memory (void const * addr, int size)
{
	byte * tr = malloc (size);
#ifdef C3X_RUN
	SIZE_T size_read;
	int success = ReadProcessMemory (civ_proc, addr, tr, size, &size_read);
	ASSERT (success && ((int)size_read == size));
#else
	memcpy (tr, get_exe_ptr (addr), size);
#endif
	return tr;
}

int
read_prog_int (void const * addr)
{
	int tr;
#ifdef C3X_RUN
	SIZE_T size_read;
	int success = ReadProcessMemory (civ_proc, addr, &tr, sizeof tr, &size_read);
	ASSERT (success && (size_read == sizeof tr));
#else
	tr = int_from_bytes (get_exe_ptr (addr));
#endif
	return tr;
}

void
write_prog_memory (void * addr, byte const  * buf, int size)
{
#ifdef C3X_RUN
	SIZE_T size_written;
	int success = WriteProcessMemory (civ_proc, addr, buf, size, &size_written);
	ASSERT (success && ((int)size_written == size));
#else
	memcpy (get_exe_ptr (addr), buf, size);
#endif
}

void
write_prog_int (void * addr, int val)
{
	write_prog_memory (addr, (byte const *)&val, sizeof val);
}

enum MEM_ALLOWED_ACCESS {
	MAA_READ = 0,
	MAA_READ_WRITE,
	MAA_READ_EXECUTE,
	MAA_READ_WRITE_EXECUTE,

	COUNT_MAA_MODES
};

int page_prot_flags[COUNT_MAA_MODES] = {PAGE_READONLY, PAGE_READWRITE, PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE};

int section_flags[COUNT_MAA_MODES] = {
	IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
	IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE,
	IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE,
	IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE,
};

int
align_pow2 (int alignment, int val)
{
	int amo = alignment - 1;
	return (val + amo) & ~amo;
}

void
recompute_civ_pe_sizes (struct civ_pe * pe)
{
	int sz_code = 0, sz_inited_data = 0, sz_uninited_data = 0, sz_headers = 0, sz_image = 0;
	for (int n = 0; n < pe->coff->NumberOfSections; n++) {
		int flags = pe->sections[n].Characteristics;
		int size = align_pow2 (pe->opt->SectionAlignment, pe->sections[n].Misc.VirtualSize);
		if (flags & IMAGE_SCN_CNT_CODE)		      sz_code          += size;
		if (flags & IMAGE_SCN_CNT_INITIALIZED_DATA)   sz_inited_data   += size;
		if (flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) sz_uninited_data += size;
	}

	sz_headers = align_pow2 (pe->opt->FileAlignment, ((byte *)pe->sections - (byte *)pe->dos) + (pe->coff->NumberOfSections * sizeof pe->sections[0]));
	sz_image = align_pow2 (pe->opt->SectionAlignment, sz_code + sz_inited_data + sz_uninited_data + sz_headers);

	pe->opt->SizeOfCode = sz_code;
	pe->opt->SizeOfInitializedData = sz_inited_data;
	pe->opt->SizeOfUninitializedData = sz_uninited_data;
	pe->opt->SizeOfHeaders = sz_headers;
	pe->opt->SizeOfImage = sz_image;
}


void *
alloc_prog_memory (char const * name, void * location, int size, enum mem_access access)
{
#ifdef C3X_RUN
	void * tr = VirtualAllocEx (civ_proc, location, size, MEM_RESERVE | MEM_COMMIT, page_prot_flags[access]);
	REQUIRE (tr != NULL, format ("Bad in-program alloc (location: %p, size: %d, access: %d)", location, size, access));
	return tr;
#else
	REQUIRE (location == NULL, "Can't specify location of alloc inside PE file");
	int virt_addr; {
		IMAGE_SECTION_HEADER * last_sect = &civ_exe.sections[civ_exe.coff->NumberOfSections - 1];
		virt_addr = align_pow2 (civ_exe.opt->SectionAlignment, last_sect->VirtualAddress + last_sect->Misc.VirtualSize);
	}

	size = align_pow2 (civ_exe.opt->FileAlignment, size);

	civ_exe_buf_occupied_size = align_pow2 (civ_exe.opt->FileAlignment, civ_exe_buf_occupied_size);
	int raw_ptr = civ_exe_buf_occupied_size;
	civ_exe_buf_occupied_size += size;

	IMAGE_SECTION_HEADER new_section = {
		.Name = {0},
		.Misc.VirtualSize = size,
		.VirtualAddress = virt_addr,
		.SizeOfRawData = size,
		.PointerToRawData = raw_ptr,
		.PointerToRelocations = 0,
		.PointerToLinenumbers = 0,
		.NumberOfRelocations = 0,
		.NumberOfLinenumbers = 0,
		.Characteristics = section_flags[access]
	};
	strncpy (new_section.Name, name, IMAGE_SIZEOF_SHORT_NAME);
	int i_new_section = (civ_exe.coff->NumberOfSections)++;
	memcpy (&civ_exe.sections[i_new_section], &new_section, sizeof new_section);

	recompute_civ_pe_sizes (&civ_exe);

	return (void *)(virt_addr + civ_exe.opt->ImageBase);
#endif
}

void
set_prog_mem_protection (void * addr, int size, enum mem_access access)
{
#ifdef C3X_RUN
	DWORD unused;
	int success = VirtualProtectEx (civ_proc, addr, size, page_prot_flags[access], &unused);
	REQUIRE (success, format ("Failed to set mem access at %p to kind %d", addr, access));
#else
	size = align_pow2 (civ_exe.opt->FileAlignment, size);
	int i_section;
	va_to_file_ptr (&civ_exe, addr, &i_section);
	REQUIRE (i_section >= 0, format ("Virtual address %p does not correspond to any data in EXE file", addr));
	REQUIRE (size == civ_exe.sections[i_section].Misc.VirtualSize, "Size mismatch when modifying section characteristics");
	civ_exe.sections[i_section].Characteristics = section_flags[access];
	recompute_civ_pe_sizes (&civ_exe);
#endif
}

// Writes an instruction redirecting control flow from "from" to "to". If "use_call_instr" is 1, a call is used, otherwise a jump is.
void
put_trampoline (void * from, void * to, int use_call_instr)
{
	byte code[5];
	code[0] = use_call_instr ? 0xE8 : 0xE9;
	int_to_bytes (&code[1], (int)to - ((int)from + 5));
	write_prog_memory (from, code, sizeof code);
}

// An inlead is an alternative entry point for a function. It contains a duplicate of at least the first 5 bytes of a target function
// followed by a jump instruction to the rest of that function. The purpose of an inlead is so that the first 5 bytes of the original
// function can be replaced with a trampoline but the original function can still be run by calling the inlead instead.
struct inlead {
	byte bytes[32];
};

// Initialize inlead object for a function in hProcess. "inlead" is a pointer to an inlead object alloced in hProcess' address space
// and func_addr is a pointer to a function in hProcess' address space.
void
init_inlead (struct inlead * inlead, int func_addr)
{
	byte * code = read_prog_memory ((void *)func_addr, sizeof *inlead);

	// The "header" is just the set of first bytes in the original function that we need to copy over to the inlead because they'll be
	// overwritten by the trampoline. The trampoline is only 5 bytes but the header might be longer since we can't stop copying in the
	// middle of an instruction.
	int header_size = 0;
	int i_call_instr = -1; // If we find a call instruction, record the index of its location so we can patch the offset later. Call
	// instructions are 5 bytes so we'll find at most one.
	while (header_size < 5) {
		if (code[header_size] == 0xE8)
			i_call_instr = header_size;
		header_size += length_disasm (&code[header_size]);
	}

	struct inlead towrite;
	memset (&towrite.bytes[0], 0xCC, sizeof towrite); // Fill with interrupt instructions
	memcpy (&towrite.bytes[0], code, header_size);
	if (i_call_instr >= 0) {
		int adjusted_offset = int_from_bytes (&towrite.bytes[i_call_instr + 1]) - ((int)inlead - func_addr);
		int_to_bytes (&towrite.bytes[i_call_instr + 1], adjusted_offset);
	}

	// Write jump from end of inlead to original function after header
	towrite.bytes[header_size] = 0xE9;
	int_to_bytes (&towrite.bytes[header_size + 1], func_addr + header_size - ((int)inlead + header_size + 5));

	write_prog_memory (inlead, (byte const *)&towrite, sizeof towrite);

	free (code);
}

enum bin_id {
	BIN_ID_NOT_FOUND,
	BIN_ID_GOG,
	BIN_ID_STEAM,
	BIN_ID_PCG,
	BIN_ID_MODDED,
	BIN_ID_UNRECOGNIZED,

	COUNT_BIN_IDS
};

char const * bin_id_strs[COUNT_BIN_IDS] = {"Not found", "GOG Complete", "Steam Complete", "PCGames.de", "Modded", "Unrecognized"};

enum bin_id
identify_binary (char const * file_name, HANDLE * out_file_handle)
{
	HANDLE file = CreateFile (file_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*out_file_handle = file;
	if (file == INVALID_HANDLE_VALUE)
		return BIN_ID_NOT_FOUND;
	DWORD size = GetFileSize (file, NULL);
	DWORD size_read;
	static byte header[5000]; // This was only make static so it doesn't take up too much stack space
	if (size == gog_binary.file_size)
		return BIN_ID_GOG;
	else if (size == steam_binary.file_size)
		return BIN_ID_STEAM;
	else if (size == pcg_binary.file_size)
		return BIN_ID_PCG;
	else if ((size > sizeof header) && (0 != ReadFile (file, header, sizeof header, &size_read, NULL)) && (size_read == sizeof header)) {
		int any_modded_sections = 0;
		struct civ_pe pe = civ_pe_from_file_contents (header);
		for (int n = 0; n < pe.coff->NumberOfSections; n++)
			if (0 == strncmp (".c3x", pe.sections[n].Name, 4)) {
				any_modded_sections = 1;
				break;
			}
		return any_modded_sections ? BIN_ID_MODDED : BIN_ID_UNRECOGNIZED;
	} else
		return BIN_ID_UNRECOGNIZED;
}

void
print_symbol_location (void * context, char const * name, void const * val)
{
	printf ("%p\t%s\n", val, name);
}

struct mangled_symbol_search {
	char * sym_name;
	int sym_name_len;
	void * matching_val;
	int count_matches;
};

void
match_mangled_symbol (void * vp_mss, char const * name, void const * val)
{
	struct mangled_symbol_search * mss = vp_mss;
	// Match against the name with '_' or '@' prepended and possibly followed by '@'. This isn't an exhaustive pattern so that's why we also track
	// ambiguities. The patterns are from here: https://docs.microsoft.com/en-us/cpp/build/reference/decorated-names?view=msvc-160 under "Format
	// of a C decorated name"
	if (((name[0] == '_') || (name[0] == '@')) &&
	    (0 == strncmp (&name[1], mss->sym_name, mss->sym_name_len)) &&
	    ((name[mss->sym_name_len + 1] == '\0') ||
	     (name[mss->sym_name_len + 1] == '@'))) {
		mss->matching_val = (void *)val;
		mss->count_matches++;
	}
}

// Returns the address of the patch function with the given name in the compiled code. Will automatically prepend "patch_" to the name (if
// prepend_patch is set) and attempt to compensate for name mangling.
void *
find_patch_function (TCCState * tcc, char const * obj_name, int prepend_patch)
{
	char * sym_name = prepend_patch ? temp_format ("patch_%s", obj_name) : (char *)obj_name;

	void * val = tcc__get_symbol (tcc, sym_name);
	if (val != NULL)
		return val;

	struct mangled_symbol_search mss = {.sym_name = sym_name, .sym_name_len = strlen (sym_name), .matching_val = NULL, .count_matches = 0};
	tcc__list_symbols (tcc, &mss, match_mangled_symbol);
	REQUIRE (mss.count_matches > 0, format ("Symbol %s not found in compiled code", sym_name));
	REQUIRE (mss.count_matches < 2, format ("Symbol %s is ambiguous", sym_name));
	return mss.matching_val;
}

enum reg { REG_EAX = 0, REG_ECX, REG_EDX, REG_EBX, REG_EBP, REG_ESI, REG_EDI };

// This writes a call to intercept_consideration at the cursor. intercept_consideration takes a single parameter, the point value of the thing being
// considered and it returns a new, possibly modified, value for it. Because this call gets inserted into a stream of instructions we must take care
// not to modify any registers other than the one which stores the point value (specified by val_reg, must be either ebx or edi).
void
emit_consideration_intercept_call (byte ** p_cursor, byte * code_base, void * addr_intercept_function, byte * addr_airlock, enum reg val_reg)
{
	ASSERT ((val_reg == REG_EBX) || (val_reg == REG_EDI));

	byte * cursor = *p_cursor;

	*cursor++ = 0x50; // push eax
	*cursor++ = 0x51; // push ecx
	*cursor++ = 0x52; // push edx

	// push val_reg
	// call intercept_consideration
	*cursor++ = (val_reg == REG_EBX) ? 0x53 : 0x57;
	*cursor++ = 0xE8;
	cursor = int_to_bytes (cursor, (int)addr_intercept_function - ((int)addr_airlock + (cursor - code_base) + 4));

	// mov val_reg, eax
	*cursor++ = 0x89;
	*cursor++ = (val_reg == REG_EBX) ? 0xC3 : 0xC7;

	*cursor++ = 0x5A; // pop edx
	*cursor++ = 0x59; // pop ecx
	*cursor++ = 0x58; // pop eax

	*p_cursor = cursor;
}

enum jump_kind { JK_UNCOND = 0, JK_LESS };

void
emit_jump (byte ** p_cursor, byte * code_base, int jump_target, byte * addr_airlock, enum jump_kind kind)
{
	byte * cursor = *p_cursor;
	if (kind == JK_UNCOND)
		*cursor++ = 0xE9; // jmp
	else if (kind == JK_LESS) {
		*cursor++ = 0x0F; // | jl
		*cursor++ = 0x8C; // |
	}
	cursor = int_to_bytes (cursor, jump_target - ((int)addr_airlock + (cursor - code_base) + 4));
	*p_cursor = cursor;
}

// This function fills out two "airlocks" which are necessary pieces of the process of seeing and modifying the AI's production choices. There are two
// because the AI production chooser function loops separately over available improvements and units. Each of the airlocks contains instructions that
// wrap a call to intercept_consideration (which is located in the injected code), then duplicate the instructions that got overwritten by the jump to
// the airlock, then finally jump back into the original code.
// There are separate cases for the GOG and Steam builds of the game since those are separate compilations of the same source and so the stack layout
// and register usage is different. One major difference is that on the Steam build, the central cmp instruction that gets replaced with a jump to an
// airlock is only 4 bytes in size (unlike 7 on GOG) so b/c a jump occupies 5 bytes we must also overwrite the following instruction which is a jl. So
// we must replicate the functionality of that second jump, hence in the Steam case there are two jumps at the end of the airlocks. The first jumps
// past the block that's run if the last candidate has the highest value so far, and the second jumps into that block. This is unlike on GOG where
// there's only one jump that goes to the equivalent jl instruction in the base code.
void
init_consideration_airlocks (enum bin_id bin_id, TCCState * tcc, byte * addr_improv_airlock, byte * addr_unit_airlock)
{
	void * addr_intercept_consideration = find_patch_function (tcc, "intercept_consideration", 0);
	ASSERT (addr_intercept_consideration != NULL);

	byte code[64] = {0};
	byte * cursor = code;

	if ((bin_id == BIN_ID_GOG) || (bin_id == BIN_ID_PCG)) {
		emit_consideration_intercept_call (&cursor, code, addr_intercept_consideration, addr_improv_airlock, REG_EBX);
		byte cmp[] = {0x3B, 0x9C, 0x24, 0x94, 0x00, 0x00, 0x00}; // cmp ebx, dword ptr [esp+0x94]
		for (int n = 0; n < sizeof cmp; n++)
			*cursor++ = cmp[n];
		emit_jump (&cursor, code, (bin_id == BIN_ID_GOG) ? 0x430FC1 : 0x431041, addr_improv_airlock, JK_UNCOND);

	} else if (bin_id == BIN_ID_STEAM) {
		emit_consideration_intercept_call (&cursor, code, addr_intercept_consideration, addr_improv_airlock, REG_EDI);
		byte cmp[] = {0x3B, 0x7C, 0x24, 0x48}; // cmp edi, dword ptr [esp+0x48]
		for (int n = 0; n < sizeof cmp; n++)
			*cursor++ = cmp[n];
		emit_jump (&cursor, code, 0x432A87, addr_improv_airlock, JK_LESS);
		emit_jump (&cursor, code, 0x432A73, addr_improv_airlock, JK_UNCOND);

	} else
		THROW ("Invalid bin_id");

	write_prog_memory (addr_improv_airlock, &code[0], sizeof code);

	memset (code, 0, sizeof code);
	cursor = code;

	if ((bin_id == BIN_ID_GOG) || (bin_id == BIN_ID_PCG)) {
		emit_consideration_intercept_call (&cursor, code, addr_intercept_consideration, addr_unit_airlock, REG_EDI);
		byte cmp[] = {0x3B, 0xBC, 0x24, 0x94, 0x00, 0x00, 0x00}; // cmp edi, dword ptr [esp+0x94]
		for (int n = 0; n < sizeof cmp; n++)
			*cursor++ = cmp[n];
		emit_jump (&cursor, code, (bin_id == BIN_ID_GOG) ? 0x433C83 : 0x433D03, addr_unit_airlock, JK_UNCOND);

	} else if (bin_id == BIN_ID_STEAM) {
		emit_consideration_intercept_call (&cursor, code, addr_intercept_consideration, addr_unit_airlock, REG_EBX);
		byte cmp[] = {0x3B, 0x5C, 0x24, 0x48}; // cmp ebx, dword ptr [esp+0x48]
		for (int n = 0; n < sizeof cmp; n++)
			*cursor++ = cmp[n];
		emit_jump (&cursor, code, 0x435730, addr_unit_airlock, JK_LESS);
		emit_jump (&cursor, code, 0x435718, addr_unit_airlock, JK_UNCOND);
	}

	write_prog_memory (addr_unit_airlock, &code[0], sizeof code);
}

void
init_set_resource_bit_airlock (TCCState * tcc, byte * addr_airlock, int addr_intercept_set_resource_bit)
{
	void * addr_intercept_function = find_patch_function (tcc, "intercept_set_resource_bit", 0);
	ASSERT (addr_intercept_function != NULL);

	byte code[64] = {0};
	byte * cursor = code;

	*cursor++ = 0x50; // push eax
	*cursor++ = 0x51; // push ecx
	*cursor++ = 0x52; // push edx

	*cursor++ = 0x51; // push resource_id arg (in ecx)
	*cursor++ = 0x50; // push city arg (in eax)

	// call intercept_set_resource_bit
	*cursor++ = 0xE8;
	cursor = int_to_bytes (cursor, (int)addr_intercept_function - ((int)addr_airlock + (cursor - code) + 4));

	*cursor++ = 0x5A; // pop edx
	*cursor++ = 0x59; // pop ecx
	*cursor++ = 0x58; // pop eax

	// jump back to the original game code
	emit_jump (&cursor, code, addr_intercept_set_resource_bit + 6, addr_airlock, JK_UNCOND);

	write_prog_memory (addr_airlock, &code[0], sizeof code);
}

// Because we're compiling with -nostdlib the entry point isn't main but is rather _runmain (if running immediately like a script) or _start (if
// compiling to an EXE file)
#ifdef C3X_INSTALL
#define ENTRY_POINT _start
#else
#define ENTRY_POINT _runmain
#endif

void
ENTRY_POINT ()
{
	DWORD unused;
	int success;

	// Change to Conquests directory
	char mod_full_dir[MAX_PATH],
	     conquests_dir[MAX_PATH+2];
	GetCurrentDirectory (sizeof mod_full_dir, mod_full_dir);
	memcpy (conquests_dir, mod_full_dir, sizeof mod_full_dir);
	char * conq_folder = strstr (conquests_dir, "\\Conquests");
	REQUIRE (conq_folder != NULL, "Must be run from subfolder of Conquests directory.");
	int i_after_conq_folder = conq_folder - conquests_dir + strlen ("\\Conquests");
	conquests_dir[i_after_conq_folder    ] = '\\';
	conquests_dir[i_after_conq_folder + 1] = '\0';
	SetCurrentDirectory (conquests_dir);

	char * mod_rel_dir = &mod_full_dir[i_after_conq_folder];
	if (*mod_rel_dir == '\\')
		mod_rel_dir++;
	if (*mod_rel_dir == '\0')
		mod_rel_dir = ".";

	HMODULE libtcc;
	if ((NULL == (libtcc = GetModuleHandleA ("libtcc.dll"))) &&
	    (NULL == (libtcc = LoadLibrary ("tcc\\libtcc.dll"))) &&
	    (NULL == (libtcc = LoadLibrary ("libtcc.dll"))))
		THROW ("Couldn't load TCC. Try placing libtcc.dll in the same directory as the script.");
	tcc__new              = (void *)GetProcAddress (libtcc, "tcc_new");
	tcc__delete           = (void *)GetProcAddress (libtcc, "tcc_delete");
	tcc__set_options      = (void *)GetProcAddress (libtcc, "tcc_set_options");
	tcc__compile_string   = (void *)GetProcAddress (libtcc, "tcc_compile_string");
	tcc__relocate         = (void *)GetProcAddress (libtcc, "tcc_relocate");
	tcc__get_symbol       = (void *)GetProcAddress (libtcc, "tcc_get_symbol");
	tcc__add_library_path = (void *)GetProcAddress (libtcc, "tcc_add_library_path");
	tcc__add_include_path = (void *)GetProcAddress (libtcc, "tcc_add_include_path");
	tcc__define_symbol    = (void *)GetProcAddress (libtcc, "tcc_define_symbol");
	tcc__list_symbols     = (void *)GetProcAddress (libtcc, "tcc_list_symbols");
	TCCState * tcc = tcc__new ();
	REQUIRE (tcc != NULL, "Failed to initialize TCC");
	tcc__set_options (tcc, "-m32");
	tcc__add_include_path (tcc, mod_full_dir);
	tcc__add_include_path (tcc, combine_strs (mod_full_dir, "\\tcc\\include"));
	tcc__add_include_path (tcc, combine_strs (mod_full_dir, "\\tcc\\include\\winapi"));
	tcc__add_library_path (tcc, combine_strs (mod_full_dir, "\\tcc\\lib"));

	// Locate compatible, unmodded executable to work on
	enum bin_id bin_id;
	char const * bin_file_name = NULL;
	HANDLE bin_file; {
		bin_file_name = standard_exe_filename;
		bin_id = identify_binary (bin_file_name, &bin_file);
		if ((bin_id != BIN_ID_GOG) && (bin_id != BIN_ID_STEAM) && (bin_id != BIN_ID_PCG)) {
			if (bin_file != INVALID_HANDLE_VALUE)
				CloseHandle (bin_file);
			bin_file_name = backup_exe_filename;
			enum bin_id unmod_id = identify_binary (bin_file_name, &bin_file);
			if ((unmod_id != BIN_ID_GOG) && (unmod_id != BIN_ID_STEAM) && (unmod_id != BIN_ID_PCG)) {
				char * error_msg = format (
					"Couldn't find compatible, unmodded executable. This mod is only compatible with Civ3Conquests.exe V1.22 from GOG, Steam, or PCGames.de.\n"
					"%s: %s\n"
					"%s: %s\n"
					"Current directory: %s",
					standard_exe_filename, bin_id_strs[bin_id], backup_exe_filename, bin_id_strs[unmod_id], conquests_dir);
				THROW (error_msg);
			} else
				bin_id = unmod_id;
		}
	}

	// Set "bin" variable appropriately
	if      (bin_id == BIN_ID_GOG)   bin = &gog_binary;
	else if (bin_id == BIN_ID_STEAM) bin = &steam_binary;
	else if (bin_id == BIN_ID_PCG)   bin = &pcg_binary;
	printf ("Found %s executable in \"%s\"\n", bin->title, bin_file_name);

	// Load civ_prog_objects.csv
	struct civ_prog_object * civ_prog_objects;
	int count_civ_prog_objects; {
		char * const prog_objs_text = ep_file_to_string (mod_full_dir, "civ_prog_objects.csv");
		int count_lines = 0;
		for (char * c = prog_objs_text; *c != '\0'; c++)
			count_lines += (*c == '\n');
		civ_prog_objects = calloc (count_lines, sizeof *civ_prog_objects);
		count_civ_prog_objects = 0;
		char * cursor = prog_objs_text;
		skip_line (&cursor); // Skip the first line containing column labels
		while (1) {
			skip_horiz_space (&cursor);
			struct civ_prog_object obj;
			if (*cursor == '\0')
				break;
			else if (*cursor  == '\n')
				cursor++; // Skip empty line
			else if (parse_civ_prog_object (&cursor, bin->addr_column, &obj))
				civ_prog_objects[count_civ_prog_objects++] = obj;
			else {
				int count_newlines = 0;
				for (char * c = prog_objs_text; c < cursor; c++)
					count_newlines += (*c == '\n');
				THROW (format ("Line %d in civ_prog_objects.csv is invalid", 1 + count_newlines));
			}
		}
		free (prog_objs_text);
	}

	PROCESS_INFORMATION civ_proc_info = {0};

#ifdef C3X_RUN
	{
		STARTUPINFO civ_startup_info = {0};
		civ_startup_info.cb = sizeof (civ_startup_info);
		CloseHandle (bin_file);
		char * cmd_line = strdup (bin_file_name); // TODO: Pass command line args through
		CreateProcessA (
			NULL,
			cmd_line,         // lpCommandLine
			NULL,             // lpProcessAttributes
			NULL,             // lpThreadAttributes
			FALSE,            // bInheritHandles
			CREATE_SUSPENDED, // dwCreationFlags
			NULL,             // lpEnvironment
			NULL,             // lpCurrentDirectory
			&civ_startup_info,// lpStartupInfo
			&civ_proc_info    // lpProcessInformation
			);
		free (cmd_line);
		civ_proc = civ_proc_info.hProcess;
	}
#else
	{
		DWORD bin_file_size = GetFileSize (bin_file, NULL);
		int civ_exe_buf_size = 10 * bin_file_size;
		civ_exe_contents = malloc (civ_exe_buf_size);
		ASSERT (civ_exe_contents != NULL);
		DWORD size_read = 0;
		ReadFile (bin_file, civ_exe_contents, bin_file_size, &size_read, NULL);
		REQUIRE (size_read == bin_file_size, format ("Failed to read file %s", bin_file_name));
		civ_exe_buf_occupied_size = bin_file_size;
		civ_exe = civ_pe_from_file_contents (civ_exe_contents);
		CloseHandle (bin_file);
		bin_file = INVALID_HANDLE_VALUE;
	}
#endif

	// Make changes to the EXE if we'll be writing out a new one
#ifdef C3X_INSTALL
	// Disable digital signature
	// NOTE: I don't know if this really matters since the executable works anyway even with an invalid signature. I'm just guessing since we're
	// invalidating the signature it's best to stub it out so Windows doesn't look for it.
	memset (&civ_exe.opt->DataDirectory[DF_CERTIFICATE_TABLE], 0, sizeof civ_exe.opt->DataDirectory[DF_CERTIFICATE_TABLE]);

	// Set LAA bit
	civ_exe.coff->Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
#endif

	for (int n = 0; n < count_civ_prog_objects; n++) {
		struct civ_prog_object const * obj = &civ_prog_objects[n];
		if (obj->job == OJ_DEFINE) {
			char val[1000];
			snprintf (val, sizeof val, "((%s)%d)", obj->type, obj->addr);
			val[(sizeof val) - 1] = '\0';
			tcc__define_symbol (tcc, obj->name, val);
		}
	}
	
	// Get write access to rdata section so we can replace entries in the vtables. Only necessary if we're modifying a live process.
#ifdef C3X_RUN
	set_prog_mem_protection (bin->addr_rdata, bin->size_rdata, MAA_READ_WRITE);
#endif

	// Allocate space for inleads
	int inleads_capacity = 100,
	    inleads_size = inleads_capacity * sizeof (struct inlead);
	struct inlead * inleads = alloc_prog_memory (".c3xinl", NULL, inleads_size, MAA_READ_WRITE_EXECUTE);
	int i_next_free_inlead = 0;

	// Create injected state
	struct injected_state * injected_state = alloc_prog_memory (".c3xdat", NULL, sizeof (struct injected_state), MAA_READ_WRITE);
	write_prog_int (&injected_state->mod_version, MOD_VERSION);
	write_prog_memory (&injected_state->mod_rel_dir, mod_rel_dir, strlen (mod_rel_dir) + 1);
	write_prog_int (&injected_state->sc_img_state, IS_UNINITED);
	write_prog_int (&injected_state->tile_highlight_state, IS_UNINITED);
	write_prog_int (&injected_state->mod_info_button_images_state, IS_UNINITED);
	tcc_define_pointer (tcc, "ADDR_INJECTED_STATE", injected_state);

	// Pass through prog objects before compiling to set things up for compilation
	for (int n = 0; n < count_civ_prog_objects; n++) {
		struct civ_prog_object const * obj = &civ_prog_objects[n];

		// Initialize inlead
		if (obj->job == OJ_INLEAD) {
			ASSERT (i_next_free_inlead < inleads_capacity);
			struct inlead * inlead = &inleads[i_next_free_inlead++];
			int func_addr = obj->addr;
			ASSERT (func_addr != 0);
			init_inlead (inlead, func_addr);
			tcc__define_symbol (tcc, obj->name, temp_format ("((%s)%d)", obj->type, (int)inlead));

		// Define base func as vptr target
		} else if (obj->job == OJ_REPL_VPTR) {
			int impl_addr = read_prog_int ((void *)obj->addr);
			tcc__define_symbol (tcc, obj->name, temp_format ("((%s)%d)", obj->type, impl_addr));
		}
	}

	// Need two small regions of executable memory to write some machine code to enable intercepting the AI's production choices, the easiest way
	// to get these is to use some of the space reserved for inleads. The contents of these regions are only written after the injected code is
	// compiled b/c they depend on the address of intercept_consideration. The injected code also depends on the addresses (see
	// apply_machine_code_edits). The regions are filled out by init_consideration_airlocks, part of the patcher.
	byte * addr_improv_consideration_airlock, * addr_unit_consideration_airlock; {
		ASSERT (i_next_free_inlead + 3 < inleads_capacity);
		addr_improv_consideration_airlock = (byte *)&inleads[i_next_free_inlead];
		addr_unit_consideration_airlock   = (byte *)&inleads[i_next_free_inlead + 2];
		i_next_free_inlead += 4;
		tcc__define_symbol (tcc, "ADDR_IMPROV_CONSIDERATION_AIRLOCK", temp_format ("((void *)0x%x)", (int)addr_improv_consideration_airlock));
		tcc__define_symbol (tcc, "ADDR_UNIT_CONSIDERATION_AIRLOCK"  , temp_format ("((void *)0x%x)", (int)addr_unit_consideration_airlock));
	}

	// Same as above, this time for the set resource bit airlock. In this case we only need one.
	byte * addr_set_resource_bit_airlock; {
		ASSERT (i_next_free_inlead + 1 < inleads_capacity);
		addr_set_resource_bit_airlock = (byte *)&inleads[i_next_free_inlead];
		i_next_free_inlead += 2;
		tcc__define_symbol (tcc, "ADDR_SET_RESOURCE_BIT_AIRLOCK", temp_format ("((void *)0x%x)", (int)addr_set_resource_bit_airlock));
	}

	// Same again, this time for the bit of code that captures the TradeOffer object pointer when a gold trade on the table is modified
	byte * addr_capture_modified_gold_trade; {
		ASSERT (i_next_free_inlead < inleads_capacity);
		addr_capture_modified_gold_trade = (byte *)&inleads[i_next_free_inlead++];
		tcc__define_symbol (tcc, "ADDR_CAPTURE_MODIFIED_GOLD_TRADE", temp_format ("((void *)0x%x)", (int)addr_capture_modified_gold_trade));
	}

	// Compile C code to inject
	{
		char * source = ep_file_to_string (mod_full_dir, "injected_code.c");
		success = tcc__compile_string (tcc, source);
		ASSERT (success != -1);
		free (source);
	}

	// Allocate pages for C code injection. This is a set of pages that is located at the same location in this process'
	// memory as in the Civ process' memory so that we can use our memory as a relocation target for TCC then copy the
	// machine code into the Civ process and have it work.
	int inject_size; {
		int min_size = tcc__relocate (tcc, NULL);
		ASSERT (min_size >= 0);
		inject_size = align_pow2 (0x1000, min_size);
	}
	byte * civ_inject_mem, * our_inject_mem; {
#ifdef C3X_RUN
		civ_inject_mem = alloc_prog_memory (".c3xtxt", (void *)0x22220000, inject_size, MAA_READ_WRITE_EXECUTE);
		our_inject_mem = VirtualAlloc (civ_inject_mem, inject_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		ASSERT (our_inject_mem == civ_inject_mem);
#else
		civ_inject_mem = alloc_prog_memory (".c3xtxt", NULL, inject_size, MAA_READ_WRITE_EXECUTE);
		REQUIRE (((int)civ_inject_mem >= (int)low_addr_buf) && ((int)civ_inject_mem + inject_size < (int)low_addr_buf + sizeof low_addr_buf),
			 "Code inject area does not fit in low_addr_buf.");
		our_inject_mem = civ_inject_mem;
#endif
	}

	// Finish compilation then copy over the compiled code.
	success = tcc__relocate (tcc, our_inject_mem);
	ASSERT (success != -1);
	write_prog_memory (civ_inject_mem, our_inject_mem, inject_size);

	// List symbol locations for debugging
#if 0
	tcc__list_symbols (tcc, NULL, print_symbol_location);
#endif
	
	// Pass through prog objects after compiling to redirect control flow to patches
	for (int n = 0; n < count_civ_prog_objects; n++) {
		struct civ_prog_object const * obj = &civ_prog_objects[n];
		if (obj->job != OJ_IGNORE) {
			ASSERT (obj->addr != 0);

			if (obj->job == OJ_INLEAD)
				put_trampoline ((void *)obj->addr, find_patch_function (tcc, obj->name, 1), 0);
			else if (obj->job == OJ_REPL_VPTR)
				write_prog_int ((void *)obj->addr, (int)find_patch_function (tcc, obj->name, 1));
			else if (obj->job == OJ_REPL_CALL) {
				byte * instr = read_prog_memory ((void *)obj->addr, 10);
				int instr_size = length_disasm (instr);
				REQUIRE (instr_size >= 5, format ("Can't perform call replacement for %s. Instruction must be at least 5 bytes.", obj->name));
				put_trampoline ((void *)obj->addr, find_patch_function (tcc, obj->name, 1), 1);
				if (instr_size > 5) {
					byte nops[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
					write_prog_memory ((void *)(obj->addr + 5), nops, instr_size - 5);
				}
				free (instr);
			}
		}
	}

	init_consideration_airlocks (bin_id, tcc, addr_improv_consideration_airlock, addr_unit_consideration_airlock);
	{
		int addr_intercept_set_resource_bit = 0;
		for (int n = count_civ_prog_objects - 1; n >= 0; n--) {
			struct civ_prog_object const * obj = &civ_prog_objects[n];
			if (0 == strcmp (obj->name, "ADDR_INTERCEPT_SET_RESOURCE_BIT")) {
				addr_intercept_set_resource_bit = obj->addr;
				break;
			}
		}
		REQUIRE (addr_intercept_set_resource_bit != 0, "Couldn't find ADDR_INTERCEPT_SET_RESOURCE_BIT in prog objects");
		init_set_resource_bit_airlock (tcc, addr_set_resource_bit_airlock, addr_intercept_set_resource_bit);
	}

	// Give up write permission on Civ proc's code injection pages
	set_prog_mem_protection (civ_inject_mem, inject_size, MAA_READ_EXECUTE);

	// Give up write permission on inleads
	set_prog_mem_protection (inleads, inleads_size, MAA_READ_EXECUTE);

	// Give up write permission on rdata section
#ifdef C3X_RUN
	set_prog_mem_protection (bin->addr_rdata, bin->size_rdata, MAA_READ);
#endif

#ifdef C3X_RUN
	ResumeThread (civ_proc_info.hThread);

	WaitForSingleObject (civ_proc, INFINITE);
	CloseHandle (civ_proc_info.hProcess);
	CloseHandle (civ_proc_info.hThread);

#else
	// If about to overwrite the standard EXE, make a backup copy first
	if (bin_file_name == standard_exe_filename) {
		success = CopyFile (standard_exe_filename, backup_exe_filename, 0);
		REQUIRE (success, "Failed to create backup of unmodded EXE file");
	}

	buffer_to_file (civ_exe_contents, civ_exe_buf_occupied_size, ".", standard_exe_filename);
	MessageBox (NULL, format ("Mod installed successfully"), "Success", MB_ICONINFORMATION);
	free (civ_exe_contents);
#endif

	if (tcc)
		tcc__delete (tcc);
	ExitProcess (0);
}
