// Scenario scripts with declared program objects and patches.
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

void
free_script_prog_object (struct civ_prog_object * object)
{
	free ((void *)object->name);
	free ((void *)object->type);
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

	for (int n = 0; n < is->count_script_prog_objects; n++)
		free_script_prog_object (&is->script_prog_objects[n]);
	free (is->script_prog_objects);
	is->script_prog_objects = NULL;
	is->count_script_prog_objects = 0;
}

bool
script_is_function_type (char const * type)
{
	return strstr (type, "(__fastcall *)") ||
	       strstr (type, "(__cdecl *)") ||
	       strstr (type, "(__stdcall *)");
}

bool
script_can_detour (struct civ_prog_object const * obj)
{
	unsigned offset = (unsigned)obj->addr - (unsigned)is->binary.addr_text;
	return (offset < (unsigned)is->binary.size_text) &&
	       ((obj->job == OJ_INLEAD) || ((obj->job == OJ_DEFINE) && script_is_function_type (obj->type)));
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
prepare_script_inlead (struct civ_prog_object const * obj, byte ** entry_out, int * binding)
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

	*entry_out = entry;
	*binding = (int)inlead;
	return NULL;
}

char const *
install_script_detour (byte * entry, void const * replacement)
{
	DWORD old_protection, unused;
	if (! VirtualProtect (entry, 5, PAGE_EXECUTE_READWRITE, &old_protection))
		return "Could not make script detour writable.";

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

struct script_object {
	struct civ_prog_object object;
	bool added;
	byte * entry;
	void const * replacement;
};

bool
script_object_has_binding (struct civ_prog_object const * obj)
{
	return (obj->job == OJ_DEFINE) || (obj->job == OJ_INLEAD) || (obj->job == OJ_REPL_VPTR);
}

struct script_object_match {
	bool exact;
	bool address_conflict;
	bool name_conflict;
};

void
match_script_object (struct script_object_match * match, struct civ_prog_object const * obj, struct civ_prog_object const * existing)
{
	if (! script_object_has_binding (existing))
		return;

	bool same_name = 0 == strcmp (obj->name, existing->name);
	if (obj->addr == existing->addr) {
		if (same_name && (0 == strcmp (obj->type, existing->type)))
			match->exact = true;
		// Values below the game's code range are constants or offsets, not addresses.
		else if ((unsigned)obj->addr >= (unsigned)is->binary.addr_text)
			match->address_conflict = true;
		else if (same_name)
			match->name_conflict = true;
	} else if (same_name)
		match->name_conflict = true;
}

char const *
read_script_objects (char * csv, struct civ_prog_object * base, int count_base, struct script_object ** out, int * count_out)
{
	int capacity = 1;
	for (char * c = csv; *c; c++)
		capacity += *c == '\n';
	struct script_object * rows = calloc (capacity, sizeof *rows);
	if (! rows)
		return "Could not allocate script program objects.";
	*out = rows;

	char * cursor = csv;
	skip_line (&cursor); // Skip the same column header used by the core CSV.
	while (*cursor) {
		skip_horiz_space (&cursor);
		if (*cursor == '\n') {
			cursor++;
			continue;
		}
		if (! *cursor)
			break;

		struct civ_prog_object obj;
		if (! parse_civ_prog_object (&cursor, is->binary.addr_column, &obj))
			return "Invalid row in script civ_prog_objects.csv.";

		// Include the row in cleanup even if validation below fails.
		struct script_object * row = &rows[(*count_out)++];
		row->object = obj;

		if (obj.job == OJ_IGNORE) {
			free_script_prog_object (&obj);
			(*count_out)--;
			continue;
		}
		if (! script_object_has_binding (&obj))
			return "Unsupported job in script civ_prog_objects.csv.";

		struct script_object_match local = {0};
		for (int n = 0; n < *count_out - 1; n++)
			match_script_object (&local, &obj, &rows[n].object);
		if (local.exact)
			return "Duplicate declaration in script civ_prog_objects.csv.";

		struct script_object_match match = {0};
		for (int n = 0; n < count_base; n++)
			match_script_object (&match, &obj, &base[n]);
		for (int n = 0; n < is->count_script_prog_objects; n++)
			match_script_object (&match, &obj, &is->script_prog_objects[n]);

		if (local.name_conflict || match.name_conflict)
			return "Script program object name conflicts with an existing definition.";
		if (local.address_conflict || (! match.exact && match.address_conflict)) {
			char warning[300];
			snprintf (warning, sizeof warning, "Script object %s at 0x%x does not match the existing name and type. Keeping the existing definition.",
			          obj.name, obj.addr);
			pop_up_in_game_warning (warning);
			free_script_prog_object (&obj);
			(*count_out)--;
			continue;
		}

		// An exact define redeclaration adds nothing. Keep the existing object's
		// binding rules (in particular, a vtable slot binds to its current target).
		if (match.exact && (obj.job == OJ_DEFINE)) {
			free_script_prog_object (&obj);
			(*count_out)--;
			continue;
		}

		if ((obj.job != OJ_DEFINE) && ! script_is_function_type (obj.type))
			return "Script patch declarations must have a function pointer type.";

		row->added = ! match.exact;
	}
	return NULL;
}

char const *
bind_script_object (struct script_compiler * compiler, TCCState * tcc, struct civ_prog_object const * obj, struct script_object * row)
{
	int address = obj->addr;
	if (row && (obj->job == OJ_INLEAD)) {
		if (! script_can_detour (obj))
			return "Script inlead target is outside .text.";
		char const * error = prepare_script_inlead (obj, &row->entry, &address);
		if (error)
			return error;
	} else if (obj->job == OJ_REPL_VPTR) {
		unsigned offset = (unsigned)obj->addr - (unsigned)is->binary.addr_rdata;
		if (row && (offset > (unsigned)(is->binary.size_rdata - sizeof (void *))))
			return "Script vtable replacement is outside .rdata.";
		address = *(int *)obj->addr;
	} else if (script_can_detour (obj)) {
		address = (int)script_current_entry (obj);
		if (! address)
			return "Invalid script jump chain.";
	}

	char value[2000];
	snprintf (value, sizeof value, "((%s)0x%x)", obj->type, address);
	compiler->define_symbol (tcc, obj->name, value);
	return NULL;
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
	struct script_object * rows = NULL;
	int count_rows = 0;

	if (error)
		goto done;

	snprintf (path, sizeof path, "%s\\civ_prog_objects.csv", directory);
	char * csv = file_to_string (path);
	if (csv) {
		error = read_script_objects (csv, objects, count, &rows, &count_rows);
		free (csv);
		if (error)
			goto done;
	}

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

	// Bind inherited objects first, except where this script declares its own binding.
	for (int n = 0; n < count + is->count_script_prog_objects; n++) {
		struct civ_prog_object const * obj = (n < count) ? &objects[n] : &is->script_prog_objects[n - count];
		if (! script_object_has_binding (obj))
			continue;

		int i = 0;
		while ((i < count_rows) && strcmp (obj->name, rows[i].object.name))
			i++;
		if (i == count_rows) {
			error = bind_script_object (compiler, tcc, obj, NULL);
			if (error)
				goto done;
		}
	}

	for (int n = 0; n < count_rows; n++) {
		error = bind_script_object (compiler, tcc, &rows[n].object, &rows[n]);
		if (error)
			goto done;
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
		int i = 0;
		for (; i < count_rows; i++) {
			struct script_object * row = &rows[i];
			int len = strlen (row->object.name);
			if ((row->object.job != OJ_DEFINE) &&
			    ! strncmp (name, row->object.name, len) && ((name[len] == '\0') || (name[len] == '@'))) {
				if (row->replacement) {
					error = "Ambiguous script replacement symbol.";
					goto done;
				}
				row->replacement = symbols.items[n].address;
				break;
			}
		}
		if (i == count_rows) {
			snprintf (diagnostic, sizeof diagnostic, "Undeclared script patch: %s", symbols.items[n].name);
			error = diagnostic;
			goto done;
		}
	}

	int count_added = 0;
	for (int n = 0; n < count_rows; n++) {
		struct script_object * row = &rows[n];
		if ((row->object.job != OJ_DEFINE) && ! row->replacement) {
			snprintf (diagnostic, sizeof diagnostic, "Missing script patch: patch_%s", row->object.name);
			error = diagnostic;
			goto done;
		}
		count_added += row->added;
	}

	if (count_added) {
		struct civ_prog_object * active = realloc (is->script_prog_objects,
		                                          (is->count_script_prog_objects + count_added) * sizeof *active);
		if (! active) {
			error = "Could not expand the active script program objects.";
			goto done;
		}
		is->script_prog_objects = active;
	}

	for (int n = 0; n < count_rows; n++) {
		struct script_object * row = &rows[n];
		if (row->object.job == OJ_INLEAD) {
			error = install_script_detour (row->entry, row->replacement);
			if (error)
				goto done;
		} else if (row->object.job == OJ_REPL_VPTR) {
			void ** slot = (void **)row->object.addr;
			DWORD old_protect, unused;
			if (! VirtualProtect (slot, sizeof *slot, PAGE_READWRITE, &old_protect)) {
				error = "Could not make script vtable slot writable.";
				goto done;
			}

			*slot = (void *)row->replacement;
			if (! VirtualProtect (slot, sizeof *slot, old_protect, &unused)) {
				error = "Could not restore script vtable slot protection.";
				goto done;
			}
		}
	}

	// Publish only new definitions from a successfully installed script.
	for (int n = 0; n < count_rows; n++) {
		if (rows[n].added) {
			is->script_prog_objects[is->count_script_prog_objects++] = rows[n].object;
			rows[n].object.name = rows[n].object.type = NULL;
		}
	}

done:

	// Include the script, its inleads, and any detours installed before an error.
	if (relocated && ! FlushInstructionCache ((HANDLE)-1, NULL, 0))
		pop_up_in_game_warning ("Could not flush the instruction cache after loading the script. Continuing to load the scenario although scripts may not work properly.");

	if (tcc)
		compiler->destroy (tcc);

	free (symbols.items);
	for (int n = 0; n < count_rows; n++)
		free_script_prog_object (&rows[n].object);
	free (rows);

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
