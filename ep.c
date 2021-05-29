
#include "stdlib.h"
#include "stdio.h"

#include "tcc/libtcc/libtcc.h"
#include "lend/ld32.c"
#include "C3X.h"

#include "civ_prog_objects.h"

// BOOL WINAPI EnumProcessModulesEx(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded, DWORD dwFilterFlag);

#define ARRAY_LEN(a) ((sizeof a) / (sizeof a[0]))

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
	int file_size;
	void * addr_rdata;
	int    size_rdata;

	// Address of Windows functions in the relocation table
	void * addr_addr_OutputDebugStringA;
	void * addr_addr_GetAsyncKeyState;
	void * addr_addr_GetProcAddress;
	void * addr_addr_GetModuleHandleA;

	void * addr_addr_init_floating_point;
	
	void * addr_stealth_attack_target_count_check;
	void * addr_unit_count_check;
	void * addr_era_count_check;

	// Address to patch to fix the submarine bug. This is the address of the last parameter (respect_unit_visiblity) for
	// a call to Unit::is_visible_to_civ inside some kind of pathfinding function. addr_b is another address that does
	// the same thing and also fixes the bug, based on my limited testing. I didn't look into it much since patching the
	// first address alone seems to work fine. The other fields here are for other tests I was running that did not lead
	// to a fix.
	void * addr_sub_bug_patch;
	// void * addr_sub_bug_patch_b;
	// void * addr_vptr_move_along_path;
	// void * addr_Unit_is_visible_to_civ;
	// int    size_Unit_is_visible_to_civ;

	// Address to patch to fix the science age bug. Actually similar in nature to the sub bug, the function that measures
	// a city's research output accepts a flag that determines whether or not it takes science ages into account. It's
	// mistakenly not set by the code that gathers all research points to increment tech progress (but it is set elsewhere
	// in code for the interface). The patch simply sets this flag.
	void * addr_science_age_bug_patch;

	// The size of the pedia background texture is hard-coded into the EXE and in the base game it's one pixel too small.
	// This shows up in game as a one pixel wide pink line along the right edge of the civilopedia. This patch simply
	// increases the texture width by one.
	void * addr_pedia_texture_bug_patch;

	void * addr_autoraze_bypass;
	void * addr_vptr_Leader_would_raze_city;

	// void * addr_Main_Screen_Form_perform_action_on_tile;
	// int    size_Main_Screen_Form_perform_action_on_tile;

	// void * addr_Leader_update_preproduction;
	// int    size_Leader_update_preproduction;

	void * addr_Main_Screen_Form_handle_left_click_on_map_1;
	void * addr_vptr_Main_Screen_Form_handle_left_click_on_map_1;

	// void * addr_Main_GUI_set_up_unit_command_buttons;
} * bin;

struct c3c_binary gog_binary = {
	.file_size = 3417464,
	.addr_rdata = (void *)0x665000,
	.size_rdata = 0x1B000,
	
	.addr_addr_OutputDebugStringA = (void *)0x665188,
	.addr_addr_GetAsyncKeyState = (void *)0x665280,
	.addr_addr_GetProcAddress = (void *)0x665168,
	.addr_addr_GetModuleHandleA = (void *)0x6650E4,

	.addr_addr_init_floating_point = (void *)0x73A8FC,

	.addr_stealth_attack_target_count_check = (void *)0x5B6B01,
	.addr_unit_count_check = (void *)0x569503,
	.addr_era_count_check = (void *)0x594B35,

	.addr_sub_bug_patch = (void *)0x57F623,
	// .addr_sub_bug_patch_b = (void *)0x57FDB6,
	// .addr_vptr_move_along_path = 0x66DD3C,
	// .addr_Unit_is_visible_to_civ = 0x5BB650,
	// .size_Unit_is_visible_to_civ = 0x9DA - 0x650,

	.addr_science_age_bug_patch = (void *)0x5601EF,

	.addr_pedia_texture_bug_patch = (void *)0x4CD0B1,

	.addr_autoraze_bypass = (void *)0x5640AC,
	.addr_vptr_Leader_would_raze_city = (void *)0x66CB50,

