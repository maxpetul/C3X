// First implementation: process-lifetime scripts with optional vtable replacements.
// Included by injected_code.c after the parsing helpers.
#include "../tcc/libtcc/libtcc.h"
#include "../lend/ld32.c"

// Script code and inleads use VirtualAlloc; only the list nodes use malloc.
void *
alloc_script_memory (int size, bool compiled_code)
{
	struct script_allocation * allocation = malloc (sizeof *allocation);
	if (! allocation)
		return NULL;

	allocation->address = VirtualAlloc (NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (! allocation->address) {
		free (allocation);
		return NULL;
	}

	allocation->code_size = compiled_code ? size : 0;
	allocation->next = is->script_allocations;
	is->script_allocations = allocation;
	return allocation->address;
}

// Call only after restoring the code and pointers that can refer to these allocations.
void
free_script_memory (void)
{
	while (is->script_allocations) {
		struct script_allocation * allocation = is->script_allocations;
		is->script_allocations = allocation->next;
		VirtualFree (allocation->address, 0, MEM_RELEASE);
		free (allocation);
	}
}

bool
script_can_detour (struct civ_prog_object const * obj)
{
	unsigned offset = (unsigned)obj->addr - (unsigned)is->binary.addr_text;
	return (offset < (unsigned)is->binary.size_text) &&
	       ((obj->job == OJ_INLEAD) || ((obj->job == OJ_DEFINE) && strstr (obj->type, "*)")));
}

void *
script_code_region (byte * address)
{
	if ((unsigned)address - (unsigned)is->core_code < (unsigned)is->size_core_code)
		return is->core_code;

	for (struct script_allocation * allocation = is->script_allocations; allocation; allocation = allocation->next)
		if ((unsigned)address - (unsigned)allocation->address < (unsigned)allocation->code_size)
			return allocation->address;
	return NULL;
}

byte *
script_current_entry (struct civ_prog_object const * obj)
{
	// Loader-installed jumps only go forward: at most one into core code, then one per script.
	// A script that writes its own jump back to an earlier entry could create a cycle.
	// Bound the walk so loading reports an invalid chain instead of hanging.
	int jumps_left = 1;
	for (struct script_allocation * allocation = is->script_allocations; allocation; allocation = allocation->next)
		jumps_left += allocation->code_size > 0;

	byte * entry = (byte *)obj->addr;
	while (*entry == 0xE9) {
		byte * target = entry + 5 + int_from_bytes (entry + 1);
		void * target_region = script_code_region (target);
		// Preserve game thunks and jumps within the same compiled module.
		if (! target_region || (target_region == script_code_region (entry)))
			break;
		if (jumps_left-- == 0)
			return NULL;
		entry = target;
	}

	return entry;
}

// Copy complete instructions over the entry jump. Reject unfamiliar prologues
// rather than copying a branch or a short function into the inlead incorrectly.
int
script_inlead_prefix_size (byte * entry)
{
	int size = 0;
	while (size < 5) {
		byte * instr = entry + size;
		byte op = *instr;
		if (op == 0xE8) {
			if (int_from_bytes (instr + 1) == 0)
				return 0; // A call to the next instruction can expose the original PC.
			size += 5;
		} else if ((op == 0xE9) && (size == 0))
			return 5;
		else if (((op >= 0x50) && (op <= 0x5F)) || ((op >= 0xB8) && (op <= 0xBF)) ||
		         (op == 0x68) || (op == 0x6A) || (op == 0x81) || (op == 0x83) ||
		         (op == 0x85) || (op == 0x89) || (op == 0x8B) || (op == 0x8D) ||
		         (op == 0x90) || (op == 0x31) || (op == 0x33))
			size += length_disasm (instr);
		else
			return 0;
	}
	return size;
}

char const *
install_script_detour (struct civ_prog_object const * obj, void const * replacement, void ** below)
{
	// Reload frees script code from inside this hook, so it must not have a script caller.
	if (0 == strcmp (obj->name, "load_scenario"))
		return "Scripts cannot replace the scenario loader.";

	byte * entry = script_current_entry (obj);
	if (! entry)
		return "Invalid script jump chain.";

	int size = script_inlead_prefix_size (entry);
	if (! size)
		return "Unsupported function prologue for script inlead.";

	byte * inlead = alloc_script_memory (size + 5, false);
	if (! inlead)
		return "Could not allocate script inlead.";

	memcpy (inlead, entry, size);
	for (int offset = 0; offset < size;) {
		byte * instr = entry + offset;
		if ((*instr == 0xE8) || (*instr == 0xE9))
			emit_branch ((*instr == 0xE8) ? BK_CALL : BK_JUMP, inlead + offset,
			             instr + 5 + int_from_bytes (instr + 1));
		offset += length_disasm (instr);
	}
	emit_branch (BK_JUMP, inlead + size, entry + size);

	DWORD old_protection, unused;
	if (! VirtualProtect (entry, 5, PAGE_EXECUTE_READWRITE, &old_protection))
		return "Could not make script detour writable.";

	*below = inlead;
	emit_branch (BK_JUMP, entry, replacement);
	if (! VirtualProtect (entry, 5, old_protection, &unused))
		return "Could not restore script detour protection.";
	return NULL;
}

struct script_compiler {
	TCCState * (*create) (void);
	void (*destroy) (TCCState *);
	void (*set_lib_path) (TCCState *, char const *);
	void (*set_error_func) (TCCState *, void *, TCCErrorFunc);
	int (*add_include_path) (TCCState *, char const *);
	int (*add_library_path) (TCCState *, char const *);
	void (*define_symbol) (TCCState *, char const *, char const *);
	int (*set_output_type) (TCCState *, int);
	int (*add_file) (TCCState *, char const *);
	int (*relocate) (TCCState *, void *);
	void (*list_symbols) (TCCState *, void *, void (*) (void *, char const *, void const *));
};

void
script_compile_error (void * context, char const * message)
{
	snprintf (context, 1000, "%s", message);
}

struct script_symbol {
	char const * name;
	void const * address;
};

struct script_symbols {
	struct script_symbol * items;
	int count;
	int capacity;
	bool allocation_failed;
};

void
collect_script_symbol (void * context, char const * name, void const * address)
{
	struct script_symbols * symbols = context;
	if (symbols->allocation_failed)
		return;

	if (symbols->count == symbols->capacity) {
		int capacity = symbols->capacity ? 2 * symbols->capacity : 32;
		struct script_symbol * items = realloc (symbols->items, capacity * sizeof *items);
		if (! items) {
			symbols->allocation_failed = true;
			return;
		}

		symbols->items = items;
		symbols->capacity = capacity;
	}

	symbols->items[symbols->count++] = (struct script_symbol) {name, address};
}

// Read the starter script's INI using the same key/value and boolean readers as the mod config.
// Disabled scripts are ignored; dependencies are rejected until ordering is implemented.
bool
script_is_enabled (char * metadata, char const ** error)
{
	bool enabled = false;
	bool has_dependencies = false;
	char * cursor = metadata;

	while (*cursor) {
		char * line_start = cursor;
		skip_line (&cursor);

		char * line_end = cursor;
		if ((line_end > line_start) && (line_end[-1] == '\n'))
			line_end--;

		struct string_slice line = {line_start, line_end - line_start};
		line = trim_string_slice (&line, 0);
		if (line_is_empty_or_comment (&line))
			continue;

		struct string_slice key, value;
		if (parse_trimmed_key_value (&line, &key, &value) != KVP_SUCCESS)
			continue;

		if (slice_matches_str (&key, "enable")) {
			int int_value;
			if (read_int (&value, &int_value))
				enabled = int_value != 0;
			else
				*error = "Expected a boolean value for enable in script.ini.";

		} else if (slice_matches_str (&key, "dependencies")) {
			value = trim_string_slice (&value, 1);
			has_dependencies = value.len > 0;
		}
	}

	if (enabled && has_dependencies)
		*error = "Script dependencies are not supported yet.";

	return enabled;
}

void
load_one_script (struct script_compiler * compiler, char const * directory, struct civ_prog_object * objects, int count)
{
	char path[MAX_PATH];
	char value[2000];
	char diagnostic[1000] = "Script compilation or relocation failed.";
	char const * error = NULL;

	snprintf (path, sizeof path, "%s\\script.ini", directory);
	char * metadata = file_to_string (path);
	if (! metadata)
		return; // A directory without script.ini is not a script.

	bool enabled = script_is_enabled (metadata, &error);
	free (metadata);

	if (! enabled && ! error)
		return;

	TCCState * tcc = NULL;
	void * code = NULL;
	bool relocated = false;
	struct script_symbols symbols = {0};

	if (error)
		goto done;

	tcc = compiler->create ();
	if (! tcc) {
		error = "Could not initialize TCC.";
		goto done;
	}

	compiler->set_error_func (tcc, diagnostic, script_compile_error);
	compiler->set_output_type (tcc, TCC_OUTPUT_MEMORY);

	snprintf (path, sizeof path, "%s\\tcc", is->mod_rel_dir);
	compiler->set_lib_path (tcc, path);

	snprintf (path, sizeof path, "%s\\tcc\\include", is->mod_rel_dir);
	compiler->add_include_path (tcc, path);

	snprintf (path, sizeof path, "%s\\tcc\\include\\winapi", is->mod_rel_dir);
	compiler->add_include_path (tcc, path);

	snprintf (path, sizeof path, "%s\\tcc\\lib", is->mod_rel_dir);
	compiler->add_library_path (tcc, path);

	snprintf (path, sizeof path, "%s\\source", is->mod_rel_dir);
	compiler->add_include_path (tcc, path);

	snprintf (value, sizeof value, "((char *)0x%x)", (int)is->mod_script_path);
	compiler->define_symbol (tcc, "MOD_SCRIPT_PATH", value);

	// An inlead's call-through address is only known after finding the script's
	// patch symbol. Keep the binding in a slot that can be set after relocation.
	void ** below = alloc_script_memory (count * sizeof *below, false);
	if (! below) {
		error = "Could not allocate script function bindings.";
		goto done;
	}

	for (int n = 0; n < count; n++) {
		struct civ_prog_object * obj = &objects[n];
		if ((obj->job != OJ_DEFINE) && (obj->job != OJ_INLEAD) && (obj->job != OJ_REPL_VPTR))
			continue;

		// Bind the plain name before installing this script's replacements, so calls chain through earlier patches.
		if (script_can_detour (obj)) {
			below[n] = script_current_entry (obj);
			if (! below[n]) {
				error = "Invalid script jump chain.";
				goto done;
			}
			snprintf (value, sizeof value, "((%s)*(void **)0x%x)", obj->type, (int)&below[n]);
		} else {
			int address = (obj->job == OJ_REPL_VPTR) ? *(int *)obj->addr : obj->addr;
			snprintf (value, sizeof value, "((%s)0x%x)", obj->type, address);
		}
		compiler->define_symbol (tcc, obj->name, value);
	}

	// Compile and relocate before installing any replacements.
	snprintf (path, sizeof path, "%s\\source.c", directory);
	if (compiler->add_file (tcc, path) < 0) {
		error = diagnostic;
		goto done;
	}

	int size = compiler->relocate (tcc, NULL);
	if (size <= 0) {
		error = diagnostic;
		goto done;
	}

	code = alloc_script_memory (size, true);
	if (! code) {
		error = "Could not allocate script memory.";
		goto done;
	}

	if (compiler->relocate (tcc, code) < 0) {
		error = diagnostic;
		goto done;
	}

	relocated = true;

	compiler->list_symbols (tcc, &symbols, collect_script_symbol);
	if (symbols.allocation_failed) {
		error = "Could not allocate script symbol list.";
		goto done;
	}

	for (int n = 0; n < symbols.count; n++) {
		char const * name = symbols.items[n].name;
		if ((*name == '_') || (*name == '@'))
			name++;
		if (strncmp (name, "patch_", 6))
			continue;

		name += 6;
		bool found = false;
		for (int i = 0; i < count; i++) {
			struct civ_prog_object const * obj = &objects[i];
			int len = strlen (obj->name);
			if (strncmp (name, obj->name, len) || ((name[len] != '\0') && (name[len] != '@')))
				continue;

			found = true;
			if (script_can_detour (obj)) {
				error = install_script_detour (obj, symbols.items[n].address, &below[i]);
				if (error)
					goto done;
				continue;
			}

			if (obj->job != OJ_REPL_VPTR) {
				error = "Script patch target is not a function or vtable slot.";
				goto done;
			}

			unsigned offset = (unsigned)obj->addr - (unsigned)is->binary.addr_rdata;
			if (offset > (unsigned)(is->binary.size_rdata - sizeof (void *))) {
				error = "Script vtable replacement is outside .rdata.";
				goto done;
			}

			void ** slot = (void **)obj->addr;
			DWORD old_protect, unused;
			if (! VirtualProtect (slot, sizeof *slot, PAGE_READWRITE, &old_protect)) {
				error = "Could not make script vtable slot writable.";
				goto done;
			}

			*slot = (void *)symbols.items[n].address;
			if (! VirtualProtect (slot, sizeof *slot, old_protect, &unused)) {
				error = "Could not restore script vtable slot protection.";
				goto done;
			}
		}

		if (! found) {
			error = "Unknown patch_ target in script.";
			goto done;
		}
	}

done:

	// Include the script, its inleads, and any detours installed before an error.
	if (relocated && ! FlushInstructionCache ((HANDLE)-1, NULL, 0))
		pop_up_in_game_warning ("Could not flush the instruction cache after loading the script. Continuing to load the scenario although scripts may not work properly.");

	if (tcc)
		compiler->destroy (tcc);

	free (symbols.items);

	if (error) {
		snprintf (value, sizeof value, "%s:\n%s", directory, error);
		pop_up_in_game_error (value);
	}

	// Keep allocations until snapshot restoration, including any partially installed script.
}

void
load_scripts (struct civ_prog_object * objects, int count)
{
	char path[MAX_PATH];
	snprintf (path, sizeof path, "%s\\tcc\\libtcc.dll", is->mod_rel_dir);
	HMODULE library = LoadLibraryA (path);
	struct script_compiler compiler;
	char const * error = NULL;

	if (! library) {
		error = "Could not load tcc/libtcc.dll for scripts.";
		goto done;
	}

	struct {
		char const * name;
		void * destination;
	} api[] = {
		{"tcc_new"                , &compiler.create},
		{"tcc_delete"             , &compiler.destroy},
		{"tcc_set_lib_path"       , &compiler.set_lib_path},
		{"tcc_set_error_func"     , &compiler.set_error_func},
		{"tcc_add_include_path"   , &compiler.add_include_path},
		{"tcc_add_library_path"   , &compiler.add_library_path},
		{"tcc_define_symbol"      , &compiler.define_symbol},
		{"tcc_set_output_type"    , &compiler.set_output_type},
		{"tcc_add_file"           , &compiler.add_file},
		{"tcc_relocate"           , &compiler.relocate},
		{"tcc_list_symbols"       , &compiler.list_symbols},
	};

	for (int n = 0; n < ARRAY_LEN (api); n++) {
		FARPROC address = (*p_GetProcAddress) (library, api[n].name);
		if (! address) {
			snprintf (path, sizeof path, "Missing TCC function: %s", api[n].name);
			error = path;
			goto done;
		}

		memcpy (api[n].destination, &address, sizeof address);
	}

	// Start with the one example script; discovery and dependency ordering can follow later.
	snprintf (path, sizeof path, "%s\\Scripts\\Disorder Warning", is->mod_rel_dir);
	load_one_script (&compiler, path, objects, count);

done:

	if (library)
		FreeLibrary (library);

	if (error)
		pop_up_in_game_error ((char *)error);
}