	// .addr_Main_Screen_Form_perform_action_on_tile = (void *)0x4E6DB0,
	// .size_Main_Screen_Form_perform_action_on_tile = 0x7538 - 0x6DB0,

	// .addr_Leader_update_preproduction = (void *)0x5604B0,
	// .size_Leader_update_preproduction = 0x1216 - 0x04B0,

	.addr_Main_Screen_Form_handle_left_click_on_map_1 = (void *)0x4EB1D0,
	.addr_vptr_Main_Screen_Form_handle_left_click_on_map_1 = (void *)0x66B44C,

	// .addr_Main_GUI_set_up_unit_command_buttons = (void *)0x550BB0,
};

struct c3c_binary steam_binary = {
	.file_size = 3518464,
	.addr_rdata = (void *)0x682000,
	.size_rdata = 0x1B000,
	
	.addr_addr_OutputDebugStringA = (void *)0x68219C,
	.addr_addr_GetAsyncKeyState = (void *)0x6822E0,
	.addr_addr_GetProcAddress = (void *)0x682130,
	.addr_addr_GetModuleHandleA = (void *)0x6820B8,

	.addr_addr_init_floating_point = (void *)0x756C74,

	.addr_stealth_attack_target_count_check = (void *)0x5C5458,
	.addr_unit_count_check = (void *)0x575933,
	.addr_era_count_check = (void *)0x5A2357,

	.addr_sub_bug_patch = (void *)0x58C352,
	// .addr_sub_bug_patch_b = ???,
	// .addr_vptr_move_along_path = ???,
	// .addr_Unit_is_visible_to_civ = ???,
	// .size_Unit_is_visible_to_civ = ???,

	.addr_science_age_bug_patch = (void *)0x56C2E3,

	.addr_pedia_texture_bug_patch = (void *)0x4D5151,

	.addr_autoraze_bypass = (void *)0x570385,
	.addr_vptr_Leader_would_raze_city = (void *)0x689C3C,

	// .addr_Main_Screen_Form_perform_action_on_tile = (void *)0x4EF820,
	// .size_Main_Screen_Form_perform_action_on_tile = 0xFFCC - 0xF820,

	// .addr_Leader_update_preproduction = (void *)0x0,
	// .size_Leader_update_preproduction = 0,

	.addr_Main_Screen_Form_handle_left_click_on_map_1 = (void *)0x4F4020,
	.addr_vptr_Main_Screen_Form_handle_left_click_on_map_1 = (void *)0x68854C,

	// .addr_Main_GUI_set_up_unit_command_buttons = (void *)0x55BCE0,
};

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

HANDLE
open_file_for_reading (char const * path, char const * filename)
{
	char * fullpath = malloc (MAX_PATH + strlen (filename) + 2);
	sprintf (fullpath, "%s\\%s", path, filename);
	HANDLE file = CreateFile (fullpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	REQUIRE (file != INVALID_HANDLE_VALUE, format ("Failed to open file \"%s\"\n", fullpath));
	free (fullpath);
	return file;
}

char *
file_to_string (char const * path, char const * filename)
{
	HANDLE file = open_file_for_reading (path, filename);
	DWORD size = GetFileSize (file, NULL);
	char * tr = malloc (size + 1);
	DWORD size_read = 0;
	ReadFile (file, tr, size, &size_read, NULL);
	tr[size] = '\0';
	REQUIRE (size_read == size, format ("Failed to read file \"%s\"", filename));
	CloseHandle (file);
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





#define TARGET_PROCESS 0
#define TARGET_EXE_FILE 1
int target;
// if target == TARGET_PROCESS, use this:
HANDLE civ_proc;
// else target == TARGET_EXE_FILE, use these:
struct civ_pe civ_exe;
byte * civ_exe_contents; // Buffer contains the contents of the EXE file
int civ_exe_buf_size;
int civ_exe_buf_occupied_size;




byte *
get_exe_ptr (void const * virt_addr)
{
	int i_section;
	int offset = va_to_file_ptr (&civ_exe, virt_addr, &i_section);
	REQUIRE (i_section >= 0, format ("Virtual address %p does not correspond to any data in EXE file", virt_addr));
	return civ_exe_contents + offset;
}






byte *
read_prog_memory (void const * addr, int size)
{
	byte * tr = malloc (size);
	if (target == TARGET_PROCESS) {
		SIZE_T size_read;
		int success = ReadProcessMemory (civ_proc, addr, tr, size, &size_read);
		ASSERT (success && ((int)size_read == size));
	} else
		memcpy (tr, get_exe_ptr (addr), size);
	return tr;
}

int
read_prog_int (void const * addr)
{
	int tr;
	if (target == TARGET_PROCESS) {
		SIZE_T size_read;
		int success = ReadProcessMemory (civ_proc, addr, &tr, sizeof tr, &size_read);
		ASSERT (success && (size_read == sizeof tr));
	} else
		tr = int_from_bytes (get_exe_ptr (addr));
	return tr;
}

void
write_prog_memory (void * addr, byte const  * buf, int size)
{
	if (target == TARGET_PROCESS) {
		SIZE_T size_written;
		int success = WriteProcessMemory (civ_proc, addr, buf, size, &size_written);
		ASSERT (success && ((int)size_written == size));
	} else
		memcpy (get_exe_ptr (addr), buf, size);
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
	if (target == TARGET_PROCESS) {
		void * tr = VirtualAllocEx (civ_proc, location, size, MEM_RESERVE | MEM_COMMIT, page_prot_flags[access]);
		REQUIRE (tr != NULL, format ("Bad in-program alloc (location: %p, size: %d, access: %d)", location, size, access));
		return tr;
	} else {
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
	}
}

void
set_prog_mem_protection (void * addr, int size, enum mem_access access)
{
	if (target == TARGET_PROCESS) {
		DWORD unused;
		int success = VirtualProtectEx (civ_proc, addr, size, page_prot_flags[access], &unused);
		REQUIRE (success, format ("Failed to set mem access at %p to kind %d", addr, access));
	} else {
		size = align_pow2 (civ_exe.opt->FileAlignment, size);
		int i_section;
		va_to_file_ptr (&civ_exe, addr, &i_section);
		REQUIRE (i_section >= 0, format ("Virtual address %p does not correspond to any data in EXE file", addr));
		REQUIRE (size == civ_exe.sections[i_section].Misc.VirtualSize, "Size mismatch when modifying section characteristics");
		civ_exe.sections[i_section].Characteristics = section_flags[access];
		recompute_civ_pe_sizes (&civ_exe);
	}
}

void
put_trampoline (void * from, void * to)
{
	byte code[5];
	code[0] = 0xE9; // jmp
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
	BIN_ID_MODDED,
	BIN_ID_UNRECOGNIZED,

	COUNT_BIN_IDS
};

char const * bin_id_strs[COUNT_BIN_IDS] = {"Not found", "GOG Complete", "Steam Complete", "Modded", "Unrecognized"};

enum bin_id
identify_binary (char const * file_name, HANDLE * out_file_handle)
{
	HANDLE file = CreateFile (file_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*out_file_handle = file;
	if (file == INVALID_HANDLE_VALUE)
		return BIN_ID_NOT_FOUND;
	DWORD size = GetFileSize (file, NULL);
	DWORD size_read;
	byte header[5000];
	if (size == gog_binary.file_size)
		return BIN_ID_GOG;
	else if (size == steam_binary.file_size)
		return BIN_ID_STEAM;
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

enum job {
	JOB_RUN = 0,
	JOB_INSTALL
};

int
main (int argc, char ** argv)
{
	DWORD unused;
	int success;

	enum job job;
	REQUIRE (argc == 2, "Expected exactly one argument");
	if (strcmp (argv[1], "--run") == 0)
		job = JOB_RUN;
	else if (strcmp (argv[1], "--install") == 0)
		job = JOB_INSTALL;
	else
		THROW (format ("Unrecognized argument: \"%s\"", argv[1]));

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

	HMODULE libtcc; ;
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
	char const * bin_file_name;
	HANDLE bin_file; {
		bin_file_name = standard_exe_filename;
		bin_id = identify_binary (bin_file_name, &bin_file);
		if ((bin_id != BIN_ID_GOG) && (bin_id != BIN_ID_STEAM)) {
			if (bin_file != INVALID_HANDLE_VALUE)
				CloseHandle (bin_file);
			bin_file_name = backup_exe_filename;
			enum bin_id unmod_id = identify_binary (bin_file_name, &bin_file);
			if ((unmod_id != BIN_ID_GOG) && (unmod_id != BIN_ID_STEAM)) {
				char * error_msg = format (
					"Couldn't find compatible, unmodded executable. Compatible versions are GOG or Steam versions of Civ 3 Complete.\n"
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
	if (bin_id == BIN_ID_GOG) {
		bin = &gog_binary;
		printf ("Found GOG executable in \"%s\"\n", bin_file_name);
	} else if (bin_id == BIN_ID_STEAM) {
		bin = &steam_binary;
		printf ("Found Steam executable in \"%s\"\n", bin_file_name);
	} else
		THROW ("Did the impossible, and not in a good way.");

	PROCESS_INFORMATION civ_proc_info = {0};
	if (job == JOB_RUN) {
		target = TARGET_PROCESS;
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

	} else if (job == JOB_INSTALL) {
		target = TARGET_EXE_FILE;
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

	// Make changes to the EXE if we'll be writing out a new one
	if (job == JOB_INSTALL) {
		// Disable digital signature
		// NOTE: I don't know if this really matters since the executable works anyway even with an invalid signature. I'm
		// just guessing since we're invalidating the signature it's best to stub it out so Windows doesn't look for it.
		memset (&civ_exe.opt->DataDirectory[DF_CERTIFICATE_TABLE], 0, sizeof civ_exe.opt->DataDirectory[DF_CERTIFICATE_TABLE]);

		// Set LAA bit
		civ_exe.coff->Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
	}

	for (int n = 0; n < ARRAY_LEN (civ_prog_objects); n++) {
		struct civ_prog_object const * obj = &civ_prog_objects[n];
		if (obj->job == OJ_DEFINE) {
			char val[1000];
			snprintf (val, sizeof val, "((%s)%d)", obj->type, (bin == &gog_binary) ? obj->gog_addr : obj->steam_addr);
			val[(sizeof val) - 1] = '\0';
			tcc__define_symbol (tcc, obj->name, val);
		}
	}
	
	tcc_define_pointer (tcc, "ADDR_ADDR_OUTPUTDEBUGSTRINGA", bin->addr_addr_OutputDebugStringA);
	tcc_define_pointer (tcc, "ADDR_ADDR_GETASYNCKEYSTATE"  , bin->addr_addr_GetAsyncKeyState);
	tcc_define_pointer (tcc, "ADDR_ADDR_GETPROCADDRESS"    , bin->addr_addr_GetProcAddress);
	tcc_define_pointer (tcc, "ADDR_ADDR_GETMODULEHANDLEA"  , bin->addr_addr_GetModuleHandleA);

	void * addr_init_floating_point = (void *)read_prog_int (bin->addr_addr_init_floating_point);
	tcc_define_pointer (tcc, "ADDR_INIT_FLOATING_POINT", addr_init_floating_point);
	void * addr_Leader_impl_would_raze_city = (void *)read_prog_int (bin->addr_vptr_Leader_would_raze_city);
	tcc_define_pointer (tcc, "ADDR_LEADER_IMPL_WOULD_RAZE_CITY", addr_Leader_impl_would_raze_city);

	tcc_define_pointer (tcc, "ADDR_STEALTH_ATTACK_TARGET_COUNT_CHECK", bin->addr_stealth_attack_target_count_check);
	tcc_define_pointer (tcc, "ADDR_UNIT_COUNT_CHECK"                 , bin->addr_unit_count_check);
	tcc_define_pointer (tcc, "ADDR_ERA_COUNT_CHECK "                 , bin->addr_era_count_check);
	tcc_define_pointer (tcc, "ADDR_SUB_BUG_PATCH"                    , bin->addr_sub_bug_patch);
	tcc_define_pointer (tcc, "ADDR_SCIENCE_AGE_BUG_PATCH"            , bin->addr_science_age_bug_patch);
	tcc_define_pointer (tcc, "ADDR_PEDIA_TEXTURE_BUG_PATCH"          , bin->addr_pedia_texture_bug_patch);
	tcc_define_pointer (tcc, "ADDR_AUTORAZE_BYPASS"                  , bin->addr_autoraze_bypass);

	tcc_define_pointer (tcc, "ADDR_MAIN_SCREEN_FORM_HANDLE_LEFT_CLICK_ON_MAP_1", bin->addr_Main_Screen_Form_handle_left_click_on_map_1);

	// Get write access to rdata section so we can replace entries in the vtables. Only necessary if we're modifying a live process.
	if (target == TARGET_PROCESS)
		set_prog_mem_protection (bin->addr_rdata, bin->size_rdata, MAA_READ_WRITE);

	// Allocate space for inleads
	int inleads_size = 100 * sizeof (struct inlead);
	struct inlead * inleads = alloc_prog_memory (".c3xinl", NULL, inleads_size, MAA_READ_WRITE_EXECUTE);
	struct inlead * next_free_inlead = inleads;

	// Create injected state
	struct injected_state * injected_state = alloc_prog_memory (".c3xdat", NULL, sizeof (struct injected_state), MAA_READ_WRITE);
	write_prog_int (&injected_state->mod_version, MOD_VERSION);
	write_prog_memory (&injected_state->mod_rel_dir, mod_rel_dir, strlen (mod_rel_dir) + 1);
	// write_prog_int (&injected_state->sb_state, SB_UNINITED);
	write_prog_int (&injected_state->sc_img_state, SC_IMG_UNINITED);
	struct c3x_config base_config = {
		.enable_stack_bombard = 1,
		.enable_disorder_warning = 1,
		.allow_stealth_attack_against_single_unit = 1,
		.show_detailed_city_production_info = 1,
		.limit_railroad_movement = 0,
		.enable_free_buildings_from_small_wonders = 1,
		.enable_stack_worker_commands = 1,
		.skip_repeated_tile_improv_replacement_asks = 1,

		.use_offensive_artillery_ai = 1,
		.ai_build_artillery_ratio = 20,
		.ai_artillery_value_damage_percent = 50,
		.ai_build_bomber_ratio = 70,
		.replace_leader_unit_ai = 1,
		.fix_ai_army_composition = 1,

		.remove_unit_limit = 1,
		.remove_era_limit = 0,

		.patch_submarine_bug = 1,
		.patch_science_age_bug = 1,
		.patch_pedia_texture_bug = 1,
		.patch_disembark_immobile_bug = 1,
		.patch_houseboat_bug = 1,

		.prevent_autorazing = 0,
		.prevent_razing_by_ai_players = 0,
	};
	write_prog_memory (&injected_state->base_config, (byte const *)&base_config, sizeof base_config);
	tcc_define_pointer (tcc, "ADDR_INJECTED_STATE", injected_state);
	
	// Allocate pages for C code injection. This is a set of pages that is located at the same location in this process'
	// memory as in the Civ process' memory so that we can use our memory as a relocation target for TCC then copy the
	// machine code into the Civ process and have it work.
	int inject_size = 0xA000;
	byte * civ_inject_mem, * our_inject_mem; {
		if (target == TARGET_PROCESS) {
			civ_inject_mem = alloc_prog_memory (".c3xtxt", (void *)0x22220000, inject_size, MAA_READ_WRITE_EXECUTE);
			our_inject_mem = VirtualAlloc (civ_inject_mem, inject_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			ASSERT (our_inject_mem == civ_inject_mem);
		} else if (target == TARGET_EXE_FILE) {
			civ_inject_mem = alloc_prog_memory (".c3xtxt", NULL, inject_size, MAA_READ_WRITE_EXECUTE);
			REQUIRE (((int)civ_inject_mem >= (int)low_addr_buf) && ((int)civ_inject_mem + inject_size < (int)low_addr_buf + sizeof low_addr_buf),
				 "Code inject area does not fit in low_addr_buf.");
			our_inject_mem = civ_inject_mem;
		}
	}
	
	// Pass through prog objects before compiling to set things up for compilation
	for (int n = 0; n < ARRAY_LEN (civ_prog_objects); n++) {
		struct civ_prog_object const * obj = &civ_prog_objects[n];

		// Initialize inlead
		if (obj->job == OJ_INLEAD) {
			struct inlead * inlead = next_free_inlead++;
			int func_addr = (bin == &gog_binary) ? obj->gog_addr : obj->steam_addr;
			ASSERT (func_addr != 0);
			init_inlead (inlead, func_addr);
			tcc__define_symbol (tcc, obj->name, temp_format ("((%s)%d)", obj->type, (int)inlead));

		// Define base func as vptr target
		} else if (obj->job == OJ_REPL_VPTR) {
			int impl_addr = read_prog_int ((void *)((bin == &gog_binary) ? obj->gog_addr : obj->steam_addr));
			tcc__define_symbol (tcc, obj->name, temp_format ("((%s)%d)", obj->type, impl_addr));
		}
	}

	// Compile C code to inject then copy it into the Civ process
	{
		char * source = file_to_string (mod_full_dir, "injected_code.c");
		success = tcc__compile_string (tcc, source);
		ASSERT (success != -1);
		success = tcc__relocate (tcc, our_inject_mem);
		ASSERT (success != -1);
		write_prog_memory (civ_inject_mem, our_inject_mem, inject_size);
		free (source);
	}

	// List symbol locations for debugging
#if 0
	tcc__list_symbols (tcc, NULL, print_symbol_location);
#endif
	
	// Pass through prog objects after compiling to redirect control flow to patches
	for (int n = 0; n < ARRAY_LEN (civ_prog_objects); n++) {
		struct civ_prog_object const * obj = &civ_prog_objects[n];
		int func_addr = (bin == &gog_binary) ? obj->gog_addr : obj->steam_addr;
		ASSERT (func_addr != 0);
		char * sym = temp_format ("patch_%s", obj->name);

		// Write trampoline to redirect calls to original function to patched version
		if (obj->job == OJ_INLEAD) {
			put_trampoline ((void *)func_addr, tcc__get_symbol (tcc, sym));

		// Replace vptr
		} else if (obj->job == OJ_REPL_VPTR) {
			write_prog_int ((void *)func_addr, (int)tcc__get_symbol (tcc, sym));
		}
	}

	// Replace v-pointers with patched functions (init_floating_point isn't technically a vptr, it's a global function pointer but the same technique applies)
	write_prog_int (bin->addr_addr_init_floating_point, (int)tcc__get_symbol (tcc, "patch_init_floating_point"));
	write_prog_int (bin->addr_vptr_Main_Screen_Form_handle_left_click_on_map_1, (int)tcc__get_symbol (tcc, "patch_Main_Screen_Form_handle_left_click_on_map_1"));
	write_prog_int (bin->addr_vptr_Leader_would_raze_city, (int)tcc__get_symbol (tcc, "patch_Leader_impl_would_raze_city"));

	// Give up write permission on Civ proc's code injection pages
	set_prog_mem_protection (civ_inject_mem, inject_size, MAA_READ_EXECUTE);

	// Give up write permission on inleads
	set_prog_mem_protection (inleads, inleads_size, MAA_READ_EXECUTE);

	// Give up write permission on rdata section
	if (target == TARGET_PROCESS)
		set_prog_mem_protection (bin->addr_rdata, bin->size_rdata, MAA_READ);

	if (job == JOB_RUN) {
		ResumeThread (civ_proc_info.hThread);

		WaitForSingleObject (civ_proc, INFINITE);
		CloseHandle (civ_proc_info.hProcess);
		CloseHandle (civ_proc_info.hThread);

	} else if (job == JOB_INSTALL) {
		// If about to overwrite the standard EXE, make a backup copy first
		if (bin_file_name == standard_exe_filename) {
			success = CopyFile (standard_exe_filename, backup_exe_filename, 0);
			REQUIRE (success, "Failed to create backup of unmodded EXE file");
		}
		buffer_to_file (civ_exe_contents, civ_exe_buf_occupied_size, ".", standard_exe_filename);
		MessageBox (NULL, format ("Mod installed successfully"), "Success", MB_ICONINFORMATION);
		free (civ_exe_contents);
	}

	if (tcc)
		tcc__delete (tcc);
	ExitProcess (0);
}
