#include "stdlib.h"
#include "stdio.h"

#include "C3X.h"

void (WINAPI ** p_OutputDebugStringA) (char * lpOutputString) = ADDR_ADDR_OUTPUTDEBUGSTRINGA;
short (WINAPI ** p_GetAsyncKeyState) (int vKey) = ADDR_ADDR_GETASYNCKEYSTATE;
FARPROC (WINAPI ** p_GetProcAddress) (HMODULE hModule, char const * lpProcName) = ADDR_ADDR_GETPROCADDRESS;
HMODULE (WINAPI ** p_GetModuleHandleA) (char const * lpModuleName) = ADDR_ADDR_GETMODULEHANDLEA;
int (WINAPI ** p_MessageBoxA) (HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) = ADDR_ADDR_MESSAGEBOXA;

struct injected_state * is = ADDR_INJECTED_STATE;

// To be used as placeholder second argument so that we can imitate thiscall convention with fastcall
#define __ 0

// Many Windows and C standard library functions have to be loaded dynamically with GetProcAddress and the addresses are stored in the injected
// state. This is covered up with macros, which is not something I would normally do, but in this case it's very useful because it means the code in
// common.c can be shared by the patcher and the injected code.
#define VirtualProtect is->VirtualProtect
#define CloseHandle is->CloseHandle
#define CreateFileA is->CreateFileA
#define GetFileSize is->GetFileSize
#define ReadFile is->ReadFile
#define LoadLibraryA is->LoadLibraryA
#define FreeLibrary is->FreeLibrary
#define MessageBoxA is->MessageBoxA
#define MultiByteToWideChar is->MultiByteToWideChar
#define WideCharToMultiByte is->WideCharToMultiByte
#define GetLastError is->GetLastError
#define QueryPerformanceCounter is->QueryPerformanceCounter
#define QueryPerformanceFrequency is->QueryPerformanceFrequency
#define GetLocalTime is->GetLocalTime
#define TransparentBlt is->TransparentBlt
#define snprintf is->snprintf
#define malloc is->malloc
#define calloc is->calloc
#define realloc is->realloc
#define free is->free
#define strtol is->strtol
#define strncmp is->strncmp
#define strcmp is->strcmp
#define strlen is->strlen
#define strncpy is->strncpy
#define strcpy is->strcpy
#define strdup is->strdup
#define strstr is->strstr
#define qsort is->qsort
#define memcmp is->memcmp
#define memcpy is->memcpy

#include "common.c"

#define TRADE_SCROLL_BUTTON_ID_LEFT  0x222001
#define TRADE_SCROLL_BUTTON_ID_RIGHT 0x222002

// Must match size of button images in descbox.pcx
#define MOD_INFO_BUTTON_WIDTH 102
#define MOD_INFO_BUTTON_HEIGHT 17

#define MOD_INFO_BUTTON_ID 0x222003

char const * const hotseat_replay_save_path = "Saves\\Auto\\ai-move-replay-before-interturn.SAV";
char const * const hotseat_resume_save_path = "Saves\\Auto\\ai-move-replay-resume.SAV";

// Need to define memmove for use by TCC when generated code for functions that return a struct
void *
memmove (void * dest, void const * src, size_t size)
{
	char const * _src = src;
	char * _dest = dest, * tr = dest;
	for (char const * src_end = _src + size; _src != src_end; _src++, _dest++)
		*_dest = *_src;
	return tr;
}

// Also need to define memset for some reason
void *
memset (void * dest, int ch, size_t count)
{
	for (size_t n = 0; n < count; n++)
		((char *)dest)[n] = ch;
	return dest;
}

// Computes num/denom randomly rounded off so that E[rand_div (x,y)] = (float)x/y
// As an example: E[rand_div (6, 5)] = 1.2, it will return 1 with 80% probability and 2 with 20% prob.
int
rand_div (int num, int denom)
{
	int q = num / denom,
	    r = num % denom;
	return q + (rand_int (p_rand_object, __, denom) < r);
}

struct pause_for_popup {
	bool done; // Set to true to exit for loop
	bool redundant; // If true, this pause would overlap a previous one and so should not be counted
	long long ts_before;
};

struct pause_for_popup
pfp_init ()
{
	struct pause_for_popup tr;
	tr.done = false;
	tr.redundant = is->paused_for_popup;
	if (! tr.redundant) {
		is->paused_for_popup = true;
		QueryPerformanceCounter ((LARGE_INTEGER *)&tr.ts_before);
	}
	return tr;
}

void
pfp_finish (struct pause_for_popup * pfp)
{
	if ((! pfp->redundant) && (! pfp->done)) {
		long long ts_after;
		QueryPerformanceCounter ((LARGE_INTEGER *)&ts_after);
		is->time_spent_paused_during_popup += ts_after - pfp->ts_before;
		is->paused_for_popup = false;
	}
	pfp->done = true;
}

#define WITH_PAUSE_FOR_POPUP for (struct pause_for_popup pfp = pfp_init (); ! pfp.done; pfp_finish (&pfp))

int __fastcall
patch_show_popup (void * this, int edx, int param_1, int param_2)
{
	int tr;
	WITH_PAUSE_FOR_POPUP {
		is->show_popup_was_called = 1;
		tr = show_popup (this, __, param_1, param_2);
	}
	return tr;
}

void
pop_up_in_game_error (char const * msg)
{
	PopupForm * popup = get_popup_form ();
	popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_ERROR", -1, 0, 0, 0);
	PopupForm_add_text (popup, __, (char *)msg, false);
	patch_show_popup (popup, __, 0, 0);
}

void
memoize (int val)
{
	if (is->memo_len < is->memo_capacity)
		is->memo[is->memo_len++] = val;
	else {
		int new_capacity = not_below (100, 2 * is->memo_capacity);
		int * new_memo = malloc (new_capacity * sizeof is->memo[0]);
		if (new_memo != NULL) {
			for (int n = 0; n < is->memo_len; n++)
				new_memo[n] = is->memo[n];
			free (is->memo);
			is->memo = new_memo;
			is->memo_capacity = new_capacity;
			is->memo[is->memo_len++] = val;
		}
	}
}

void
clear_memo ()
{
	is->memo_len = 0;
}

// Resets is->current_config to the base config and updates the list of config names. Does NOT re-apply machine code edits.
void
reset_to_base_config ()
{
	struct c3x_config * cc = &is->current_config;

	stable_deinit (&cc->perfume_specs);

	// Free building-unit prereqs table
	size_t building_unit_prereqs_capacity = table_capacity (&cc->building_unit_prereqs);
	for (size_t n = 0; n < building_unit_prereqs_capacity; n++) {
		int ptr;
		if (table_get_by_index (&cc->building_unit_prereqs, n, &ptr) && ((ptr & 1) == 0))
			free ((void *)ptr);
	}
	table_deinit (&cc->building_unit_prereqs);

	// Free list of mills
	if (cc->mills != NULL) {
		free (cc->mills);
		cc->mills = NULL;
		cc->count_mills = 0;
	}

	// Free list of AI multi-city start extra palaces
	if (cc->ai_multi_start_extra_palaces != NULL) {
		free (cc->ai_multi_start_extra_palaces);
		cc->ai_multi_start_extra_palaces = NULL;
		cc->count_ai_multi_start_extra_palaces = 0;
		cc->ai_multi_start_extra_palaces_capacity = 0;
	}

	// Free list of PTW artillery types
	if (cc->ptw_arty_types != NULL) {
		free (cc->ptw_arty_types);
		cc->ptw_arty_types = NULL;
		cc->count_ptw_arty_types = 0;
		cc->ptw_arty_types_capacity = 0;
	}

	// Free era alias lists
	if (cc->civ_era_alias_lists != NULL) {
		for (int n = 0; n < cc->count_civ_era_alias_lists; n++) {
			struct civ_era_alias_list * list = &cc->civ_era_alias_lists[n];
			free (list->key);
			for (int k = 0; k < ERA_ALIAS_LIST_CAPACITY; k++)
				free (list->aliases[k]);
		}
		free (cc->civ_era_alias_lists);
		cc->civ_era_alias_lists = NULL;
		cc->count_civ_era_alias_lists = 0;
	}
	if (cc->leader_era_alias_lists != NULL) {
		for (int n = 0; n < cc->count_leader_era_alias_lists; n++) {
			struct leader_era_alias_list * list = &cc->leader_era_alias_lists[n];
			free (list->key);
			for (int k = 0; k < ERA_ALIAS_LIST_CAPACITY; k++) {
				free (list->aliases[k]);
				free (list->titles[k]);
			}
		}
		free (cc->leader_era_alias_lists);
		cc->leader_era_alias_lists = NULL;
		cc->count_leader_era_alias_lists = 0;
	}

	// Free unit limits table
	size_t unit_limits_capacity = table_capacity (&cc->unit_limits);
	for (size_t n = 0; n < unit_limits_capacity; n++) {
		int ptr;
		if (table_get_by_index (&cc->unit_limits, n, &ptr))
			free ((void *)ptr);
	}
	stable_deinit (&cc->unit_limits);

	// Free the linked list of loaded config names and the string name contained in each one
	if (is->loaded_config_names != NULL) {
		struct loaded_config_name * next = is->loaded_config_names;
		while (next != NULL) {
			struct loaded_config_name * to_free = next;
			next = next->next;
			free (to_free->name);
			free (to_free);
		}
	}

	// Overwrite the current config with the base config
	memcpy (&is->current_config, &is->base_config, sizeof is->current_config);

	// Recreate loaded config names list with just the base config
	is->loaded_config_names = malloc (sizeof *is->loaded_config_names);
	is->loaded_config_names->name = strdup ("(base)");
	is->loaded_config_names->next = NULL;
}

struct error_line {
	char text[200];
	struct error_line * next;
};

struct error_line *
add_error_line (struct error_line ** p_lines)
{
	struct error_line * tr = calloc (1, sizeof *tr);

	struct error_line ** p_prev = p_lines;
	while (*p_prev != NULL)
		p_prev = &(*p_prev)->next;
	*p_prev = tr;

	return tr;
}

void
add_unrecognized_line (struct error_line ** p_lines, struct string_slice const * name)
{
	struct error_line * line = add_error_line (p_lines);
	char s[100];
	snprintf (line->text, sizeof line->text, "^  %.*s", name->len, name->str);
	line->text[(sizeof line->text) - 1] = '\0';
}

void
free_error_lines (struct error_line * lines)
{
	while (lines != NULL) {
		struct error_line * next = lines->next;
		free (lines);
		lines = next;
	}
}

bool
find_improv_id_by_name (struct string_slice const * name, int * out)
{
	Improvement * improv;
	if (name->len <= sizeof improv->Name)
		for (int n = 0; n < p_bic_data->ImprovementsCount; n++)
			if (slice_matches_str (name, p_bic_data->Improvements[n].Name.S)) {
				*out = n;
				return true;
			}
	return false;
}

// start_id specifies where the search will start. It's useful for finding multiple type IDs with the same name, which commonly happens due to the
// game duplicating unit types.
bool
find_unit_type_id_by_name (struct string_slice const * name, int start_id, int * out)
{
	UnitType * unit_type;
	if (name->len <= sizeof unit_type->Name)
		for (int n = start_id; n < p_bic_data->UnitTypeCount; n++)
			if (slice_matches_str (name, p_bic_data->UnitTypes[n].Name)) {
				*out = n;
				return true;
			}
	return false;
}

bool
find_resource_id_by_name (struct string_slice const * name, int * out)
{
	Resource_Type * res_type;
	if (name->len <= sizeof res_type->Name)
		for (int n = 0; n < p_bic_data->ResourceTypeCount; n++)
			if (slice_matches_str (name, p_bic_data->ResourceTypes[n].Name)) {
				*out = n;
				return true;
			}
	return false;
}

// Converts a build name (like "Spearman" or "Granary") into a City_Order struct. Returns whether or not any improvement or unit type was found under
// the given name.
bool
find_city_order_by_name (struct string_slice const * name, City_Order * out)
{
	int id;
	if (find_improv_id_by_name (name, &id)) {
		out->OrderID = id;
		out->OrderType = COT_Improvement;
		return true;
	} else if (find_unit_type_id_by_name (name, 0, &id)) {
		out->OrderID = id;
		out->OrderType = COT_Unit;
		return true;
	} else
		return false;
}

// Returns a list of AI strat duplicates of the unit type with the given id. This means a list of all other unit types that are identical to the given
// one except were separated out so each can have only one AI strategy. Returns the total number of duplicates, which may be greater than dup_ids_len.
int
list_unit_type_duplicates (int type_id, int * out_dup_ids, int dup_ids_len)
{
	// type_id may be a duplicate itself. Find the base type so we can start assembling the array from there. Otherwise we may miss some.
	int alt_for_id = p_bic_data->UnitTypes[type_id].alternate_strategy_for_id;
	int base_type_id = (alt_for_id >= 0) ? alt_for_id : type_id;

	int tr = 0, current = base_type_id;
	while (1) {
		if (current != type_id) {
			if (tr < dup_ids_len)
				out_dup_ids[tr] = current;
			tr++;
		}
		int next;
		if (itable_look_up (&is->unit_type_duplicates, current, &next))
			current = next;
		else
			break;
	}

	return tr;
}

// A "recognizable" is something that contains the name of a unit, building, etc. When parsing one, it's possible for it to be grammatically valid but
// contain a name that doesn't match anything in the scenario data. That's a special kind of error. In that case, we record the error to report in a
// popup and continue parsing.
enum recognizable_parse_result {
	RPR_OK = 0,
	RPR_UNRECOGNIZED,
	RPR_PARSE_ERROR
};

struct perfume_spec {
	char name[36]; // Must be large enough to fit the name of a unit type or improvement
	int amount;
};

enum recognizable_parse_result
parse_perfume_spec (char ** p_cursor, struct error_line ** p_unrecognized_lines, void * out_perfume_spec)
{
	char * cur = *p_cursor;
	struct string_slice name;
	City_Order unused;
	int amount;
	if (parse_string (&cur, &name) &&
	    skip_punctuation (&cur, ':') &&
	    parse_int (&cur, &amount)) {
		*p_cursor = cur;
		if (find_city_order_by_name (&name, &unused)) {
			struct perfume_spec * out = out_perfume_spec;
			snprintf (out->name, sizeof out->name, "%.*s", name.len, name.str);
			out->name[(sizeof out->name) - 1] = '\0';
			out->amount = amount;
			return RPR_OK;
		} else {
			add_unrecognized_line (p_unrecognized_lines, &name);
			return RPR_UNRECOGNIZED;
		}
	} else
		return RPR_PARSE_ERROR;
}

enum recognizable_parse_result
parse_mill (char ** p_cursor, struct error_line ** p_unrecognized_lines, void * out_mill)
{
	char * cur = *p_cursor;
	struct string_slice improv_name;
	if (parse_string (&cur, &improv_name) &&
	    skip_punctuation (&cur, ':')) {

		short flags = 0;
		struct string_slice resource_name;
		while (1) {
			if (! parse_string (&cur, &resource_name))
				return RPR_PARSE_ERROR;
			else if (slice_matches_str (&resource_name, "local"))          flags |= MF_LOCAL;
			else if (slice_matches_str (&resource_name, "no-tech-req"))    flags |= MF_NO_TECH_REQ;
			else if (slice_matches_str (&resource_name, "yields"))         flags |= MF_YIELDS;
			else if (slice_matches_str (&resource_name, "show-bonus"))     flags |= MF_SHOW_BONUS;
			else if (slice_matches_str (&resource_name, "hide-non-bonus")) flags |= MF_HIDE_NON_BONUS;
			else
				break;
		}

		*p_cursor = cur;
		int improv_id, resource_id;
		int any_unrecognized = 0;
		if (! find_improv_id_by_name (&improv_name, &improv_id)) {
			add_unrecognized_line (p_unrecognized_lines, &improv_name);
			any_unrecognized = 1;
		}
		if (! find_resource_id_by_name (&resource_name, &resource_id)) {
			add_unrecognized_line (p_unrecognized_lines, &resource_name);
			any_unrecognized = 1;
		}
		if (any_unrecognized)
			return RPR_UNRECOGNIZED;
		else {
			struct mill * out = out_mill;
			out->improv_id = improv_id;
			out->resource_id = resource_id;
			out->flags = flags;
			return RPR_OK;
		}
	} else
		return RPR_PARSE_ERROR;
}

enum recognizable_parse_result
parse_era_alias_list (char ** p_cursor, struct error_line ** p_unrecognized_lines, void * out_leader_or_civ_alias_list, bool leader_else_civ_name)
{
	char * cur = *p_cursor;
	struct string_slice key;
	if (parse_string (&cur, &key) &&
	    skip_punctuation (&cur, ':')) {

		char * aliases[ERA_ALIAS_LIST_CAPACITY] = {0};
		char * titles[ERA_ALIAS_LIST_CAPACITY] = {0};
		int alias_count = 0,
		    female_bits = 0,
		    gender_specified_bits = 0; // For each alias, set to 1 if a gender was specified
		struct string_slice alias;
		while (1) {
			if (parse_string (&cur, &alias)) {
				if (alias_count < ERA_ALIAS_LIST_CAPACITY)
					aliases[alias_count] = extract_slice (&alias);

				// If we're parsing a list of leader names, read in the gender & title if present
				if (leader_else_civ_name) {
					char * inner_cur = cur;
					struct string_slice gender;
					struct string_slice title = {0};
					if (   skip_punctuation (&inner_cur, '(')
					    && parse_string (&inner_cur, &gender)
					    && (   skip_punctuation (&inner_cur, ')')
						|| (   skip_punctuation (&inner_cur, ',')
						    && parse_string (&inner_cur, &title)
						    && skip_punctuation (&inner_cur, ')')))
					    && (   slice_matches_str (&gender, "M")
						|| slice_matches_str (&gender, "m")
						|| slice_matches_str (&gender, "F")
						|| slice_matches_str (&gender, "f"))) {
						if (alias_count < 32) {
							if (slice_matches_str (&gender, "F") || slice_matches_str (&gender, "f"))
								female_bits |= 1 << alias_count;
							gender_specified_bits |= 1 << alias_count;
						}
						if (title.len > 0)
							titles[alias_count] = extract_slice (&title);
						cur = inner_cur;
					}
				}

				alias_count++;
			} else
				break;
		}
		if (alias_count == 0)
			return RPR_PARSE_ERROR;

		*p_cursor = cur;

		// Check that "key" matches a noun, adjective, or formal name of any civ ("race") in the scenario data. Store that civ.
		Race * race_matching_key = NULL; {
			for (int n = 0; n < p_bic_data->RacesCount; n++) {
				Race * race = &p_bic_data->Races[n];
				if (   (   leader_else_civ_name
				        && slice_matches_str (&key, race->LeaderName))
				    || (   (! leader_else_civ_name)
					&& (   slice_matches_str (&key, race->AdjectiveName)
					    || slice_matches_str (&key, race->CountryName)
					    || slice_matches_str (&key, race->SingularName)))) {
					race_matching_key = race;
					break;
				}
			}
		}
		if (race_matching_key == NULL) {
			add_unrecognized_line (p_unrecognized_lines, &key);
			return RPR_UNRECOGNIZED;
		}

		if (leader_else_civ_name) {
			struct leader_era_alias_list * out = out_leader_or_civ_alias_list;
			memset (out, 0, sizeof *out); // Make sure unspecified aliases & titles are NULL
			out->key = extract_slice (&key);
			for (int n = 0; n < alias_count; n++) {
				out->aliases[n] = aliases[n];
				out->titles[n] = titles[n];
			}

			// Set gender bits
			int unreplaced_bits = (race_matching_key->LeaderGender == 0) ? 0 : ~0;
			out->gender_bits = (unreplaced_bits & ~gender_specified_bits) | female_bits;


		} else {
			struct civ_era_alias_list * out = out_leader_or_civ_alias_list;
			memset (out, 0, sizeof *out);
			out->key = extract_slice (&key);
			for (int n = 0; n < alias_count; n++)
				out->aliases[n] = aliases[n];
		}

		return RPR_OK;
	} else
		return RPR_PARSE_ERROR;
}

enum recognizable_parse_result
parse_civ_name_alias_list  (char ** p_cursor, struct error_line ** p_unrecognized_lines, void * out_civ_era_alias_list)
{
	return parse_era_alias_list (p_cursor, p_unrecognized_lines, out_civ_era_alias_list, false);
}

enum recognizable_parse_result
parse_leader_name_alias_list  (char ** p_cursor, struct error_line ** p_unrecognized_lines, void * out_leader_era_alias_list)
{
	return parse_era_alias_list (p_cursor, p_unrecognized_lines, out_leader_era_alias_list, true);
}

struct parsed_unit_type_limit {
	char name[32]; // Same length as Name in Unit_Type
	struct unit_type_limit limit;
};

enum recognizable_parse_result
parse_unit_type_limit (char ** p_cursor, struct error_line ** p_unrecognized_lines, void * out_parsed_unit_type_limit)
{
	char * cur = *p_cursor;
	struct parsed_unit_type_limit * out = out_parsed_unit_type_limit;

	struct string_slice name;
	struct unit_type_limit limit = {0};
	if (skip_white_space (&cur) &&
	    parse_string (&cur, &name) &&
	    (name.len < (sizeof out->name)) &&
	    skip_punctuation (&cur, ':')) {

		do {
			int num;
			if (! parse_int (&cur, &num))
				return RPR_PARSE_ERROR;

			struct string_slice ss;
			if (parse_string (&cur, &ss)) {
				if (slice_matches_str (&ss, "per-city"))
					limit.per_city += num;
				else if (slice_matches_str (&ss, "cities-per"))
					limit.cities_per += num;
				else
					return RPR_PARSE_ERROR;
			} else
				limit.per_civ += num;

		} while (skip_punctuation (&cur, '+'));

		int unused;
		if (find_unit_type_id_by_name (&name, 0, &unused)) {
			memset (out->name, 0, sizeof out->name);
			strncpy (out->name, name.str, name.len);
			out->limit = limit;
			*p_cursor = cur;
			return RPR_OK;
		} else {
			add_unrecognized_line (p_unrecognized_lines, &name);
			*p_cursor = cur;
			return RPR_UNRECOGNIZED;
		}

	} else
		return RPR_PARSE_ERROR;
}

// Recognizable items are appended to out_list/count, which must have been previously initialized (NULL/0 is valid for an empty list).
// If an error occurs while reading, returns the location of the error inside the slice, specifically the number of characters before the unreadable
// item. If no error occurs, returns -1.
int
read_recognizables (struct string_slice const * s,
		    struct error_line ** p_unrecognized_lines,
		    int item_size,
		    enum recognizable_parse_result (* parse_item) (char **, struct error_line **, void *),
		    void ** inout_list,
		    int * inout_count)
{
	if (s->len <= 0)
		return -1;
	char * extracted_slice = extract_slice (s);
	char * cursor = extracted_slice;

	bool success = false;
	void * new_items = NULL;
	int count_new_items = 0;
	int new_items_capacity = 0;
	void * temp_item = malloc (item_size);

	while (1) {
		enum recognizable_parse_result result = parse_item (&cursor, p_unrecognized_lines, temp_item);
		if (result != RPR_PARSE_ERROR) {
			if (result == RPR_OK) {
				reserve (item_size, &new_items, &new_items_capacity, count_new_items);
				memcpy ((byte *)new_items + count_new_items * item_size, temp_item, item_size);
				count_new_items++;
			}

			if (skip_punctuation (&cursor, ',') && skip_white_space (&cursor))
				continue;
			else if (skip_horiz_space (&cursor) && (*cursor == '\0')) {
				success = true;
				break;
			} else
				break;
		} else
			break;
	}

	if (success && (count_new_items > 0)) {
		*inout_list = realloc (*inout_list, (*inout_count + count_new_items) * item_size);
		memcpy ((byte *)*inout_list + *inout_count * item_size, new_items, count_new_items * item_size);
		*inout_count += count_new_items;
	}
	free (temp_item);
	free (new_items);
	free (extracted_slice);
	return success ? -1 : cursor - extracted_slice;
}

// Like read_recognizables, returns -1 for success or the location of an error if there is one
int
read_building_unit_prereqs (struct string_slice const * s,
			    struct error_line ** p_unrecognized_lines,
			    struct table * building_unit_prereqs)
{
	if (s->len <= 0)
		return -1;
	char * extracted_slice = extract_slice (s);
	char * cursor = extracted_slice;
	bool success = false;

	struct prereq {
		int building_id;
		struct string_slice unit_type_name;
	} * new_prereqs = NULL;
	int new_prereqs_capacity = 0;
	int count_new_prereqs = 0;

	while (1) {
		struct string_slice building_name;
		if (skip_white_space (&cursor) && parse_string (&cursor, &building_name)) {
			int building_id;
			bool have_building_id = find_improv_id_by_name (&building_name, &building_id);
			if (! have_building_id)
				add_unrecognized_line (p_unrecognized_lines, &building_name);
			if (! skip_punctuation (&cursor, ':'))
				break;
			struct string_slice unit_type_name;
			while (skip_white_space (&cursor) && parse_string (&cursor, &unit_type_name)) {
				int unused;
				if (find_unit_type_id_by_name (&unit_type_name, 0, &unused)) { // if there is any by this name, later we'll deal with the possibility of multiple
					if (have_building_id) {
						reserve (sizeof new_prereqs[0], (void **)&new_prereqs, &new_prereqs_capacity, count_new_prereqs);
						new_prereqs[count_new_prereqs++] = (struct prereq) { .building_id = building_id, .unit_type_name = unit_type_name };
					}
				} else
					add_unrecognized_line (p_unrecognized_lines, &unit_type_name);
			}
			skip_punctuation (&cursor, ',');
		} else {
			success = (*cursor == '\0');
			break;
		}
	}

	// If parsing succeeded, add the new prereq rules to the table
	if (success)
		for (int n = 0; n < count_new_prereqs; n++) {
			struct prereq * prereq = &new_prereqs[n];

			int unit_type_id = -1;
			while (find_unit_type_id_by_name (&prereq->unit_type_name, unit_type_id + 1, &unit_type_id)) {

				// If this unit type ID is not already in the table, insert it paired with the encoded building ID
				int prev_val;
				if (! itable_look_up (building_unit_prereqs, unit_type_id, &prev_val))
					itable_insert (building_unit_prereqs, unit_type_id, (prereq->building_id << 1) | 1);

				// If the unit type ID is already associated with a building ID, create a list for both the old and new building IDs
				else if (prev_val & 1) {
					int * list = malloc (MAX_BUILDING_PREREQS_FOR_UNIT * sizeof *list);
					for (int n = 0; n < MAX_BUILDING_PREREQS_FOR_UNIT; n++)
						list[n] = -1;
					list[0] = prev_val >> 1; // Decode
					list[1] = prereq->building_id;
					itable_insert (building_unit_prereqs, unit_type_id, (int)list);

				// Otherwise, it's already associated with a list. Search the list for a free spot and fill it with the new building ID
				} else {
					int * list = (int *)prev_val;
					for (int n = 0; n < MAX_BUILDING_PREREQS_FOR_UNIT; n++)
						if (list[n] < 0) {
							list[n] = prereq->building_id;
							break;
						}
				}
			}
		}


	free (new_prereqs);
	free (extracted_slice);
	return success ? -1 : cursor - extracted_slice;
}

bool
read_ptw_arty_types (struct string_slice const * s,
		     struct error_line ** p_unrecognized_lines,
		     int ** p_ptw_arty_types,
		     int * p_count_ptw_arty_types,
		     int * p_ptw_arty_types_capacity)
{
	if (s->len <= 0)
		return true;
	char * extracted_slice = extract_slice (s);
	char * cursor = extracted_slice;
	bool success = false;

	while (1) {
		struct string_slice name;
		if (parse_string (&cursor, &name)) {

			int id = -1, matched_any = 0;
			while (find_unit_type_id_by_name (&name, id + 1, &id)) {
				int count = *p_count_ptw_arty_types;
				reserve (sizeof **p_ptw_arty_types, (void **)p_ptw_arty_types, p_ptw_arty_types_capacity, count);
				(*p_ptw_arty_types)[count] = id;
				*p_count_ptw_arty_types = count + 1;
				matched_any = 1;
			}

			if (! matched_any)
				add_unrecognized_line (p_unrecognized_lines, &name);

		} else {
			skip_white_space (&cursor);
			success = *cursor == '\0';
			break;
		}
	}

	free (extracted_slice);
	return success;
}

bool
read_ai_multi_start_extra_palaces (struct string_slice const * s,
				  struct error_line ** p_unrecognized_lines,
				  int ** p_ai_multi_start_extra_palaces,
				  int * p_count_ai_multi_start_extra_palaces,
				  int * p_ai_multi_start_extra_palaces_capacity)
{
	if (s->len <= 0)
		return true;
	char * extracted_slice = extract_slice (s);
	char * cursor = extracted_slice;
	bool success = false;

	while (1) {
		struct string_slice name;
		if (parse_string (&cursor, &name)) {
			int id;
			if (find_improv_id_by_name (&name, &id)) {
				int count = *p_count_ai_multi_start_extra_palaces;
				reserve (sizeof **p_ai_multi_start_extra_palaces, (void **)p_ai_multi_start_extra_palaces, p_ai_multi_start_extra_palaces_capacity, count);
				(*p_ai_multi_start_extra_palaces)[count] = id;
				*p_count_ai_multi_start_extra_palaces = count + 1;
			} else
				add_unrecognized_line (p_unrecognized_lines, &name);

		} else {
			skip_white_space (&cursor);
			success = *cursor == '\0';
			break;
		}
	}

	free (extracted_slice);
	return success;
}

bool
read_retreat_rules (struct string_slice const * s, int * out_val)
{
	struct string_slice trimmed = trim_string_slice (s, 1);
	if      (slice_matches_str (&trimmed, "standard" )) { *out_val = RR_STANDARD;  return true; }
	else if (slice_matches_str (&trimmed, "none"     )) { *out_val = RR_NONE;      return true; }
	else if (slice_matches_str (&trimmed, "all-units")) { *out_val = RR_ALL_UNITS; return true; }
	else if (slice_matches_str (&trimmed, "if-faster")) { *out_val = RR_IF_FASTER; return true; }
	else
		return false;
}

bool
read_line_drawing_override (struct string_slice const * s, int * out_val)
{
	struct string_slice trimmed = trim_string_slice (s, 1);
	if      (slice_matches_str (&trimmed, "never" )) { *out_val = LDO_NEVER;  return true; }
	else if (slice_matches_str (&trimmed, "wine"  )) { *out_val = LDO_WINE;   return true; }
	else if (slice_matches_str (&trimmed, "always")) { *out_val = LDO_ALWAYS; return true; }
	else
		return false;
}

struct parsable_field_bit {
	char * name;
	int bit_value;
};

bool
read_bit_field (struct string_slice const * s, struct parsable_field_bit const * bits, int count_bits, int * out_field)
{
	struct string_slice trimmed = trim_string_slice (s, 0);
	s = &trimmed;

	int tr;
	if (s->len <= 0)
		tr = 0;
	else if (slice_matches_str (s, "all"))
		tr = ~0;
	else {
		tr = 0;
		char * cursor = &s->str[0];
		char * s_end = &s->str[s->len];
		while (1) {
			struct string_slice name;

			if (cursor >= s_end)
				break;
			else if (! parse_string (&cursor, &name)) {
				skip_white_space (&cursor);
				if (cursor >= s_end)
					break;
				else
					return false; // Invalid character in value
			}

			bool matched_any = false;
			for (int n = 0; n < count_bits; n++)
				if (slice_matches_str (&name, bits[n].name)) {
					tr |= bits[n].bit_value;
					matched_any = true;
					break;
				}
			if (! matched_any)
				return false;
		}
	}
	*out_field = tr;
	return true;
}

struct config_parsing {
	char * file_path;
	char * text;
	char * cursor;
	struct string_slice key;
	int displayed_error_message;
};

enum config_parse_error {
	CPE_GENERIC,
	CPE_BAD_VALUE,
	CPE_BAD_BOOL_VALUE,
	CPE_BAD_INT_VALUE,
	CPE_BAD_KEY
};

void
handle_config_error_at (struct config_parsing * p, char * error_loc, enum config_parse_error err)
{
	char err_msg[1000];
	if (! p->displayed_error_message) {
		int line_no = 1;
		for (char * c = p->text; c < error_loc; c++)
			line_no += *c == '\n';

		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_ERROR", -1, 0, 0, 0);
		snprintf (err_msg, sizeof err_msg, "Error reading \"%s\" on line %d.", p->file_path, line_no);
		err_msg[(sizeof err_msg) - 1] = '\0';
		PopupForm_add_text (popup, __, err_msg, false);

		if (err == CPE_GENERIC) {
			if (p->key.str != NULL)
				snprintf (err_msg, sizeof err_msg, "^The last key successfully read was \"%.*s\".", p->key.len, p->key.str);
			else
				snprintf (err_msg, sizeof err_msg, "^Error occurred before any keys could be read.");
		} else if (err == CPE_BAD_VALUE)
			snprintf (err_msg, sizeof err_msg, "^The value for \"%.*s\" is invalid.", p->key.len, p->key.str);
		else if (err == CPE_BAD_BOOL_VALUE)
			snprintf (err_msg, sizeof err_msg, "^The value for \"%.*s\" is invalid. Expected \"true\" or \"false\".", p->key.len, p->key.str);
		else if (err == CPE_BAD_INT_VALUE)
			snprintf (err_msg, sizeof err_msg, "^The value for \"%.*s\" is invalid. Expected an integer.", p->key.len, p->key.str);
		else if (err == CPE_BAD_KEY)
			snprintf (err_msg, sizeof err_msg, "^The key name \"%.*s\" is not recognized.", p->key.len, p->key.str);
		err_msg[(sizeof err_msg) - 1] = '\0';
		PopupForm_add_text (popup, __, err_msg, false);

		patch_show_popup (popup, __, 0, 0);
		p->displayed_error_message = 1;
	}
}

void
handle_config_error (struct config_parsing * p, enum config_parse_error err)
{
	handle_config_error_at (p, p->cursor, err);
}

// Loads a config from the given file, layering it on top of is->current_config and appending its name to the list of loaded configs. Does NOT
// re-apply machine code edits.
void
load_config (char const * file_path, int path_is_relative_to_mod_dir)
{
	char err_msg[1000];
	struct c3x_config * cfg = &is->current_config;

	int full_path_size = 2 * MAX_PATH;
	char * full_path = malloc (full_path_size);
	if (path_is_relative_to_mod_dir)
		snprintf (full_path, full_path_size, "%s\\%s", is->mod_rel_dir, file_path);
	else
		strncpy (full_path, file_path, full_path_size);
	full_path[full_path_size - 1] = '\0';

	char * text; {
		char * utf8_text = file_to_string (full_path);
		if (utf8_text == NULL) {
			free (full_path);
			return;
		}
		text = utf8_to_windows1252 (utf8_text);
		free (utf8_text);
		if (text == NULL) {
			snprintf (err_msg, sizeof err_msg, "Failed to re-encode contents of \"%s\". This file must contain UTF-8 text and only characters usable by Civ 3.", full_path);
			err_msg[(sizeof err_msg) - 1] = '\0';
			pop_up_in_game_error (err_msg);
			free (full_path);
			return;
		}
	}

	struct perfume_spec * perfume_spec_list = NULL;
	int perfume_spec_count = 0;

	struct parsed_unit_type_limit * parsed_unit_type_limits = NULL;
	int parsed_unit_type_limit_count = 0;

	struct config_parsing p = { .file_path = full_path, .text = text, .cursor = text, .key = {0}, .displayed_error_message = 0 };
	struct error_line * unrecognized_lines = NULL;
	while (1) {
		skip_horiz_space (&p.cursor);
		if (*p.cursor == '\0')
			break;
		else if (*p.cursor == '\n')
			p.cursor++; // Continue to next line
		else if (*p.cursor == ';')
			skip_to_line_end (&p.cursor); // Skip comment line
		else if (*p.cursor == '[')
			skip_to_line_end (&p.cursor); // Skip section line
		else if (parse_string (&p.cursor, &p.key) && skip_punctuation (&p.cursor, '=')) { // Parse key and equals sign

			struct string_slice value;
			if (parse_string (&p.cursor, &value) || parse_bracketed_block (&p.cursor, &value)) { // Parse value
				int ival, offset, recog_err_offset;

				// if key is for a boolean option
				if (stable_look_up_slice (&is->boolean_config_offsets, &p.key, &offset)) {
					if (read_int (&value, &ival))
						*((char *)cfg + offset) = ival != 0;
					else
						handle_config_error (&p, CPE_BAD_BOOL_VALUE);

				// if key is for an integer option
				} else if (stable_look_up_slice (&is->integer_config_offsets, &p.key, &offset)) {
					if (read_int (&value, &ival))
						*(int *)((byte *)cfg + offset) = ival;
					else
						handle_config_error (&p, CPE_BAD_INT_VALUE);

				// if key is for something special
				} else if (slice_matches_str (&p.key, "perfume_specs")) {
					if (0 <= (recog_err_offset = read_recognizables (&value,
											 &unrecognized_lines,
											 sizeof (struct perfume_spec),
											 parse_perfume_spec,
											 (void **)&perfume_spec_list,
											 &perfume_spec_count)))
						handle_config_error_at (&p, value.str + recog_err_offset, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "building_prereqs_for_units")) {
					if (0 <= (recog_err_offset = read_building_unit_prereqs (&value, &unrecognized_lines, &cfg->building_unit_prereqs)))
						handle_config_error_at (&p, value.str + recog_err_offset, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "buildings_generating_resources")) {
					if (0 <= (recog_err_offset = read_recognizables (&value,
											 &unrecognized_lines,
											 sizeof (struct mill),
											 parse_mill,
											 (void **)&cfg->mills,
											 &cfg->count_mills)))
						handle_config_error_at (&p, value.str + recog_err_offset, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "ai_multi_start_extra_palaces")) {
					if (! read_ai_multi_start_extra_palaces (&value,
										 &unrecognized_lines,
										 &cfg->ai_multi_start_extra_palaces,
										 &cfg->count_ai_multi_start_extra_palaces,
										 &cfg->ai_multi_start_extra_palaces_capacity))
					    handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "land_retreat_rules")) {
					if (! read_retreat_rules (&value, (int *)&cfg->land_retreat_rules))
						handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "sea_retreat_rules")) {
					if (! read_retreat_rules (&value, (int *)&cfg->sea_retreat_rules))
						handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "draw_lines_using_gdi_plus")) {
					if (! read_line_drawing_override (&value, (int *)&cfg->draw_lines_using_gdi_plus))
						handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "special_defensive_bombard_rules")) {
					struct parsable_field_bit bits[] = {
						{"lethal"        , SDBR_LETHAL},
						{"not-invisible" , SDBR_NOT_INVISIBLE},
						{"aerial"        , SDBR_AERIAL},
						{"blitz"         , SDBR_BLITZ},
						{"docked-vs-land", SDBR_DOCKED_VS_LAND},
					};
					if (! read_bit_field (&value, bits, ARRAY_LEN (bits), (int *)&cfg->special_defensive_bombard_rules))
						handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "special_zone_of_control_rules")) {
					struct parsable_field_bit bits[] = {
						{"lethal"    , SZOCR_LETHAL},
						{"aerial"    , SZOCR_AERIAL},
						{"amphibious", SZOCR_AMPHIBIOUS},
					};
					if (! read_bit_field (&value, bits, ARRAY_LEN (bits), (int *)&cfg->special_zone_of_control_rules))
						handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "ptw_like_artillery_targeting")) {
					if (! read_ptw_arty_types (&value,
								   &unrecognized_lines,
								   &cfg->ptw_arty_types,
								   &cfg->count_ptw_arty_types,
								   &cfg->ptw_arty_types_capacity))
						handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "civ_aliases_by_era")) {
					if (0 <= (recog_err_offset = read_recognizables (&value,
											 &unrecognized_lines,
											 sizeof (struct civ_era_alias_list),
											 parse_civ_name_alias_list,
											 (void **)&cfg->civ_era_alias_lists,
											 &cfg->count_civ_era_alias_lists)))
						handle_config_error_at (&p, value.str + recog_err_offset, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "leader_aliases_by_era")) {
					if (0 <= (recog_err_offset = read_recognizables (&value,
											 &unrecognized_lines,
											 sizeof (struct leader_era_alias_list),
											 parse_leader_name_alias_list,
											 (void **)&cfg->leader_era_alias_lists,
											 &cfg->count_leader_era_alias_lists)))
						handle_config_error_at (&p, value.str + recog_err_offset, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "unit_limits")) {
					if (0 <= (recog_err_offset = read_recognizables (&value,
											 &unrecognized_lines,
											 sizeof (struct parsed_unit_type_limit),
											 parse_unit_type_limit,
											 (void **)&parsed_unit_type_limits,
											 &parsed_unit_type_limit_count)))
						handle_config_error_at (&p, value.str + recog_err_offset, CPE_BAD_VALUE);

				// if key is for an obsolete option
				} else if (slice_matches_str (&p.key, "patch_disembark_immobile_bug")) {
					if (read_int (&value, &ival))
						cfg->patch_blocked_disembark_freeze = ival != 0;
					else
						handle_config_error (&p, CPE_BAD_BOOL_VALUE);
				} else if (slice_matches_str (&p.key, "anarchy_length_reduction_percent")) {
					if (read_int (&value, &ival))
						cfg->anarchy_length_percent = 100 - ival;
					else
						handle_config_error (&p, CPE_BAD_INT_VALUE);
				} else if (slice_matches_str (&p.key, "adjust_minimum_city_separation")) {
					if (read_int (&value, &ival))
						cfg->minimum_city_separation = ival + 1;
					else
						handle_config_error (&p, CPE_BAD_INT_VALUE);
				} else if (slice_matches_str (&p.key, "reduce_max_escorts_per_ai_transport")) {
					if (read_int (&value, &ival))
						cfg->max_ai_naval_escorts = 3 - ival;
					else
						handle_config_error (&p, CPE_BAD_INT_VALUE);
				} else if (slice_matches_str (&p.key, "retreat_rules")) {
					int rules;
					if (read_retreat_rules (&value, &rules)) {
						cfg->land_retreat_rules = rules;
						cfg->sea_retreat_rules  = rules;
					} else
						handle_config_error (&p, CPE_BAD_VALUE);
				} else if (slice_matches_str (&p.key, "halve_ai_research_rate")) {
					if (read_int (&value, &ival))
						if (ival) // halving = true => set multiplier to 50, otherwise do nothing
							cfg->ai_research_multiplier = 50;
					else
						handle_config_error (&p, CPE_BAD_BOOL_VALUE);
				} else if (slice_matches_str (&p.key, "enable_ai_two_city_start")) {
					if (read_int (&value, &ival))
						cfg->ai_multi_city_start = (ival != 0) ? 2 : 0;
					else
						handle_config_error (&p, CPE_BAD_BOOL_VALUE);

				} else {
					handle_config_error (&p, CPE_BAD_KEY);
				}


			} else { // Failed to parse value
				handle_config_error (&p, CPE_BAD_VALUE);
				skip_to_line_end (&p.cursor);
			}

		} else { // Failed to categorize line
			handle_config_error (&p, CPE_GENERIC);
			skip_to_line_end (&p.cursor);
		}
	}

	if (cfg->warn_about_unrecognized_names && (unrecognized_lines != NULL)) {
		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_WARNING", -1, 0, 0, 0);
		char s[200];
		snprintf (s, sizeof s, "Unrecognized names in %s:", full_path);
		s[(sizeof s) - 1] = '\0';
		PopupForm_add_text (popup, __, s, false);
		for (struct error_line * line = unrecognized_lines; line != NULL; line = line->next)
			PopupForm_add_text (popup, __, line->text, false);
		patch_show_popup (popup, __, 0, 0);
	}

	// Copy perfume specs from list to table
	if (perfume_spec_list != NULL) {
		for (int n = 0; n < perfume_spec_count; n++) {
			struct perfume_spec * ps = &perfume_spec_list[n];
			stable_insert (&cfg->perfume_specs, ps->name, ps->amount);
		}
		free (perfume_spec_list);
	}

	// Copy unit type limits from list to table
	if (parsed_unit_type_limits != NULL) {
		for (int n = 0; n < parsed_unit_type_limit_count; n++) {
			struct parsed_unit_type_limit * parsed_lim = &parsed_unit_type_limits[n];
			struct unit_type_limit * lim_values = malloc (sizeof *lim_values);
			*lim_values = parsed_lim->limit;
			stable_insert (&cfg->unit_limits, parsed_lim->name, (int)lim_values);
		}
		free (parsed_unit_type_limits);
	}

	free (text);
	free_error_lines (unrecognized_lines);

	struct loaded_config_name * top_lcn = is->loaded_config_names;
	while (top_lcn->next != NULL)
		top_lcn = top_lcn->next;

	struct loaded_config_name * new_lcn = malloc (sizeof *new_lcn);
	new_lcn->name = full_path;
	new_lcn->next = NULL;

	top_lcn->next = new_lcn;
}

bool __fastcall
patch_Leader_impl_would_raze_city (Leader * this, int edx, City * city)
{
	return is->current_config.prevent_razing_by_players ? false : Leader_impl_would_raze_city (this, __, city);
}

// This function is used to fix a bug where the game would freeze when using disembark all on a transport that contained an immobile unit. The bug
// comes from the fact that the function to disembark all units loops continuously over units in the transport until there are none left that
// can be disembarked. The problem is the logic to check disembarkability erroneously reports immobile units as disembarkable when they're not,
// so the program gets stuck in an infinite loop. The fix affects the function that checks disembarkability, replacing a call to
// Unit_get_max_move_points with a call to the function below. This function returns zero for immobile units, causing the caller to report
// (correctly) that the unit cannot be disembarked.
int __fastcall
patch_Unit_get_max_move_points_for_disembarking (Unit * this)
{
	if (is->current_config.patch_blocked_disembark_freeze &&
	    UnitType_has_ability (&p_bic_data->UnitTypes[this->Body.UnitTypeID], __, UTA_Immobile))
		return 0;
	else
		return Unit_get_max_move_points (this);
}

// This func implements GA remaining turns indicator. It intercepts a call to some kind of text processing function when it's called to process the
// text in the lower right (containing current research, GA status, & mobilization) and splices in the number of remaining GA turns if in a GA.
int __fastcall
patch_PCX_Image_process_tech_ga_status (PCX_Image * this, int edx, char * str)
{
	Leader * player = &leaders[p_main_screen_form->Player_CivID];
	if (is->current_config.show_golden_age_turns_remaining &&
	    (*p_current_turn_no < player->Golden_Age_End)) {
		int turns_left = player->Golden_Age_End - *p_current_turn_no;
		char const * ga_label = (*p_labels)[LBL_GOLDEN_AGE];
		char const * ga_str_start = strstr (str, ga_label);
		if (ga_str_start != NULL) {
			char s[250];
			char const * ga_str_end = ga_str_start + strlen (ga_label);
			snprintf (s, sizeof s, "%.*s (%d)%s", ga_str_end - str, str, turns_left, ga_str_end);
			s[(sizeof s) - 1] = '\0';
			strncpy (str, s, sizeof s);
		}
	}
	return PCX_Image_process_text (this, __, str);
}

bool __fastcall
patch_Leader_is_tile_visible (Leader * this, int edx, int x, int y)
{
	Tile_Body * tile = &tile_at (x, y)->Body;
	unsigned vis_bits = tile->FOWStatus | tile->V3 | tile->Visibility | tile->field_D0_Visibility;
	if (vis_bits & (1 << this->ID))
		return true;
	else if (is->current_config.share_visibility_in_hoseat && // if shared hotseat vis is enabled AND
		 (*p_is_offline_mp_game && ! *p_is_pbem_game) && // is hotseat game AND
		 ((1 << this->ID) & *p_human_player_bits) && // "this" is a human player AND
		 (vis_bits & *p_human_player_bits)) // any human player has visibility on the tile
		return true;
	else
		return false;
}

bool __fastcall
patch_Main_Screen_Form_is_unit_visible_to_player (Main_Screen_Form * this, int edx, int tile_x, int tile_y, Unit * unit)
{
	return (unit->Body.CivID == this->Player_CivID) || patch_Leader_is_tile_visible (&leaders[this->Player_CivID], __, tile_x, tile_y);
}

enum direction
reverse_dir (enum direction dir)
{
	enum direction const reversed[] = {
		DIR_ZERO, // DIR_ZERO
		DIR_SW  , // DIR_NE
		DIR_W   , // DIR_E
		DIR_NW  , // DIR_SE
		DIR_N   , // DIR_S
		DIR_NE  , // DIR_SW
		DIR_E   , // DIR_W
		DIR_SE  , // DIR_NW
		DIR_S   , // DIR_N
	};
	int n = (int)dir;
	if ((n >= 0) && (n < ARRAY_LEN (reversed)))
		return reversed[n];
	else
		return DIR_ZERO;
}

void
wrap_tile_coords (Map * map, int * x, int * y)
{
	if (map->Flags & 1) {
		if      (*x < 0)           *x += map->Width;
		else if (*x >= map->Width) *x -= map->Width;
	}
	if (map->Flags & 2) {
		if      (*y < 0)            *y += map->Height;
		else if (*y >= map->Height) *y -= map->Height;
	}
}

void
tile_index_to_coords (Map * map, int index, int * out_x, int * out_y)
{
	if ((index >= 0) && (index < map->TileCount)) {
		int width = map->Width;
		int double_row = index / width, double_row_rem = index % width;
		if (double_row_rem < width/2) {
			*out_x = 2 * double_row_rem;
			*out_y = 2 * double_row;
		} else {
			*out_x = 1 + 2 * (double_row_rem - width/2);
			*out_y = 2 * double_row + 1;
		}
	} else
		*out_x = *out_y = -1;
}

Tile *
tile_at_index (Map * map, int i)
{
	int x, y;
	tile_index_to_coords (map, i, &x, &y);
	return tile_at (x, y);
}

void
get_neighbor_coords (Map * map, int x, int y, int neighbor_index, int * out_x, int * out_y)
{
	int dx, dy;
	neighbor_index_to_diff (neighbor_index, &dx, &dy);
	*out_x = x + dx;
	*out_y = y + dy;
	wrap_tile_coords (map, out_x, out_y);
}

City *
get_city_ptr (int id)
{
	if ((p_cities->Cities != NULL) &&
	    (id >= 0) && (id <= p_cities->LastIndex)) {
		City_Body * body = p_cities->Cities[id].City;
		if (body != NULL) {
			City * city = (City *)((char *)body - offsetof (City, Body));
			if (city != NULL)
				return city;
		}
	}
	return NULL;
}

Tile * __stdcall
tile_at_city_or_null (City * city_or_null)
{
	if (city_or_null)
		return tile_at (city_or_null->Body.X, city_or_null->Body.Y);
	else
		return p_null_tile;
}

Unit *
get_unit_ptr (int id)
{
	if ((p_units->Units != NULL) &&
	    (id >= 0) && (id <= p_units->LastIndex)) {
		Unit_Body * body = p_units->Units[id].Unit;
		if (body != NULL) {
			Unit * unit = (Unit *)((char *)body - offsetof (Unit, Body));
			if (unit != NULL)
				return unit;
		}
	}
	return NULL;
}

struct unit_tile_iter {
	int id;
	int item_index;
	Unit * unit;
};

void
uti_next (struct unit_tile_iter * uti)
{
	if (((p_tile_units->Base.Items == NULL) || (uti->item_index < 0)) ||
	    (uti->item_index > p_tile_units->Base.LastIndex)) {
		uti->item_index = -1;
		uti->id = p_tile_units->DefaultValue;
	} else {
		Base_List_Item * item = &p_tile_units->Base.Items[uti->item_index];
		uti->item_index = item->V;
		uti->id = (int)item->Object;
	}
	uti->unit = get_unit_ptr (uti->id);
}

struct unit_tile_iter
uti_init (Tile * tile)
{
	struct unit_tile_iter tr;
	int tile_unit_id = tile->vtable->m40_get_TileUnit_ID (tile);
	tr.id = TileUnits_TileUnitID_to_UnitID (p_tile_units, __, tile_unit_id, &tr.item_index);
	tr.unit = get_unit_ptr (tr.id);
	return tr;
}

#define FOR_UNITS_ON(uti_name, tile) for (struct unit_tile_iter uti_name = uti_init (tile); uti_name.id != -1; uti_next (&uti_name))

struct citizen_iter {
	int index;
	Citizens * list;
	Citizen_Base * ctzn;
};

void
ci_next (struct citizen_iter * ci)
{
	while (1) {
		ci->index++;
		if (ci->index > ci->list->LastIndex) {
			ci->ctzn = NULL;
			break;
		} else {
			Citizen_Body * body = ci->list->Items[ci->index].Body;
			if ((body != NULL) && ((int)body != offsetof (Citizen_Base, Body))) {
				ci->ctzn = (Citizen_Base *)((int)body - offsetof (Citizen_Base, Body));
				break;
			}
		}
	}
}

struct citizen_iter
ci_init (City * city)
{
	struct citizen_iter tr;
	tr.index = -1;
	tr.list = &city->Body.Citizens;
	tr.ctzn = NULL;
	if (city->Body.Citizens.Items != NULL)
		ci_next (&tr);
	return tr;
}

#define FOR_CITIZENS_IN(ci_name, city) for (struct citizen_iter ci_name = ci_init (city); (ci_name.list->Items != NULL) && (ci_name.index <= ci.list->LastIndex); ci_next (&ci_name))

struct city_of_iter {
	int city_id;
	int civ_id;
	City * city;
};

void
coi_next (struct city_of_iter * coi)
{
	coi->city_id++;
	while (coi->city_id <= p_cities->LastIndex) {
		City_Body * body = p_cities->Cities[coi->city_id].City;
		if ((body != NULL) && ((int)body != offsetof (City, Body)) && (body->CivID == coi->civ_id)) {
			coi->city = (City *)((char *)body - offsetof (City, Body));
			break;
		}
		coi->city_id++;
	}
}

struct city_of_iter
coi_init (int civ_id)
{
	struct city_of_iter tr = { .city_id = -1, .civ_id = civ_id, .city = NULL };
	if (p_cities->Cities != NULL)
		coi_next (&tr);
	else
		tr.city_id = p_cities->LastIndex + 1;
	return tr;
}

#define FOR_CITIES_OF(coi_name, civ_id) for (struct city_of_iter coi_name = coi_init (civ_id); coi_name.city_id <= p_cities->LastIndex; coi_next (&coi_name))

struct tiles_around_iter {
	int center_x, center_y;
	int n, num_tiles;
	Tile * tile;
};

void
tai_next (struct tiles_around_iter * tai)
{
	tai->tile = p_null_tile;
	while ((tai->n < tai->num_tiles) && (tai->tile == p_null_tile)) {
		tai->n += 1;
		int tx, ty;
		get_neighbor_coords (&p_bic_data->Map, tai->center_x, tai->center_y, tai->n, &tx, &ty);
		if ((tx >= 0) && (tx < p_bic_data->Map.Width) && (ty >= 0) && (ty < p_bic_data->Map.Height))
			tai->tile = tile_at (tx, ty);
	}
}

struct tiles_around_iter
tai_init (int num_tiles, int x, int y)
{
	struct tiles_around_iter tr;
	tr.center_x = x;
	tr.center_y = y;
	tr.n = 0;
	tr.num_tiles = num_tiles;
	tr.tile = tile_at (x, y);
	return tr;
}

#define FOR_TILES_AROUND(tai_name, _num_tiles, _x, _y) for (struct tiles_around_iter tai_name = tai_init (_num_tiles, _x, _y); (tai_name.n < tai_name.num_tiles); tai_next (&tai_name))

void
tai_get_coords (struct tiles_around_iter * tai, int * out_x, int * out_y)
{
	if (tai->tile != p_null_tile)
		get_neighbor_coords (&p_bic_data->Map, tai->center_x, tai->center_y, tai->n, out_x, out_y);
	else {
		*out_x = -1;
		*out_y = -1;
	}
}

bool
has_active_building (City * city, int improv_id)
{
	Leader * owner = &leaders[city->Body.CivID];
	Improvement * improv = &p_bic_data->Improvements[improv_id];
	return City_has_improvement (city, __, improv_id, 1) && // building is physically present in city AND
		((improv->ObsoleteID < 0) || (! Leader_has_tech (owner, __, improv->ObsoleteID))) && // building is not obsolete AND
		((improv->GovernmentID < 0) || (improv->GovernmentID == owner->GovernmentType)); // building is not restricted to a different govt
}

bool
can_generate_resource (int for_civ_id, struct mill * mill)
{
	int req_tech_id = (mill->flags & MF_NO_TECH_REQ) ? -1 : p_bic_data->ResourceTypes[mill->resource_id].RequireID;
	return (req_tech_id < 0) || Leader_has_tech (&leaders[for_civ_id], __, req_tech_id);
}

void
init_unit_type_count (Leader * leader)
{
	int id = leader->ID;
	struct table * counts = &is->unit_type_counts[id];

	if (counts->len > 0)
		table_deinit (counts);

	if (p_units->Units != NULL)
		for (int n = 0; n <= p_units->LastIndex; n++) {
			Unit_Body * body = p_units->Units[n].Unit;
			if ((body != NULL) && ((int)body != offsetof (Unit, Body)) && (body->CivID == id)) {
				int prev_count;
				if (! itable_look_up (counts, body->UnitTypeID, &prev_count))
					prev_count = 0;
				itable_insert (counts, body->UnitTypeID, prev_count + 1);
			}
		}

	is->unit_type_count_init_bits |= 1 << id;
}

int
get_unit_type_count (Leader * leader, int unit_type_id)
{
	int id = leader->ID;
	struct table * counts = &is->unit_type_counts[id];

	if ((is->unit_type_count_init_bits & 1<<id) == 0)
		init_unit_type_count (leader);

	int count;
	if (itable_look_up (counts, unit_type_id, &count))
		return count;
	else
		return 0;
}

void
change_unit_type_count (Leader * leader, int unit_type_id, int amount)
{
	int id = leader->ID;
	struct table * counts = &is->unit_type_counts[id];
	if ((is->unit_type_count_init_bits & (1 << id)) == 0)
		init_unit_type_count (leader);

	int prev_amount;
	if (! itable_look_up (counts, unit_type_id, &prev_amount))
		prev_amount = 0;
	itable_insert (counts, unit_type_id, prev_amount + amount);
}

// If this unit type is limited, returns true and writes how many units of the type the given player is allowed to "out_limit". If the type is not
// limited, returns false.
bool
get_unit_limit (Leader * leader, int unit_type_id, int * out_limit)
{
	UnitType * type = &p_bic_data->UnitTypes[unit_type_id];
	struct unit_type_limit * lim;
	if ((unit_type_id >= 0) && (unit_type_id < p_bic_data->UnitTypeCount) &&
	    stable_look_up (&is->current_config.unit_limits, type->Name, (int *)&lim)) {
		int city_count = leader->Cities_Count;
		int tr = lim->per_civ + lim->per_city * city_count;
		if (lim->cities_per != 0)
			tr += city_count / lim->cities_per;
		*out_limit = tr;
		return true;
	} else
		return false;
}

// This this unit type is limited, returns true and writes to "out_available" how many units the given player can add before reaching the limit. If
// the type is not limited, returns false.
bool
get_available_unit_count (Leader * leader, int unit_type_id, int * out_available)
{
	int limit;
	if (get_unit_limit (leader, unit_type_id, &limit)) {
		int count = get_unit_type_count (leader, unit_type_id);
		int dups[30];
		int dups_count = list_unit_type_duplicates (unit_type_id, dups, ARRAY_LEN (dups));
		for (int n = 0; n < dups_count; n++)
			count += get_unit_type_count (leader, dups[n]);

		*out_available = limit - count;
		return true;
	} else
		return false;
}

int __stdcall
intercept_consideration (int valuation)
{
	City * city = is->ai_considering_production_for_city;
	City_Order * order = &is->ai_considering_order;

	char * order_name; {
		if (order->OrderType == COT_Improvement)
			order_name = p_bic_data->Improvements[order->OrderID].Name.S;
		else
			order_name = p_bic_data->UnitTypes[order->OrderID].Name;
	}

	// Apply perfume
	int perfume_amount;
	if (stable_look_up (&is->current_config.perfume_specs, order_name, &perfume_amount))
		valuation += perfume_amount;

	// Expand the list of valuations if necessary
	reserve (sizeof is->ai_prod_valuations[0], (void **)&is->ai_prod_valuations, &is->ai_prod_valuations_capacity, is->count_ai_prod_valuations);

	// Record this valuation
	int n = is->count_ai_prod_valuations++;
	is->ai_prod_valuations[n] = (struct ai_prod_valuation) {
		.order_type = order->OrderType,
		.order_id = order->OrderID,
		.point_value = valuation
	};

	return valuation;
}

// Returns a pointer to a bitfield that can be used to record resource access for resource IDs >= 32. This procedure can work with any city ID since
// it allocates and zero-initializes these bit fields as necessary. The given resource ID must be at least 32. The index of the bit for that resource
// within the field is resource_id%32.
unsigned *
get_extra_resource_bits (int city_id, int resource_id)
{
	int extra_resource_count = not_below (0, p_bic_data->ResourceTypeCount - 32);
	int ints_per_city = 1 + extra_resource_count/32;
	if (city_id >= is->extra_available_resources_capacity) {
		int new_capacity = city_id + 100;
		unsigned * new_array = calloc (new_capacity * ints_per_city, sizeof new_array[0]);
		if (is->extra_available_resources != NULL) {
			memcpy (new_array, is->extra_available_resources, is->extra_available_resources_capacity * ints_per_city * sizeof (unsigned));
			free (is->extra_available_resources);
		}
		is->extra_available_resources = new_array;
		is->extra_available_resources_capacity = new_capacity;
	}
	return &is->extra_available_resources[city_id * ints_per_city + (resource_id-32)/32];
}

void __stdcall
intercept_set_resource_bit (City * city, int resource_id)
{
	if (resource_id < 32)
		city->Body.Available_Resources |= 1 << resource_id;
	else
		*get_extra_resource_bits (city->Body.ID, resource_id) |= 1 << (resource_id&31);
}

// Must forward declare this function since there's a circular dependency between it and patch_City_has_resource
bool has_resources_required_by_building_r (City * city, int improv_id, int max_req_resource_id);

bool __fastcall
patch_City_has_resource (City * this, int edx, int resource_id)
{
	bool tr;
	if (is->current_config.patch_phantom_resource_bug &&
	    (resource_id >= 32) && (resource_id < p_bic_data->ResourceTypeCount) &&
	    (! City_has_trade_connection_to_capital (this))) {
		unsigned bits = (this->Body.ID < is->extra_available_resources_capacity) ? *get_extra_resource_bits (this->Body.ID, resource_id) : 0;
		tr = (bits >> (resource_id&31)) & 1;
	} else
		tr = City_has_resource (this, __, resource_id);

	// Check if access to this resource is provided by a building in the city
	if (! tr)
		for (int n = 0; n < is->current_config.count_mills; n++) {
			struct mill * mill = &is->current_config.mills[n];
			if ((mill->resource_id == resource_id) &&
			    (mill->flags & MF_LOCAL) &&
			    can_generate_resource (this->Body.CivID, mill) &&
			    has_active_building (this, mill->improv_id) &&
			    has_resources_required_by_building_r (this, mill->improv_id, mill->resource_id - 1)) {
				tr = true;
				break;
			}
		}

	return tr;
}

// Checks if the resource requirements for an improvement are satisfied in a given city. The "max_req_resource_id" parameter is to guard against
// infinite loops in case of circular resource dependencies due to mills. The way it works is that resource requirements can only be satisfied if
// their ID is not greater than that limit. This function is called recursively by patch_City_has_resource and in the recursive calls, the limit is
// set to be below the resource ID provided by the mill being considered. So, when considering resource production chains, the limit approaches zero
// with each recursive call, hence infinite loops are not possible.
bool
has_resources_required_by_building_r (City * city, int improv_id, int max_req_resource_id)
{
	Improvement * improv = &p_bic_data->Improvements[improv_id];
	if (! (improv->ImprovementFlags & ITF_Required_Goods_Must_Be_In_City_Radius)) {
		for (int n = 0; n < 2; n++) {
			int res_id = (&improv->Resource1ID)[n];
			if ((res_id >= 0) &&
			    ((res_id > max_req_resource_id) || (! patch_City_has_resource (city, __, res_id))))
				return false;
		}
		return true;
	} else {
		int * targets = &improv->Resource1ID;
		if ((targets[0] < 0) && (targets[1] < 0))
			return true;
		int finds[2] = {0, 0};

		int civ_id = city->Body.CivID;
		FOR_TILES_AROUND(tai, 21, city->Body.X, city->Body.Y) {
			if (tai.tile->vtable->m38_Get_Territory_OwnerID (tai.tile) == civ_id) {
				int res_here = Tile_get_resource_visible_to (tai.tile, __, civ_id);
				if (res_here >= 0) {
					finds[0] |= targets[0] == res_here;
					finds[1] |= targets[1] == res_here;
				}
			}
		}

		return ((targets[0] < 0) || finds[0]) && ((targets[1] < 0) || finds[1]);
	}
}

bool
has_resources_required_by_building (City * city, int improv_id)
{
	return has_resources_required_by_building_r (city, improv_id, INT_MAX);
}


// Recomputes yields in cities with active mills that depend on input resources. Intended to be called when an input resource has been potentially
// gained or lost. Recomputes only for the cities of a given leader or, if NULL, for all cities on the map.
void
recompute_mill_yields_after_resource_change (Leader * leader_or_null)
{
	if (p_cities->Cities != NULL)
		for (int city_index = 0; city_index <= p_cities->LastIndex; city_index++) {
			City * city = get_city_ptr (city_index);
			if ((city != NULL) &&
			    ((leader_or_null == NULL) || (city->Body.CivID == leader_or_null->ID))) {
				bool any_relevant_mills = false;
				for (int n = 0; n < is->current_config.count_mills; n++) {
					struct mill * mill = &is->current_config.mills[n];
					Improvement * mill_improv = &p_bic_data->Improvements[mill->improv_id];
					if ((mill->flags & MF_YIELDS) &&
					    ((mill_improv->Resource1ID >= 0) || (mill_improv->Resource2ID >= 0)) &&
					    has_active_building (city, mill->improv_id)) {
						any_relevant_mills = true;
						break;
					}
				}
				if (any_relevant_mills)
					City_recompute_yields_and_happiness (city);
			}
		}
}


int
compare_mill_tiles (void const * vp_a, void const * vp_b)
{
	struct mill_tile const * a = vp_a, * b = vp_b;
	return a->mill->resource_id - b->mill->resource_id;
}

void __fastcall
patch_Trade_Net_recompute_resources (Trade_Net * this, int edx, bool skip_popups)
{
	is->is_computing_resource_access = true;
	int extra_resource_count = not_below (0, p_bic_data->ResourceTypeCount - 32);
	int ints_per_city = 1 + extra_resource_count/32;
	memset (is->extra_available_resources, 0, is->extra_available_resources_capacity * ints_per_city * sizeof (unsigned));

	// Assemble list of mill tiles
	is->count_mill_tiles = 0;
	if (p_cities->Cities != NULL)
		for (int city_index = 0; city_index <= p_cities->LastIndex; city_index++) {
			City * city = get_city_ptr (city_index);
			if (city != NULL)
				for (int n = 0; n < is->current_config.count_mills; n++) {
					struct mill * mill = &is->current_config.mills[n];
					if (((mill->flags & MF_LOCAL) == 0) &&
					    has_active_building (city, mill->improv_id) &&
					    can_generate_resource (city->Body.CivID, mill)) {
						reserve (sizeof is->mill_tiles[0],
							 (void **)&is->mill_tiles,
							 &is->mill_tiles_capacity,
							 is->count_mill_tiles);
						is->mill_tiles[is->count_mill_tiles++] = (struct mill_tile) {
							.tile = tile_at (city->Body.X, city->Body.Y),
							.city = city,
							.mill = mill
						};
					}
				}
		}
	qsort (is->mill_tiles, is->count_mill_tiles, sizeof is->mill_tiles[0], compare_mill_tiles);

	is->got_mill_tile = NULL;
	is->saved_tile_count = p_bic_data->Map.TileCount;
	p_bic_data->Map.TileCount += is->count_mill_tiles;
	Trade_Net_recompute_resources (this, __, skip_popups);

	// Restore the tile count if necessary. It may have already been restored by patch_Map_Renderer_m71_Draw_Tiles. This happens when the call to
	// recompute_resources above opens a popup message about connecting a resource for the first time, which triggers redraw of the map.
	if (is->saved_tile_count >= 0) {
		p_bic_data->Map.TileCount = is->saved_tile_count;
		is->saved_tile_count = -1;
	}

	recompute_mill_yields_after_resource_change (NULL);
	is->is_computing_resource_access = false;
}

Tile *
get_mill_tile (int index)
{
	struct mill_tile * mt = &is->mill_tiles[index];
	is->got_mill_tile = mt;
	return mt->tile;
}

Tile * __fastcall patch_Map_get_tile_when_recomputing_resources_1 (Map * map, int edx, int index) { return (index < is->saved_tile_count) ? Map_get_tile (map, __, index) : get_mill_tile (index - is->saved_tile_count); }
Tile * __fastcall patch_Map_get_tile_when_recomputing_resources_2 (Map * map, int edx, int index) { return (index < is->saved_tile_count) ? Map_get_tile (map, __, index) : get_mill_tile (index - is->saved_tile_count); }
Tile * __fastcall patch_Map_get_tile_when_recomputing_resources_3 (Map * map, int edx, int index) { return (index < is->saved_tile_count) ? Map_get_tile (map, __, index) : get_mill_tile (index - is->saved_tile_count); }
Tile * __fastcall patch_Map_get_tile_when_recomputing_resources_4 (Map * map, int edx, int index) { return (index < is->saved_tile_count) ? Map_get_tile (map, __, index) : get_mill_tile (index - is->saved_tile_count); }
Tile * __fastcall patch_Map_get_tile_when_recomputing_resources_5 (Map * map, int edx, int index) { return (index < is->saved_tile_count) ? Map_get_tile (map, __, index) : get_mill_tile (index - is->saved_tile_count); }

int __fastcall
patch_Tile_get_visible_resource_when_recomputing (Tile * tile, int edx, int civ_id)
{
	if (is->got_mill_tile == NULL)
		return Tile_get_resource_visible_to (tile, __, civ_id);
	else {
		struct mill_tile * mt = is->got_mill_tile;
		is->got_mill_tile = NULL;
		if (has_resources_required_by_building (mt->city, mt->mill->improv_id))
			return mt->mill->resource_id;
		else
			return -1;
	}
}

int WINAPI
patch_MessageBoxA (HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	if (is->current_config.suppress_hypertext_links_exceeded_popup &&
	    (strcmp (lpText, "Maximum hypertext links exceeded!") == 0))
		return IDOK;
	else
		return MessageBoxA (hWnd, lpText, lpCaption, uType);
}

char * __fastcall
do_capture_modified_gold_trade (TradeOffer * trade_offer, int edx, int val, char * str, unsigned base)
{
	is->modifying_gold_trade = trade_offer;
	return print_int (val, str, base);
}

struct register_set {
	int edi, esi, ebp, esp, ebx, edx, ecx, eax;
};

// Return 1 to allow the candidate unit to exert ZoC, 0 to exclude it. A pointer to the candidate is in esi.
int __stdcall
filter_zoc_candidate (struct register_set * reg)
{
	Unit * candidate = (Unit *)reg->esi,
	     * defender = is->zoc_defender;

	UnitType * candidate_type = &p_bic_data->UnitTypes[candidate->Body.UnitTypeID],
		 * defender_type  = &p_bic_data->UnitTypes[defender ->Body.UnitTypeID];

	enum UnitTypeClasses candidate_class = candidate_type->Unit_Class,
		             defender_class  = defender_type ->Unit_Class;

	bool lethal = ((is->current_config.special_zone_of_control_rules & SZOCR_LETHAL) != 0) && (! is->temporarily_disallow_lethal_zoc);
	bool aerial     = (is->current_config.special_zone_of_control_rules & SZOCR_AERIAL    ) != 0,
	     amphibious = (is->current_config.special_zone_of_control_rules & SZOCR_AMPHIBIOUS) != 0;

	// Exclude air units if aerial ZoC is not enabled and exclude land-to-sea & sea-to-land ZoC if amphibious is not enabled
	if ((! aerial) && (candidate_class == UTC_Air))
		return 0;
	if ((! amphibious) &&
	    (((candidate_class == UTC_Land) && (defender_class == UTC_Sea )) ||
	     ((candidate_class == UTC_Sea ) && (defender_class == UTC_Land))))
		return 0;

	// In case of cross-domain ZoC, filter out units with zero bombard strength or range. They can't use their attack strength in this case, so
	// without bombard they can be ruled out. Don't forget units may have non-zero bombard strength and zero range for defensive bombard.
	int range = (candidate_class != UTC_Air) ? candidate_type->Bombard_Range : candidate_type->OperationalRange;
	if ((candidate_class != defender_class) && ((candidate_type->Bombard_Strength <= 0) || (range <= 0)))
		return 0;

	// Require lethal config option & lethal bombard against one HP defender
	if ((Unit_get_max_hp (defender) - defender->Body.Damage <= 1) &&
	    ((! lethal) ||
	     ((defender_class == UTC_Sea) && ! UnitType_has_ability (candidate_type, __, UTA_Lethal_Sea_Bombardment)) ||
	     ((defender_class != UTC_Sea) && ! UnitType_has_ability (candidate_type, __, UTA_Lethal_Land_Bombardment))))
		return 0;

	// Air units require the bombing action to perform ZoC
	if ((candidate_class == UTC_Air) && ! (candidate_type->Air_Missions & UCV_Bombing))
		return 0;

	// Exclude land units in transports
	if (candidate_class == UTC_Land) {
		Unit * container = get_unit_ptr (candidate->Body.Container_Unit);
		if ((container != NULL) && ! UnitType_has_ability (&p_bic_data->UnitTypes[container->Body.UnitTypeID], __, UTA_Army))
			return 0;
	}

	return 1;
}

enum branch_kind { BK_CALL, BK_JUMP };

byte *
emit_branch (enum branch_kind kind, byte * cursor, void const * target)
{
	int offset = (int)target - ((int)cursor + 5);
	*cursor++ = (kind == BK_CALL) ? 0xE8 : 0xE9;
	return int_to_bytes (cursor, offset);
}

// Just calls VirtualProtect and displays an error message if it fails. Made for use by the WITH_MEM_PROTECTION macro.
bool
check_virtual_protect (LPVOID addr, SIZE_T size, DWORD flags, PDWORD old_protect)
{
	if (VirtualProtect (addr, size, flags, old_protect))
		return true;
	else {
		char err_msg[1000];
		snprintf (err_msg, sizeof err_msg, "VirtualProtect failed! Args:\n  Address: 0x%p\n  Size: %d\n  Flags: 0x%x", addr, size, flags);
		err_msg[(sizeof err_msg) - 1] = '\0';
		MessageBoxA (NULL, err_msg, NULL, MB_ICONWARNING);
		return false;
	}
}

#define WITH_MEM_PROTECTION(addr, size, flags)				                \
	for (DWORD old_protect, unused, iter_count = 0;			                \
	     (iter_count == 0) && check_virtual_protect (addr, size, flags, &old_protect); \
	     VirtualProtect (addr, size, old_protect, &unused), iter_count++)

void __fastcall adjust_sliders_preproduction (Leader * this);

struct nopified_area {
	int size;
	byte original_contents[];
};

// Replaces an area of code with no-ops. The original contents of the area are saved and can be restored with restore_area. This method assumes that
// the necessary memory protection has already been set on the area, specifically that it can be written to. The method will do nothing if the area
// has already been nopified with the same size. It's an error to re-nopify an address with a different size or overlap two nopified areas.
void
nopify_area (byte * addr, int size)
{
	struct nopified_area * na;
	if (itable_look_up (&is->nopified_areas, (int)addr, (int *)&na)) {
		if (na->size != 0) {
			if (na->size != size) {
				char s[200];
				snprintf (s, sizeof s, "Nopification conflict: address %p was already nopified with size %d, conflicting with new size %d.", addr, na->size, size);
				s[(sizeof s) - 1] = '\0';
				pop_up_in_game_error (s);
			}
			return;
		}
	} else {
		na = malloc (size + sizeof *na);
		itable_insert (&is->nopified_areas, (int)addr, (int)na);
	}
	na->size = size;
	memcpy (&na->original_contents, addr, size);
	memset (addr, 0x90, size);
}

// De-nopifies an area, restoring the original contents. Does nothing if the area hasn't been nopified. Assumes the appropriate memory protection has
// already been set.
void
restore_area (byte * addr)
{
	struct nopified_area * na;
	if (itable_look_up (&is->nopified_areas, (int)addr, (int *)&na)) {
		memcpy (addr, &na->original_contents, na->size);
		na->size = 0;
	}
}

bool
is_area_nopified (byte * addr)
{
	struct nopified_area * na;
	return itable_look_up (&is->nopified_areas, (int)addr, (int *)&na) && (na->size > 0);
}

// Nopifies or restores an area depending on if yes_or_no is 1 or 0. Sets the necessary memory protections.
void
set_nopification (int yes_or_no, byte * addr, int size)
{
	WITH_MEM_PROTECTION (addr, size, PAGE_EXECUTE_READWRITE) {
		if (yes_or_no)
			nopify_area (addr, size);
		else
			restore_area (addr);
	}
}

void
apply_machine_code_edits (struct c3x_config const * cfg)
{
	DWORD old_protect, unused;

	// Allow stealth attack against single unit
	WITH_MEM_PROTECTION (ADDR_STEALTH_ATTACK_TARGET_COUNT_CHECK, 1, PAGE_EXECUTE_READWRITE)
		*(byte *)ADDR_STEALTH_ATTACK_TARGET_COUNT_CHECK = cfg->allow_stealth_attack_against_single_unit ? 0 : 1;

	// Enable small wonders providing free buildings
	WITH_MEM_PROTECTION (ADDR_RECOMPUTE_AUTO_IMPROVS_FILTER, 10, PAGE_EXECUTE_READWRITE) {
		byte normal[8] = {0x83, 0xE1, 0x04, 0x80, 0xF9, 0x04, 0x0F, 0x85}; // and ecx,  4; cmp ecx, 4; jnz [offset]
		byte modded[8] = {0x83, 0xE1, 0x0C, 0x80, 0xF9, 0x00, 0x0F, 0x84}; // and ecx, 12; cmp ecx, 0; jz  [offset]
		for (int n = 0; n < 8; n++)
			((byte *)ADDR_RECOMPUTE_AUTO_IMPROVS_FILTER)[n] = cfg->enable_free_buildings_from_small_wonders ? modded[n] : normal[n];
	}

	// Bypass artillery in city check
	// replacing 0x74 (= jump if [city ptr is] zero) with 0xEB (= uncond. jump)
	WITH_MEM_PROTECTION (ADDR_CHECK_ARTILLERY_IN_CITY, 1, PAGE_EXECUTE_READWRITE)
		*(byte *)ADDR_CHECK_ARTILLERY_IN_CITY = cfg->use_offensive_artillery_ai ? 0xEB : 0x74;
	
	// Remove unit limit
	// replacing 0x7C (= jump if less than [unit limit]) with 0xEB (= uncond. jump)
	WITH_MEM_PROTECTION (ADDR_UNIT_COUNT_CHECK, 1, PAGE_EXECUTE_READWRITE)
		*(byte *)ADDR_UNIT_COUNT_CHECK = cfg->remove_unit_limit ? 0xEB : 0x7C;

	// Remove era limit
	// replacing 0x74 (= jump if zero [after cmp'ing era count with 4]) with 0xEB
	WITH_MEM_PROTECTION (ADDR_ERA_COUNT_CHECK, 1, PAGE_EXECUTE_READWRITE)
		*(byte *)ADDR_ERA_COUNT_CHECK = cfg->remove_era_limit ? 0xEB : 0x74;

	// Fix science age bug
	// Similar in nature to the sub bug, the function that measures a city's research output accepts a flag that determines whether or not it
	// takes science ages into account. It's mistakenly not set by the code that gathers all research points to increment tech progress (but it
	// is set elsewhere in code for the interface). The patch simply sets this flag.
	WITH_MEM_PROTECTION (ADDR_SCIENCE_AGE_BUG_PATCH, 1, PAGE_EXECUTE_READWRITE)
		*(byte *)ADDR_SCIENCE_AGE_BUG_PATCH = cfg->patch_science_age_bug ? 1 : 0;

	// Pedia pink line bug fix
	// The size of the pedia background texture is hard-coded into the EXE and in the base game it's one pixel too small. This shows up in game as
	// a one pixel wide pink line along the right edge of the civilopedia. This patch simply increases the texture width by one.
	WITH_MEM_PROTECTION (ADDR_PEDIA_TEXTURE_BUG_PATCH, 1, PAGE_EXECUTE_READWRITE)
		*(byte *)ADDR_PEDIA_TEXTURE_BUG_PATCH = cfg->patch_pedia_texture_bug ? 0xA6 : 0xA5;

	// Fix for houseboat bug
	// See my posts on CFC for an explanation of the bug and its fix:
	// https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/page-10#post-16084386
	// https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/page-10#post-16085242
	WITH_MEM_PROTECTION (ADDR_HOUSEBOAT_BUG_PATCH, ADDR_HOUSEBOAT_BUG_PATCH_END - ADDR_HOUSEBOAT_BUG_PATCH, PAGE_EXECUTE_READWRITE) {
		if (cfg->patch_houseboat_bug) {
			nopify_area (ADDR_HOUSEBOAT_BUG_PATCH, ADDR_HOUSEBOAT_BUG_PATCH_END - ADDR_HOUSEBOAT_BUG_PATCH);
			byte * cursor = ADDR_HOUSEBOAT_BUG_PATCH;
			*cursor++ = 0x50; // push eax
			int call_offset = (int)&tile_at_city_or_null - ((int)cursor + 5);
			*cursor++ = 0xE8; // call
			cursor = int_to_bytes (cursor, call_offset);
		} else
			restore_area (ADDR_HOUSEBOAT_BUG_PATCH);
	}

	// NoRaze
	WITH_MEM_PROTECTION (ADDR_AUTORAZE_BYPASS, 2, PAGE_EXECUTE_READWRITE) {
		byte normal[2] = {0x0F, 0x85}; // jnz
		byte bypass[2] = {0x90, 0xE9}; // nop, jmp
		for (int n = 0; n < 2; n++)
			((byte *)ADDR_AUTORAZE_BYPASS)[n] = cfg->prevent_autorazing ? bypass[n] : normal[n];
	}

	// Overwrite the instruction(s) where the AI's production choosing code compares the value of what it's currently considering to the best
	// option so far. This is done twice since improvements and units are handled in separate loops. The instr(s) are overwritten with a jump to
	// an "airlock", which is a bit of code that wraps the call to intercept_consideration. The contents of the airlocks are prepared by the
	// patcher in init_consideration_airlocks.
	// TODO: This instruction replacement could be done in the patcher too and that might be a better place for it. Think about this.
	for (int n = 0; n < 2; n++) {
		void * addr_intercept = (n == 0) ? ADDR_INTERCEPT_AI_IMPROV_VALUE    : ADDR_INTERCEPT_AI_UNIT_VALUE;
		void * addr_airlock   = (n == 0) ? ADDR_IMPROV_CONSIDERATION_AIRLOCK : ADDR_UNIT_CONSIDERATION_AIRLOCK;

		WITH_MEM_PROTECTION (addr_intercept, AI_CONSIDERATION_INTERCEPT_LEN, PAGE_EXECUTE_READWRITE) {
			byte * cursor = addr_intercept;

			// write jump to airlock
			*cursor++ = 0xE9;
			int offset = (int)addr_airlock - ((int)addr_intercept + 5);
			cursor = int_to_bytes (cursor, offset);

			// fill the rest of the space with NOPs
			while (cursor < (byte *)addr_intercept + AI_CONSIDERATION_INTERCEPT_LEN)
				*cursor++ = 0x90; // nop
		}
	}

	// Overwrite instruction that sets bits in City.Body.Available_Resources with a jump to the airlock
	WITH_MEM_PROTECTION (ADDR_INTERCEPT_SET_RESOURCE_BIT, 6, PAGE_EXECUTE_READWRITE) {
		byte * cursor = ADDR_INTERCEPT_SET_RESOURCE_BIT;
		if (cfg->patch_phantom_resource_bug) {
			*cursor++ = 0xE9;
			int offset = (int)ADDR_SET_RESOURCE_BIT_AIRLOCK - ((int)ADDR_INTERCEPT_SET_RESOURCE_BIT + 5);
			cursor = int_to_bytes (cursor, offset);
			*cursor++ = 0x90; // nop
		} else {
			byte original[6] = {0x09, 0xB0, 0x9C, 0x00, 0x00, 0x00}; // or dword ptr [eax+0x9C], esi
			for (int n = 0; n < 6; n++)
				cursor[n] = original[n];
		}
	}

	// Enlarge the mask that's applied to p_bic_data->Map.TileCount in the loop over lines in Trade_Net::recompute_resources. This lets us loop
	// over more than 0xFFFF tiles.
	WITH_MEM_PROTECTION (ADDR_RESOURCE_TILE_COUNT_MASK, 1, PAGE_EXECUTE_READWRITE) {
		*(byte *)ADDR_RESOURCE_TILE_COUNT_MASK = (cfg->count_mills > 0) ? 0xFF : 0x00;
	}
	// Similarly, enlarge the cmp instruction that jumps over the entire loop when the tile count is zero. Do this by simply removing the operand
	// override prefix byte (0x66), overwriting it with a nop (0x90). This converts the instruction "cmp word ptr [bic_data.Map.TileCount], di"
	// into "nop; cmp dword ptr [bic_data.Map.TileCount], edi"
	WITH_MEM_PROTECTION (ADDR_RESOURCE_TILE_COUNT_ZERO_COMPARE, 1, PAGE_EXECUTE_READWRITE) {
		*(byte *)ADDR_RESOURCE_TILE_COUNT_ZERO_COMPARE = 0x90;
	}

	byte * addr_turn_metalimits[] = {ADDR_TURN_METALIMIT_1, ADDR_TURN_METALIMIT_2, ADDR_TURN_METALIMIT_3, ADDR_TURN_METALIMIT_4,
					 ADDR_TURN_METALIMIT_5, ADDR_TURN_METALIMIT_6, ADDR_TURN_METALIMIT_7};
	for (int n = 0; n < ARRAY_LEN (addr_turn_metalimits); n++) {
		byte * addr = addr_turn_metalimits[n];
		WITH_MEM_PROTECTION (addr, 4, PAGE_EXECUTE_READWRITE) {
			int_to_bytes (addr, cfg->remove_cap_on_turn_limit ? 1000000 : 1000);
		}
	}

	// Overwrite the human-ness test and call to Leader::ai_adjust_sliders that happens during the preproduction player update method. The new
	// code calls adjust_sliders_preproduction for all players.
	WITH_MEM_PROTECTION (ADDR_AI_PREPRODUCTION_SLIDER_ADJUSTMENT, 9, PAGE_EXECUTE_READWRITE) {
		byte * cursor = ADDR_AI_PREPRODUCTION_SLIDER_ADJUSTMENT;
		*cursor++ = 0x8B; *cursor++ = 0xCE; // mov ecx, esi
		cursor = emit_branch (BK_CALL, cursor, adjust_sliders_preproduction);
		for (; cursor < ADDR_AI_PREPRODUCTION_SLIDER_ADJUSTMENT + 9; cursor++)
			*cursor = 0x90; // nop
	}

	// Set up a special intercept of the base game's calls to print_int in order to grab a pointer to a gold TradeOffer object being modified. The
	// calls to print_int happen in the context of creating the default text to be placed in the set-gold-amount popup. Conveniently, a pointer to
	// the TradeOffer object is always stored in register ecx when this call happens. It's not easily accessible since print_int uses the cdecl
	// convention so we must use an airlock-like thing to effectively convert the calling convention to fastcall. The first part of this code
	// simply replaces the call to print_int with a call to the airlock-like thing, and the second part initializes its contents.
	byte * addr_print_gold_amounts[] = {ADDR_PRINT_GOLD_AMOUNT_1, ADDR_PRINT_GOLD_AMOUNT_2};
	for (int n = 0; n < ARRAY_LEN (addr_print_gold_amounts); n++) {
		byte * addr = addr_print_gold_amounts[n];
		WITH_MEM_PROTECTION (addr, 5, PAGE_EXECUTE_READWRITE)
			emit_branch (BK_CALL, addr, ADDR_CAPTURE_MODIFIED_GOLD_TRADE);
	}
	WITH_MEM_PROTECTION (ADDR_CAPTURE_MODIFIED_GOLD_TRADE, 32, PAGE_EXECUTE_READWRITE) {
		byte * cursor = ADDR_CAPTURE_MODIFIED_GOLD_TRADE;

		// Repush all of the arguments to print_int onto the stack, they will be consumed by do_capture_modified_gold_trade since it's
		// fastcall. The original args don't need to be removed b/c we're replacing a cdecl function so that's the caller's
		// responsibility. The TradeOffer pointer is already in ECX, so that's fine as long as we don't touch that register.
		byte repush[] = {0xFF, 0x74, 0x24, 0x0C}; // push [esp+0xC]
		for (int n = 0; n < 3; n++)
			for (int k = 0; k < ARRAY_LEN (repush); k++)
				*cursor++ = repush[k];

		cursor = emit_branch (BK_CALL, cursor, do_capture_modified_gold_trade); // call do_capture_modified_gold_trade
		*cursor++ = 0xC3; // ret
	}

	// Edit branch in capture_city to never run code for barbs, this allows barbs to capture cities
	WITH_MEM_PROTECTION (ADDR_CAPTURE_CITY_BARB_BRANCH, 2, PAGE_EXECUTE_READWRITE) {
		byte normal[2] = {0x0F, 0x85}; // jnz
		byte bypass[2] = {0x90, 0xE9}; // nop, jmp
		for (int n = 0; n < 2; n++)
			((byte *)ADDR_CAPTURE_CITY_BARB_BRANCH)[n] = cfg->enable_city_capture_by_barbarians ? bypass[n] : normal[n];
	}

	// After the production phase is done for the barb player, there are two jump instructions skipping the production code for civs. Replacing
	// those jumps lets us run the civ production code for the barbs as well.
	WITH_MEM_PROTECTION (ADDR_PROD_PHASE_BARB_DONE_NO_SPAWN_JUMP, 6, PAGE_EXECUTE_READWRITE) {
		if (cfg->enable_city_capture_by_barbarians) {
			nopify_area (ADDR_PROD_PHASE_BARB_DONE_NO_SPAWN_JUMP, 6);
			byte jump_to_civ[6] = {0x0F, 0x8D, 0x0C, 0x00, 0x00, 0x00}; // jge +0x12
			for (int n = 0; n < 6; n++)
				ADDR_PROD_PHASE_BARB_DONE_NO_SPAWN_JUMP[n] = jump_to_civ[n];
		} else
			restore_area (ADDR_PROD_PHASE_BARB_DONE_NO_SPAWN_JUMP);
	}
	set_nopification (cfg->enable_city_capture_by_barbarians, ADDR_PROD_PHASE_BARB_DONE_JUMP, 5);

	for (int domain = 0; domain < 2; domain++) {
		byte * addr_skip    = (domain == 0) ? ADDR_SKIP_LAND_UNITS_FOR_SEA_ZOC : ADDR_SKIP_SEA_UNITS_FOR_LAND_ZOC,
		     * addr_airlock = (domain == 0) ? ADDR_SEA_ZOC_FILTER_AIRLOCK      : ADDR_LAND_ZOC_FILTER_AIRLOCK;

		WITH_MEM_PROTECTION (addr_skip, 6, PAGE_EXECUTE_READWRITE) {
			if ((cfg->special_zone_of_control_rules != 0) && ! is_area_nopified (addr_skip)) {
				byte * original_target = addr_skip + 6 + int_from_bytes (addr_skip + 2); // target addr of jump instr we're replacing
				nopify_area (addr_skip, 6);

				// Initialize airlock. The airlock preserves all registers and calls filter_zoc_candidate then either follows or skips the
				// original jump depending on what it returns. If zero is returned, follows the jump, skipping a bunch of code and filtering
				// out the unit as a candidate for ZoC.
				WITH_MEM_PROTECTION (addr_airlock, INLEAD_SIZE, PAGE_READWRITE) {
					byte * cursor = addr_airlock;
					*cursor++ = 0x60; // pusha
					*cursor++ = 0x54; // push esp
					cursor = emit_branch (BK_CALL, cursor, filter_zoc_candidate);
					*cursor++ = 0x83; *cursor++ = 0xF8; *cursor++ = 0x01; // cmp eax, 1
					*cursor++ = 0x75; *cursor++ = 0x06; // jne 6
					*cursor++ = 0x61; // popa
					cursor = emit_branch (BK_JUMP, cursor, addr_skip + 6);
					*cursor++ = 0x61; // popa
					cursor = emit_branch (BK_JUMP, cursor, original_target);
				}

				// Write jump to airlock
				emit_branch (BK_JUMP, addr_skip, addr_airlock);
			} else if (cfg->special_zone_of_control_rules == 0)
				restore_area (addr_skip);
		}
	}

	set_nopification ( cfg->special_zone_of_control_rules                 != 0, ADDR_ZOC_CHECK_ATTACKER_ANIM_FIELD_111, 6);
	set_nopification ((cfg->special_zone_of_control_rules & SZOCR_LETHAL) != 0, ADDR_SKIP_ZOC_FOR_ONE_HP_LAND_UNIT    , 6);
	set_nopification ((cfg->special_zone_of_control_rules & SZOCR_LETHAL) != 0, ADDR_SKIP_ZOC_FOR_ONE_HP_SEA_UNIT     , 6);

	WITH_MEM_PROTECTION (ADDR_LUXURY_BOX_ROW_HEIGHT, 4, PAGE_EXECUTE_READWRITE) {
		byte normal [4] = {0x8B, 0x44, 0x24, LUXURY_BOX_ROW_HEIGHT_STACK_OFFSET}; // mov eax, dword ptr [esp + LUXURY_BOX_ROW_HEIGHT_STACK_OFFSET]
		byte compact[4] = {0x31, 0xC0, 0xB0, 0x10}; // xor eax, eax; mov al, 0x10
		for (int n = 0; n < 4; n++)
			ADDR_LUXURY_BOX_ROW_HEIGHT[n] = cfg->compact_luxury_display_on_city_screen ? compact[n] : normal[n];
	}
}

void
patch_init_floating_point ()
{
	init_floating_point ();

	// NOTE: At this point the program is done with the CRT initialization stuff and will start calling constructors for global
	// objects as soon as this function returns. This is a good place to inject code that will run at program start.

	// Specify metadata about all boolean options on the mod config. We'll use this info to set up the table of offsets (for easy parsing) and to
	// fill out the base config.
	struct boolean_config_option {
		char * name;
		bool base_val;
		int offset;
	} boolean_config_options[] = {
		{"enable_stack_bombard"                                , true , offsetof (struct c3x_config, enable_stack_bombard)},
		{"enable_disorder_warning"                             , true , offsetof (struct c3x_config, enable_disorder_warning)},
		{"allow_stealth_attack_against_single_unit"            , false, offsetof (struct c3x_config, allow_stealth_attack_against_single_unit)},
		{"show_detailed_city_production_info"                  , true , offsetof (struct c3x_config, show_detailed_city_production_info)},
		{"enable_free_buildings_from_small_wonders"            , true , offsetof (struct c3x_config, enable_free_buildings_from_small_wonders)},
		{"enable_stack_unit_commands"                          , true , offsetof (struct c3x_config, enable_stack_unit_commands)},
		{"skip_repeated_tile_improv_replacement_asks"          , true , offsetof (struct c3x_config, skip_repeated_tile_improv_replacement_asks)},
		{"autofill_best_gold_amount_when_trading"              , true , offsetof (struct c3x_config, autofill_best_gold_amount_when_trading)},
		{"disallow_founding_next_to_foreign_city"              , true , offsetof (struct c3x_config, disallow_founding_next_to_foreign_city)},
		{"enable_trade_screen_scroll"                          , true , offsetof (struct c3x_config, enable_trade_screen_scroll)},
		{"group_units_on_right_click_menu"                     , true , offsetof (struct c3x_config, group_units_on_right_click_menu)},
		{"gray_out_units_on_menu_with_no_remaining_moves"      , true , offsetof (struct c3x_config, gray_out_units_on_menu_with_no_remaining_moves)},
		{"put_movement_icons_on_units_on_menu"                 , true , offsetof (struct c3x_config, put_movement_icons_on_units_on_menu)},
		{"describe_states_of_units_on_menu"                    , true , offsetof (struct c3x_config, describe_states_of_units_on_menu)},
		{"show_golden_age_turns_remaining"                     , true , offsetof (struct c3x_config, show_golden_age_turns_remaining)},
		{"show_zoc_attacks_from_mid_stack"                     , true , offsetof (struct c3x_config, show_zoc_attacks_from_mid_stack)},
		{"cut_research_spending_to_avoid_bankruptcy"           , true , offsetof (struct c3x_config, cut_research_spending_to_avoid_bankruptcy)},
		{"dont_pause_for_love_the_king_messages"               , true , offsetof (struct c3x_config, dont_pause_for_love_the_king_messages)},
		{"reverse_specialist_order_with_shift"                 , true , offsetof (struct c3x_config, reverse_specialist_order_with_shift)},
		{"dont_give_king_names_in_non_regicide_games"          , true , offsetof (struct c3x_config, dont_give_king_names_in_non_regicide_games)},
		{"no_elvis_easter_egg"                                 , false, offsetof (struct c3x_config, no_elvis_easter_egg)},
		{"disable_worker_automation"                           , false, offsetof (struct c3x_config, disable_worker_automation)},
		{"enable_land_sea_intersections"                       , false, offsetof (struct c3x_config, enable_land_sea_intersections)},
		{"disallow_trespassing"                                , false, offsetof (struct c3x_config, disallow_trespassing)},
		{"show_detailed_tile_info"                             , true , offsetof (struct c3x_config, show_detailed_tile_info)},
		{"warn_about_unrecognized_names"                       , true , offsetof (struct c3x_config, warn_about_unrecognized_names)},
		{"enable_ai_production_ranking"                        , true , offsetof (struct c3x_config, enable_ai_production_ranking)},
		{"enable_ai_city_location_desirability_display"        , true , offsetof (struct c3x_config, enable_ai_city_location_desirability_display)},
		{"zero_corruption_when_off"                            , true , offsetof (struct c3x_config, zero_corruption_when_off)},
		{"disallow_land_units_from_affecting_water_tiles"      , true , offsetof (struct c3x_config, disallow_land_units_from_affecting_water_tiles)},
		{"dont_end_units_turn_after_airdrop"                   , false, offsetof (struct c3x_config, dont_end_units_turn_after_airdrop)},
		{"allow_airdrop_without_airport"                       , false, offsetof (struct c3x_config, allow_airdrop_without_airport)},
		{"enable_negative_pop_pollution"                       , true , offsetof (struct c3x_config, enable_negative_pop_pollution)},
		{"allow_defensive_retreat_on_water"                    , false, offsetof (struct c3x_config, allow_defensive_retreat_on_water)},
		{"promote_forbidden_palace_decorruption"               , false, offsetof (struct c3x_config, promote_forbidden_palace_decorruption)},
		{"allow_military_leaders_to_hurry_wonders"             , false, offsetof (struct c3x_config, allow_military_leaders_to_hurry_wonders)},
		{"aggressively_penalize_bankruptcy"                    , false, offsetof (struct c3x_config, aggressively_penalize_bankruptcy)},
		{"no_penalty_exception_for_agri_fresh_water_city_tiles", false, offsetof (struct c3x_config, no_penalty_exception_for_agri_fresh_water_city_tiles)},
		{"use_offensive_artillery_ai"                          , true , offsetof (struct c3x_config, use_offensive_artillery_ai)},
		{"dont_escort_unflagged_units"                         , false, offsetof (struct c3x_config, dont_escort_unflagged_units)},
		{"replace_leader_unit_ai"                              , true , offsetof (struct c3x_config, replace_leader_unit_ai)},
		{"fix_ai_army_composition"                             , true , offsetof (struct c3x_config, fix_ai_army_composition)},
		{"enable_pop_unit_ai"                                  , true , offsetof (struct c3x_config, enable_pop_unit_ai)},
		{"remove_unit_limit"                                   , true , offsetof (struct c3x_config, remove_unit_limit)},
		{"remove_era_limit"                                    , false, offsetof (struct c3x_config, remove_era_limit)},
		{"remove_cap_on_turn_limit"                            , true , offsetof (struct c3x_config, remove_cap_on_turn_limit)},
		{"patch_submarine_bug"                                 , true , offsetof (struct c3x_config, patch_submarine_bug)},
		{"patch_science_age_bug"                               , true , offsetof (struct c3x_config, patch_science_age_bug)},
		{"patch_pedia_texture_bug"                             , true , offsetof (struct c3x_config, patch_pedia_texture_bug)},
		{"patch_blocked_disembark_freeze"                      , true , offsetof (struct c3x_config, patch_blocked_disembark_freeze)},
		{"patch_houseboat_bug"                                 , true , offsetof (struct c3x_config, patch_houseboat_bug)},
		{"patch_intercept_lost_turn_bug"                       , true , offsetof (struct c3x_config, patch_intercept_lost_turn_bug)},
		{"patch_phantom_resource_bug"                          , true , offsetof (struct c3x_config, patch_phantom_resource_bug)},
		{"patch_maintenance_persisting_for_obsolete_buildings" , true , offsetof (struct c3x_config, patch_maintenance_persisting_for_obsolete_buildings)},
		{"patch_barbarian_diagonal_bug"                        , true , offsetof (struct c3x_config, patch_barbarian_diagonal_bug)},
		{"prevent_autorazing"                                  , false, offsetof (struct c3x_config, prevent_autorazing)},
		{"prevent_razing_by_players"                           , false, offsetof (struct c3x_config, prevent_razing_by_players)},
		{"suppress_hypertext_links_exceeded_popup"             , true , offsetof (struct c3x_config, suppress_hypertext_links_exceeded_popup)},
		{"indicate_non_upgradability_in_pedia"                 , true , offsetof (struct c3x_config, indicate_non_upgradability_in_pedia)},
		{"show_message_after_dodging_sam"                      , true , offsetof (struct c3x_config, show_message_after_dodging_sam)},
		{"include_stealth_attack_cancel_option"                , false, offsetof (struct c3x_config, include_stealth_attack_cancel_option)},
		{"intercept_recon_missions"                            , false, offsetof (struct c3x_config, intercept_recon_missions)},
		{"charge_one_move_for_recon_and_interception"          , false, offsetof (struct c3x_config, charge_one_move_for_recon_and_interception)},
		{"polish_non_air_precision_striking"                   , true , offsetof (struct c3x_config, polish_non_air_precision_striking)},
		{"enable_stealth_attack_via_bombardment"               , false, offsetof (struct c3x_config, enable_stealth_attack_via_bombardment)},
		{"immunize_aircraft_against_bombardment"               , false, offsetof (struct c3x_config, immunize_aircraft_against_bombardment)},
		{"replay_ai_moves_in_hotseat_games"                    , false, offsetof (struct c3x_config, replay_ai_moves_in_hotseat_games)},
		{"restore_unit_directions_on_game_load"                , true , offsetof (struct c3x_config, restore_unit_directions_on_game_load)},
		{"apply_grid_ini_setting_on_game_load"                 , true , offsetof (struct c3x_config, apply_grid_ini_setting_on_game_load)},
		{"charm_flag_triggers_ptw_like_targeting"              , false, offsetof (struct c3x_config, charm_flag_triggers_ptw_like_targeting)},
		{"city_icons_show_unit_effects_not_trade"              , true , offsetof (struct c3x_config, city_icons_show_unit_effects_not_trade)},
		{"ignore_king_ability_for_defense_priority"            , false, offsetof (struct c3x_config, ignore_king_ability_for_defense_priority)},
		{"show_untradable_techs_on_trade_screen"               , false, offsetof (struct c3x_config, show_untradable_techs_on_trade_screen)},
		{"disallow_useless_bombard_vs_airfields"               , true , offsetof (struct c3x_config, disallow_useless_bombard_vs_airfields)},
		{"compact_luxury_display_on_city_screen"               , false, offsetof (struct c3x_config, compact_luxury_display_on_city_screen)},
		{"enable_trade_net_x"                                  , true , offsetof (struct c3x_config, enable_trade_net_x)},
		{"optimize_improvement_loops"                          , true , offsetof (struct c3x_config, optimize_improvement_loops)},
		{"measure_turn_times"                                  , false, offsetof (struct c3x_config, measure_turn_times)},
		{"enable_city_capture_by_barbarians"                   , false, offsetof (struct c3x_config, enable_city_capture_by_barbarians)},
		{"share_visibility_in_hoseat"                          , false, offsetof (struct c3x_config, share_visibility_in_hoseat)},
		{"allow_precision_strikes_against_tile_improvements"   , false, offsetof (struct c3x_config, allow_precision_strikes_against_tile_improvements)},
		{"dont_end_units_turn_after_bombarding_barricade"      , false, offsetof (struct c3x_config, dont_end_units_turn_after_bombarding_barricade)},
		{"remove_land_artillery_target_restrictions"           , false, offsetof (struct c3x_config, remove_land_artillery_target_restrictions)},
		{"allow_bombard_of_other_improvs_on_occupied_airfield" , false, offsetof (struct c3x_config, allow_bombard_of_other_improvs_on_occupied_airfield)},
		{"show_total_city_count"                               , false, offsetof (struct c3x_config, show_total_city_count)},
		{"strengthen_forbidden_palace_ocn_effect"              , false, offsetof (struct c3x_config, strengthen_forbidden_palace_ocn_effect)},
		{"allow_upgrades_in_any_city"                          , false, offsetof (struct c3x_config, allow_upgrades_in_any_city)},
		{"do_not_generate_volcanos"                            , false, offsetof (struct c3x_config, do_not_generate_volcanos)},
	};

	struct integer_config_option {
		char * name;
		int base_val;
		int offset;
	} integer_config_options[] = {
		{"limit_railroad_movement"            ,     0, offsetof (struct c3x_config, limit_railroad_movement)},
		{"minimum_city_separation"            ,     1, offsetof (struct c3x_config, minimum_city_separation)},
		{"anarchy_length_percent"             ,   100, offsetof (struct c3x_config, anarchy_length_percent)},
		{"ai_multi_city_start"                ,     0, offsetof (struct c3x_config, ai_multi_city_start)},
		{"max_tries_to_place_fp_city"         , 10000, offsetof (struct c3x_config, max_tries_to_place_fp_city)},
		{"ai_research_multiplier"             ,   100, offsetof (struct c3x_config, ai_research_multiplier)},
		{"extra_unit_maintenance_per_shields" ,     0, offsetof (struct c3x_config, extra_unit_maintenance_per_shields)},
		{"ai_build_artillery_ratio"           ,    16, offsetof (struct c3x_config, ai_build_artillery_ratio)},
		{"ai_artillery_value_damage_percent"  ,    50, offsetof (struct c3x_config, ai_artillery_value_damage_percent)},
		{"ai_build_bomber_ratio"              ,    70, offsetof (struct c3x_config, ai_build_bomber_ratio)},
		{"max_ai_naval_escorts"               ,     3, offsetof (struct c3x_config, max_ai_naval_escorts)},
		{"ai_worker_requirement_percent"      ,   150, offsetof (struct c3x_config, ai_worker_requirement_percent)},
	};

	is->kernel32 = (*p_GetModuleHandleA) ("kernel32.dll");
	is->user32   = (*p_GetModuleHandleA) ("user32.dll");
	is->msvcrt   = (*p_GetModuleHandleA) ("msvcrt.dll");

	// Remember the function names here are macros that expand to is->...
	VirtualProtect            = (void *)(*p_GetProcAddress) (is->kernel32, "VirtualProtect");
	CloseHandle               = (void *)(*p_GetProcAddress) (is->kernel32, "CloseHandle");
	CreateFileA               = (void *)(*p_GetProcAddress) (is->kernel32, "CreateFileA");
	GetFileSize               = (void *)(*p_GetProcAddress) (is->kernel32, "GetFileSize");
	ReadFile                  = (void *)(*p_GetProcAddress) (is->kernel32, "ReadFile");
	LoadLibraryA              = (void *)(*p_GetProcAddress) (is->kernel32, "LoadLibraryA");
	FreeLibrary               = (void *)(*p_GetProcAddress) (is->kernel32, "FreeLibrary");
	MultiByteToWideChar       = (void *)(*p_GetProcAddress) (is->kernel32, "MultiByteToWideChar");
	WideCharToMultiByte       = (void *)(*p_GetProcAddress) (is->kernel32, "WideCharToMultiByte");
	GetLastError              = (void *)(*p_GetProcAddress) (is->kernel32, "GetLastError");
	QueryPerformanceCounter   = (void *)(*p_GetProcAddress) (is->kernel32, "QueryPerformanceCounter");
	QueryPerformanceFrequency = (void *)(*p_GetProcAddress) (is->kernel32, "QueryPerformanceFrequency");
	GetLocalTime              = (void *)(*p_GetProcAddress) (is->kernel32, "GetLocalTime");
	MessageBoxA  = (void *)(*p_GetProcAddress) (is->user32, "MessageBoxA");
	is->msimg32  = LoadLibraryA ("Msimg32.dll");
	TransparentBlt = (void *)(*p_GetProcAddress) (is->msimg32, "TransparentBlt");
	snprintf = (void *)(*p_GetProcAddress) (is->msvcrt, "_snprintf");
	malloc   = (void *)(*p_GetProcAddress) (is->msvcrt, "malloc");
	calloc   = (void *)(*p_GetProcAddress) (is->msvcrt, "calloc");
	realloc  = (void *)(*p_GetProcAddress) (is->msvcrt, "realloc");
	free     = (void *)(*p_GetProcAddress) (is->msvcrt, "free");
	strtol   = (void *)(*p_GetProcAddress) (is->msvcrt, "strtol");
	strcmp   = (void *)(*p_GetProcAddress) (is->msvcrt, "strcmp");
	strncmp  = (void *)(*p_GetProcAddress) (is->msvcrt, "strncmp");
	strlen   = (void *)(*p_GetProcAddress) (is->msvcrt, "strlen");
	strncpy  = (void *)(*p_GetProcAddress) (is->msvcrt, "strncpy");
	strcpy   = (void *)(*p_GetProcAddress) (is->msvcrt, "strcpy");
	strdup   = (void *)(*p_GetProcAddress) (is->msvcrt, "_strdup");
	strstr   = (void *)(*p_GetProcAddress) (is->msvcrt, "strstr");
	qsort    = (void *)(*p_GetProcAddress) (is->msvcrt, "qsort");
	memcmp   = (void *)(*p_GetProcAddress) (is->msvcrt, "memcmp");
	memcpy   = (void *)(*p_GetProcAddress) (is->msvcrt, "memcpy");

	// Intercept the game's calls to MessageBoxA. We can't do this through the patcher since that would interfere with the runtime loader.
	WITH_MEM_PROTECTION (p_MessageBoxA, 4, PAGE_READWRITE)
		*p_MessageBoxA = patch_MessageBoxA;

	// Set file path to mod's script.txt
	snprintf (is->mod_script_path, sizeof is->mod_script_path, "%s\\Text\\c3x-script.txt", is->mod_rel_dir);
	is->mod_script_path[(sizeof is->mod_script_path) - 1] = '\0';

	// Fill in base config
	struct c3x_config base_config = {0};
	base_config.land_retreat_rules = RR_STANDARD;
	base_config.sea_retreat_rules  = RR_STANDARD;
	base_config.draw_lines_using_gdi_plus = LDO_WINE;
	for (int n = 0; n < ARRAY_LEN (boolean_config_options); n++)
		*((char *)&base_config + boolean_config_options[n].offset) = boolean_config_options[n].base_val;
	for (int n = 0; n < ARRAY_LEN (integer_config_options); n++)
		*(int *)((byte *)&base_config + integer_config_options[n].offset) = integer_config_options[n].base_val;
	memcpy (&is->base_config, &base_config, sizeof base_config);

	// Load labels
	{
		for (int n = 0; n < COUNT_C3X_LABELS; n++)
			is->c3x_labels[n] = "";

		char labels_path[MAX_PATH];
		snprintf (labels_path, sizeof labels_path, "%s\\Text\\c3x-labels.txt", is->mod_rel_dir);
		labels_path[(sizeof labels_path) - 1] = '\0';
		char * labels_file_contents = file_to_string (labels_path);

		if (labels_file_contents != NULL) {
			char * cursor = labels_file_contents;
			int n = 0;
			while ((n < COUNT_C3X_LABELS) && (*cursor != '\0')) {
				if (*cursor == '\n')
					cursor++;
				else if ((cursor[0] == '\r') && (cursor[1] == '\n'))
					cursor += 2;
				else if (*cursor == ';') {
					while ((*cursor != '\0') && (*cursor != '\n'))
						cursor++;
				} else {
					char * line_start = cursor;
					while ((*cursor != '\0') && (*cursor != '\r') && (*cursor != '\n'))
						cursor++;
					int line_len = cursor - line_start;
					if (NULL != (is->c3x_labels[n] = malloc (line_len + 1))) {
						strncpy (is->c3x_labels[n], line_start, line_len);
						is->c3x_labels[n][line_len] = '\0';
					}
					n++;
				}
			}
			free (labels_file_contents);

		} else {
			char err_msg[500];
			snprintf (err_msg, sizeof err_msg, "Couldn't read labels from %s\nPlease make sure the file exists. If you moved the modded EXE you must move the mod folder after it.", labels_path);
			err_msg[(sizeof err_msg) - 1] = '\0';
			MessageBoxA (NULL, err_msg, NULL, MB_ICONWARNING);
		}
	}

	is->tnx_cache = NULL;
	is->is_computing_city_connections = is->is_computing_resource_access = false;
	is->keep_tnx_cache = false;
	is->is_placing_scenario_things = false;
	is->paused_for_popup = false;
	is->time_spent_paused_during_popup = 0;
	is->time_spent_computing_city_connections = 0;
	is->count_calls_to_recompute_city_connections = 0;

	is->have_job_and_loc_to_skip = 0;

	// Initialize trade screen scroll vars
	is->open_diplo_form_straight_to_trade = 0;
	is->trade_screen_scroll_to_id = -1;
	is->trade_scroll_button_left = is->trade_scroll_button_right = NULL;
	is->trade_scroll_button_images = NULL;
	is->trade_scroll_button_state = IS_UNINITED;
	is->eligible_for_trade_scroll = 0;

	memset (&is->nopified_areas, 0, sizeof is->nopified_areas);

	is->unit_menu_duplicates = NULL;

	is->memo = NULL;
	is->memo_len = 0;
	is->memo_capacity = 0;

	is->city_loc_display_perspective = -1;

	is->aliased_civ_noun_bits = is->aliased_civ_adjective_bits = is->aliased_civ_formal_name_bits = is->aliased_leader_name_bits = is->aliased_leader_title_bits = 0;

	is->extra_available_resources = NULL;
	is->extra_available_resources_capacity = 0;

	memset (is->interceptor_reset_lists, 0, sizeof is->interceptor_reset_lists);

	is->ai_prod_valuations = NULL;
	is->count_ai_prod_valuations = 0;
	is->ai_prod_valuations_capacity = 0;

	is->mill_tiles = NULL;
	is->count_mill_tiles = 0;
	is->mill_tiles_capacity = 0;
	is->saved_tile_count = -1;
	is->mill_input_resource_bits = NULL;

	is->drawing_icons_for_improv_id = -1;

	is->resources_sheet = NULL;

	is->modifying_gold_trade = NULL;

	is->bombard_stealth_target = NULL;
	is->selecting_stealth_target_for_bombard = 0;

	is->map_message_text_override = NULL;

	is->load_file_path_override = NULL;

	is->replay_for_players = 0;

	is->suppress_intro_after_load_popup = 0;

	is->force_barb_activity_for_cities = 0;

	is->dummy_tile = calloc (1, sizeof *is->dummy_tile);

	is->unit_bombard_attacking_tile = NULL;

	is->temporarily_disallow_lethal_zoc = false;
	is->moving_unit_to_adjacent_tile = false;
	is->showing_hotseat_replay = false;
	is->getting_tile_occupier_for_ai_pathfinding = false;

	is->running_on_wine = false; {
		HMODULE ntdll = (*p_GetModuleHandleA) ("ntdll.dll");
		is->running_on_wine = (ntdll != NULL) && ((*p_GetProcAddress) (ntdll, "wine_get_version") != NULL);
	}

	is->gdi_plus.init_state = IS_UNINITED;

	is->water_trade_improvs    = (struct improv_id_list) {0};
	is->air_trade_improvs      = (struct improv_id_list) {0};
	is->combat_defense_improvs = (struct improv_id_list) {0};

	is->unit_display_override = (struct unit_display_override) {-1, -1, -1};

	is->dbe = (struct defensive_bombard_event) {0};

	memset (&is->boolean_config_offsets, 0, sizeof is->boolean_config_offsets);
	for (int n = 0; n < ARRAY_LEN (boolean_config_options); n++)
		stable_insert (&is->boolean_config_offsets, boolean_config_options[n].name, boolean_config_options[n].offset);
	memset (&is->integer_config_offsets, 0, sizeof is->integer_config_offsets);
	for (int n = 0; n < ARRAY_LEN (integer_config_options); n++)
		stable_insert (&is->integer_config_offsets, integer_config_options[n].name, integer_config_options[n].offset);

	memset (&is->unit_type_alt_strategies, 0, sizeof is->unit_type_alt_strategies);
	memset (&is->unit_type_duplicates    , 0, sizeof is->unit_type_duplicates);
	memset (&is->extra_defensive_bombards, 0, sizeof is->extra_defensive_bombards);

	is->unit_type_count_init_bits = 0;
	for (int n = 0; n < 32; n++)
		memset (&is->unit_type_counts[n], 0, sizeof is->unit_type_counts[n]);

	is->penciled_in_upgrades = NULL;
	is->penciled_in_upgrade_count = is->penciled_in_upgrade_capacity = 0;

	is->currently_capturing_city = NULL;

	is->loaded_config_names = NULL;
	reset_to_base_config ();
	apply_machine_code_edits (&is->current_config);
}

void __fastcall
patch_Main_Screen_Form_show_map_message (Main_Screen_Form * this, int edx, int tile_x, int tile_y, char * text_key, bool pause)
{
	WITH_PAUSE_FOR_POPUP {
		Main_Screen_Form_show_map_message (this, __, tile_x, tile_y, text_key, pause);
	}
}

int __cdecl
patch_process_text_for_map_message (char * in, char * out)
{
	if (is->map_message_text_override != NULL) {
		strcpy (out, is->map_message_text_override);
		is->map_message_text_override = NULL;
		return 0;
	} else
		return process_text_snippet (in, out);
}

// Works like show_map_message but takes a bit of text to display instead of a key for script.txt
void
show_map_specific_text (int tile_x, int tile_y, char const * text, bool pause)
{
	is->map_message_text_override = text;
	patch_Main_Screen_Form_show_map_message (p_main_screen_form, __, tile_x, tile_y, "LANDCONQUER", pause); // Use any key here. It will be overridden.
}

void
get_mod_art_path (char const * file_name, char * out_path, int path_buf_size)
{
	char s[1000];
	snprintf (s, sizeof s, "Art\\%s", file_name);
	s[(sizeof s) - 1] = '\0';

	char * scenario_path = BIC_get_asset_path (p_bic_data, __, s, false);
	if (0 != strcmp (scenario_path, s)) // get_asset_path returns its input when the file is not found
		snprintf (out_path, path_buf_size, "%s", scenario_path);
	else
		snprintf (out_path, path_buf_size, "%s\\Art\\%s", is->mod_rel_dir, file_name);
	out_path[path_buf_size - 1] = '\0';
}

void
init_stackable_command_buttons ()
{
	if (is->sc_img_state != IS_UNINITED)
		return;

	PCX_Image pcx;
	PCX_Image_construct (&pcx);
	for (int sc = 0; sc < COUNT_STACKABLE_COMMANDS; sc++)
		for (int n = 0; n < 4; n++)
			Tile_Image_Info_construct (&is->sc_button_image_sets[sc].imgs[n]);

	char temp_path[2*MAX_PATH];

	is->sb_activated_by_button = 0;
	is->sc_img_state = IS_INIT_FAILED;

	char const * filenames[4] = {"StackedNormButtons.pcx", "StackedRolloverButtons.pcx", "StackedHighlightedButtons.pcx", "StackedButtonsAlpha.pcx"};
	for (int n = 0; n < 4; n++) {
		get_mod_art_path (filenames[n], temp_path, sizeof temp_path);
		PCX_Image_read_file (&pcx, __, temp_path, NULL, 0, 0x100, 2);
		if (pcx.JGL.Image == NULL) {
			(*p_OutputDebugStringA) ("[C3X] Failed to load stacked command buttons sprite sheet.");
			for (int sc = 0; sc < COUNT_STACKABLE_COMMANDS; sc++)
				for (int k = 0; k < 4; k++) {
					Tile_Image_Info * tii = &is->sc_button_image_sets[sc].imgs[k];
					tii->vtable->destruct (tii, __, 0);
				}
			pcx.vtable->destruct (&pcx, __, 0);
			return;
		}
		
		for (int sc = 0; sc < COUNT_STACKABLE_COMMANDS; sc++) {
			int x = 32 * sc_button_infos[sc].tile_sheet_column,
			    y = 32 * sc_button_infos[sc].tile_sheet_row;
			Tile_Image_Info_slice_pcx (&is->sc_button_image_sets[sc].imgs[n], __, &pcx, x, y, 32, 32, 1, 0);
		}

		pcx.vtable->clear_JGL (&pcx);
	}

	is->sc_img_state = IS_OK;
	pcx.vtable->destruct (&pcx, __, 0);
}

void
init_disabled_command_buttons ()
{
	if (is->disabled_command_img_state != IS_UNINITED)
		return;

	is->disabled_command_img_state = IS_INIT_FAILED;

	PCX_Image pcx;
	PCX_Image_construct (&pcx);

	char temp_path[2*MAX_PATH];
	get_mod_art_path ("DisabledButtons.pcx", temp_path, sizeof temp_path);
	PCX_Image_read_file (&pcx, __, temp_path, NULL, 0, 0x100, 2);
	if (pcx.JGL.Image == NULL) {
		(*p_OutputDebugStringA) ("[C3X] Failed to load disabled command buttons sprite sheet.");
		pcx.vtable->destruct (&pcx, __, 0);
		return;
	}

	Tile_Image_Info_construct (&is->disabled_build_city_button_img);
	Tile_Image_Info_slice_pcx (&is->disabled_build_city_button_img, __, &pcx, 32*5, 32*2, 32, 32, 1, 0);

	is->disabled_command_img_state = IS_OK;
	pcx.vtable->destruct (&pcx, __, 0);
}

void
deinit_stackable_command_buttons ()
{
	if (is->sc_img_state == IS_OK)
		for (int sc = 0; sc < COUNT_STACKABLE_COMMANDS; sc++)
			for (int n = 0; n < 4; n++) {
				Tile_Image_Info * tii = &is->sc_button_image_sets[sc].imgs[n];
				tii->vtable->destruct (tii, __, 0);
			}
	is->sc_img_state = IS_UNINITED;
}

void
deinit_disabled_command_buttons ()
{
	Tile_Image_Info * tii = &is->disabled_build_city_button_img;
	if (is->disabled_command_img_state == IS_OK)
		tii->vtable->destruct (tii, __, 0);
	memset (tii, 0, sizeof *tii);
	is->disabled_command_img_state = IS_UNINITED;
}

void
init_tile_highlights ()
{
	if (is->tile_highlight_state != IS_UNINITED)
		return;

	PCX_Image pcx;
	PCX_Image_construct (&pcx);

	char temp_path[2*MAX_PATH];

	is->tile_highlight_state = IS_INIT_FAILED;

	snprintf (temp_path, sizeof temp_path, "%s\\Art\\TileHighlights.pcx", is->mod_rel_dir);
	temp_path[(sizeof temp_path) - 1] = '\0';
	PCX_Image_read_file (&pcx, __, temp_path, NULL, 0, 0x100, 2);
	if (pcx.JGL.Image == NULL) {
		(*p_OutputDebugStringA) ("[C3X] Failed to load stacked command buttons sprite sheet.");
		goto cleanup;
	}

	for (int n = 0; n < COUNT_TILE_HIGHLIGHTS; n++)
		Tile_Image_Info_slice_pcx (&is->tile_highlights[n], __, &pcx, 128*n, 0, 128, 64, 1, 0);

	is->tile_highlight_state = IS_OK;
cleanup:
	pcx.vtable->destruct (&pcx, __, 0);
}

void
init_unit_rcm_icons ()
{
	if (is->unit_rcm_icon_state != IS_UNINITED)
		return;
	is->unit_rcm_icon_state = IS_INIT_FAILED;

	PCX_Image pcx;
	PCX_Image_construct (&pcx);

	char temp_path[2*MAX_PATH];
	get_mod_art_path ("UnitRCMIcons.pcx", temp_path, sizeof temp_path);

	PCX_Image_read_file (&pcx, __, temp_path, NULL, 0, 0x100, 2);
	int width;
	if ((pcx.JGL.Image == NULL) ||
	    (2 >= (width = pcx.JGL.Image->vtable->m54_Get_Width (pcx.JGL.Image)))){
		(*p_OutputDebugStringA) ("[C3X] PCX file for unit RCM icons failed to load or is too narrow.");
		goto cleanup;
	}

	for (int n = 0; n < COUNT_UNIT_RCM_ICONS; n++) {
		Tile_Image_Info_construct (&is->unit_rcm_icons[n]);
		Tile_Image_Info_slice_pcx (&is->unit_rcm_icons[n], __, &pcx, 1, 1 + 16*n, width-2, 15, 1, 0);
	}

	is->unit_rcm_icon_state = IS_OK;
cleanup:
	pcx.vtable->destruct (&pcx, __, 0);
}

void
deinit_unit_rcm_icons ()
{
	if (is->unit_rcm_icon_state == IS_OK) {
		for (int n = 0; n < COUNT_UNIT_RCM_ICONS; n++) {
			Tile_Image_Info * tii = &is->unit_rcm_icons[n];
			tii->vtable->destruct (tii, __, 0);
		}
		is->unit_rcm_icon_state = IS_UNINITED;
	}
}

int __cdecl
patch_get_tile_occupier_for_ai_path (int x, int y, int pov_civ_id, bool respect_unit_invisibility)
{
	is->getting_tile_occupier_for_ai_pathfinding = true;
	return get_tile_occupier_id (x, y, pov_civ_id, respect_unit_invisibility);
	is->getting_tile_occupier_for_ai_pathfinding = false;
}

char __fastcall patch_Unit_is_visible_to_civ (Unit * this, int edx, int civ_id, int param_2);

char __fastcall
patch_Unit_is_tile_occupier_visible (Unit * this, int edx, int civ_id, int param_2)
{
	// Here's the fix for the submarine bug: If we're constructing a path for an AI unit, ignore unit invisibility so the pathfinder will path
	// around instead of over other civ's units. We must carve out an exception if the AI is at war with the unit in question. In that case if it
	// "accidentally" paths over the unit, it should get stuck in combat like the human player would.
	if (is->current_config.patch_submarine_bug &&
	    is->getting_tile_occupier_for_ai_pathfinding &&
	    ! this->vtable->is_enemy_of_civ (this, __, civ_id, 0))
		return 1;
	else
		return patch_Unit_is_visible_to_civ (this, __, civ_id, param_2);
}

void do_trade_scroll (DiploForm * diplo, int forward);

void __cdecl
activate_trade_scroll_button (int control_id)
{
	do_trade_scroll (p_diplo_form, control_id == TRADE_SCROLL_BUTTON_ID_RIGHT);
}

void
init_trade_scroll_buttons (DiploForm * diplo_form)
{
	if (is->trade_scroll_button_state != IS_UNINITED)
		return;

	char temp_path[2*MAX_PATH];
	PCX_Image pcx;
	PCX_Image_construct (&pcx);
	get_mod_art_path ("TradeScrollButtons.pcx", temp_path, sizeof temp_path);
	PCX_Image_read_file (&pcx, __, temp_path, NULL, 0, 0x100, 2);
	if (pcx.JGL.Image == NULL) {
		is->trade_scroll_button_state = IS_INIT_FAILED;
		(*p_OutputDebugStringA) ("[C3X] Failed to load TradeScrollButtons.pcx");
		goto cleanup;
	}

	// Stores normal, rollover, and highlight images, in that order, first for the left button then for the right
	is->trade_scroll_button_images = calloc (6, sizeof is->trade_scroll_button_images[0]);
	for (int n = 0; n < 6; n++)
		Tile_Image_Info_construct (&is->trade_scroll_button_images[n]);
	Tile_Image_Info * imgs = is->trade_scroll_button_images;

	for (int right = 0; right < 2; right++)
		for (int n = 0; n < 3; n++)
			Tile_Image_Info_slice_pcx (&imgs[n + 3*right], __, &pcx, right ? 44 : 0, 1 + 48*n, 43, 47, 1, 1);

	for (int right = 0; right < 2; right++) {
		Button * b = new (sizeof *b);

		Button_construct (b);
		int id = right ? TRADE_SCROLL_BUTTON_ID_RIGHT : TRADE_SCROLL_BUTTON_ID_LEFT;
		Button_initialize (b, __, NULL, id, right ? 622 : 358, 50, 43, 47, (Base_Form *)diplo_form, 0);
		for (int n = 0; n < 3; n++)
			b->Images[n] = &imgs[n + 3*right];

		b->activation_handler = &activate_trade_scroll_button;
		b->field_630[0] = 0; // TODO: Is this necessary? It's done by the base game code when creating the city screen scroll buttons

		if (! right)
			is->trade_scroll_button_left = b;
		else
			is->trade_scroll_button_right = b;
	}

	is->trade_scroll_button_state = IS_OK;
cleanup:
	pcx.vtable->destruct (&pcx, __, 0);
}

void
deinit_trade_scroll_buttons ()
{
	if (is->trade_scroll_button_state == IS_OK) {
		is->trade_scroll_button_left ->vtable->destruct ((Base_Form *)is->trade_scroll_button_left , __, 0);
		is->trade_scroll_button_right->vtable->destruct ((Base_Form *)is->trade_scroll_button_right, __, 0);
		is->trade_scroll_button_left = is->trade_scroll_button_right = NULL;
		for (int n = 0; n < 6; n++) {
			Tile_Image_Info * tii = &is->trade_scroll_button_images[n];
			tii->vtable->destruct (tii, __, 0);
		}
		free (is->trade_scroll_button_images);
		is->trade_scroll_button_images = NULL;
	}
	is->trade_scroll_button_state = IS_UNINITED;
}

void
init_mod_info_button_images ()
{
	if (is->mod_info_button_images_state != IS_UNINITED)
		return;

	is->mod_info_button_images_state = IS_INIT_FAILED;

	PCX_Image * descbox_pcx = new (sizeof *descbox_pcx);
	PCX_Image_construct (descbox_pcx);
	char * descbox_path = BIC_get_asset_path (p_bic_data, __, "art\\civilopedia\\descbox.pcx", true);
	PCX_Image_read_file (descbox_pcx, __, descbox_path, NULL, 0, 0x100, 1);
	if (descbox_pcx->JGL.Image == NULL) {
		(*p_OutputDebugStringA) ("[C3X] Failed to load descbox.pcx");
		return;
	}

	for (int n = 0; n < 3; n++) {
		Tile_Image_Info_construct (&is->mod_info_button_images[n]);
		Tile_Image_Info_slice_pcx_with_color_table (&is->mod_info_button_images[n], __, descbox_pcx, 1 + n * 103, 1, MOD_INFO_BUTTON_WIDTH,
							    MOD_INFO_BUTTON_HEIGHT, 1, 1);
	}

	is->mod_info_button_images_state = IS_OK;
}

int
count_escorters (Unit * unit)
{
	IDLS * idls = &unit->Body.IDLS;
	if (idls->escorters.contents != NULL) {
		int tr = 0;
		for (int * p_escorter_id = idls->escorters.contents; p_escorter_id < idls->escorters.contents_end; p_escorter_id++)
			tr += NULL != get_unit_ptr (*p_escorter_id);
		return tr;
	} else
		return 0;
}

// If "unit" belongs to the AI, records that all players that have visibility on (x, y) have seen an AI unit
void
record_ai_unit_seen (Unit * unit, int x, int y)
{
	if (0 == (*p_human_player_bits & 1<<unit->Body.CivID)) {
		Tile * tile = tile_at (x, y);
		if ((tile != NULL) && (tile != p_null_tile)) {
			Tile_Body * body = &tile->Body;
			is->players_saw_ai_unit |= body->FOWStatus | body->V3 | body->Visibility | body->field_D0_Visibility;
		}
	}
}

void __fastcall
patch_Unit_bombard_tile (Unit * this, int edx, int x, int y)
{
	record_ai_unit_seen (this, x, y);
	Unit_bombard_tile (this, __, x, y);
	is->bombard_stealth_target = NULL;
}

void __fastcall
patch_Unit_move (Unit * this, int edx, int tile_x, int tile_y)
{
	record_ai_unit_seen (this, tile_x, tile_y);
	Unit_move (this, __, tile_x, tile_y);
}

bool
can_attack_this_turn (Unit * unit)
{
	// return unit has moves left AND (unit hasn't attacked this turn OR unit has blitz ability)
	return (unit->Body.Moves < Unit_get_max_move_points (unit)) &&
		(((unit->Body.Status & USF_USED_ATTACK) == 0) ||
		 UnitType_has_ability (&p_bic_data->UnitTypes[unit->Body.UnitTypeID], __, UTA_Blitz));
}

bool
can_damage_bombarding (UnitType * attacker_type, Unit * defender, Tile * defender_tile)
{
	UnitType * defender_type = &p_bic_data->UnitTypes[defender->Body.UnitTypeID];
	if (defender_type->Unit_Class == UTC_Land) {
		int has_lethal_land_bombard = UnitType_has_ability (attacker_type, __, UTA_Lethal_Land_Bombardment);
		return defender->Body.Damage + (! has_lethal_land_bombard) < Unit_get_max_hp (defender);
	} else if (defender_type->Unit_Class == UTC_Sea) {
		// Land artillery can't normally damage ships in port
		if ((attacker_type->Unit_Class == UTC_Land) && (! is->current_config.remove_land_artillery_target_restrictions) && Tile_has_city (defender_tile))
			return false;
		int has_lethal_sea_bombard = UnitType_has_ability (attacker_type, __, UTA_Lethal_Sea_Bombardment);
		return defender->Body.Damage + (! has_lethal_sea_bombard) < Unit_get_max_hp (defender);
	} else if (defender_type->Unit_Class == UTC_Air) {
		if (is->current_config.immunize_aircraft_against_bombardment)
			return false;
		// Can't damage aircraft in an airfield by bombarding, the attack doesn't even go off
		if ((defender_tile->vtable->m42_Get_Overlays (defender_tile, __, 0) & 0x20000000) != 0)
			return false;
		// Land artillery can't normally damage aircraft but naval artillery and other aircraft can. Lethal bombard doesn't apply; anything
		// that can damage can kill.
		return (attacker_type->Unit_Class != UTC_Land) || is->current_config.remove_land_artillery_target_restrictions;
	} else // UTC_Space? UTC_Alternate_Dimension???
		return false;
}

char __fastcall
patch_Unit_is_visible_to_civ (Unit * this, int edx, int civ_id, int param_2)
{
	char base_vis = Unit_is_visible_to_civ (this, __, civ_id, param_2);
	if ((! base_vis) && // if unit is not visible to civ_id AND
	    is->current_config.share_visibility_in_hoseat && // shared hotseat vis is enabled AND
	    ((1 << civ_id) & *p_human_player_bits) && // civ_id is a human player AND
	    (*p_is_offline_mp_game && ! *p_is_pbem_game)) { // we're in a hotseat game

		// Check if the unit is visible to any other human player in the game
		unsigned player_bits = *(unsigned *)p_human_player_bits >> 1;
		int n_player = 1;
		while (player_bits != 0) {
			if ((player_bits & 1) &&
			    (n_player != civ_id) &&
			    Unit_is_visible_to_civ (this, __, n_player, param_2))
				return 1;
			player_bits >>= 1;
			n_player++;
		}
	}

	return base_vis;
}

bool
has_any_destructible_improvements (City * city)
{
	for (int n = 0; n < p_bic_data->ImprovementsCount; n++) {
		Improvement * improv = &p_bic_data->Improvements[n];
		if (((improv->Characteristics & (ITC_Wonder | ITC_Small_Wonder)) == 0) && // if improv is not a wonder AND
		    ((improv->ImprovementFlags & ITF_Center_of_Empire) == 0) && // it's not the palace AND
		    (improv->SpaceshipPart < 0) && // it's not a spaceship part AND
		    City_has_improvement (city, __, n, 0)) // it's present in the city ignoring free improvements
			return true;
	}
	return false;
}

int const destructible_overlays =
	0x00000003 | // road, railroad
	0x00000004 | // mine
	0x00000008 | // irrigation
	0x00000010 | // fortress
	0x10000000 | // barricade
	0x20000000 | // airfield
	0x40000000 | // radar
	0x80000000;  // outpost

bool
has_any_destructible_overlays (Tile * tile, bool precision_strike)
{
	int overlays = tile->vtable->m42_Get_Overlays (tile, __, 0);
	if ((overlays & destructible_overlays) == 0)
		return false;
	else {
		if (! precision_strike)
			return true;
		else {
			if (overlays == 0x20000000) { // if tile ONLY has an airfield
				int any_aircraft_on_tile = 0; {
					FOR_UNITS_ON (uti, tile)
						if (p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID].Unit_Class == UTC_Air) {
							any_aircraft_on_tile = 1;
							break;
						}
				}
				return ! any_aircraft_on_tile;
			} else
				return true;
		}
	}
}

void __fastcall
patch_Main_Screen_Form_perform_action_on_tile (Main_Screen_Form * this, int edx, enum Unit_Mode_Actions action, int x, int y)
{
	if ((! is->current_config.enable_stack_bombard) || // if stack bombard is disabled OR
	    (! ((action == UMA_Bombard) || (action == UMA_Air_Bombard))) || // action is not bombard OR
	    ((((*p_GetAsyncKeyState) (VK_CONTROL)) >> 8 == 0) &&  // (control key is not down AND
	     ((is->sc_img_state != IS_UNINITED) && (is->sb_activated_by_button == 0))) || // (button flag is valid AND not set)) OR
	    is_online_game ()) { // is online game
		Main_Screen_Form_perform_action_on_tile (this, __, action, x, y);
		return;
	}

	// Save preferences so we can restore them at the end of the stack bombard operation. We might change them to turn off combat animations.
	unsigned init_prefs = *p_preferences;

	clear_memo ();

	wrap_tile_coords (&p_bic_data->Map, &x, &y);
	Tile * base_tile = tile_at (this->Current_Unit->Body.X, this->Current_Unit->Body.Y);
	Tile * target_tile = tile_at (x, y);
	int attacker_type_id = this->Current_Unit->Body.UnitTypeID;
	UnitType * attacker_type = &p_bic_data->UnitTypes[attacker_type_id];
	int civ_id = this->Current_Unit->Body.CivID;

	// Count & memoize attackers
	int selected_unit_id = this->Current_Unit->Body.ID;
	FOR_UNITS_ON (uti, base_tile)
		if ((uti.id != selected_unit_id) &&
		    (uti.unit->Body.CivID == civ_id) &&
		    (uti.unit->Body.UnitTypeID == attacker_type_id) &&
		    ((uti.unit->Body.Container_Unit < 0) || (attacker_type->Unit_Class == UTC_Air)) &&
		    (uti.unit->Body.UnitState == 0) &&
		    can_attack_this_turn (uti.unit))
			memoize (uti.id);
	int count_attackers = is->memo_len;
	
	// Count & memoize targets (also count air units while we're at it)
	int num_air_units_on_target_tile = 0;
	FOR_UNITS_ON (uti, target_tile) {
		num_air_units_on_target_tile += UTC_Air == p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID].Unit_Class;
		if ((uti.unit->Body.CivID != civ_id) &&
		    (Unit_get_defense_strength (uti.unit) > 0) &&
		    (uti.unit->Body.Container_Unit < 0) &&
		    patch_Unit_is_visible_to_civ (uti.unit, __, civ_id, 0) &&
		    can_damage_bombarding (attacker_type, uti.unit, target_tile))
			memoize (uti.id);
	}
	int count_targets = is->memo_len - count_attackers;

	// Now our attackers and targets arrays will just be pointers into the memo
	int * attackers = &is->memo[0], * targets = &is->memo[count_attackers];

	int attacking_units = 0, attacking_tile = 0;
	City * target_city = NULL;
	if (count_targets > 0)
		attacking_units = 1;
	else if (Tile_has_city (target_tile))
		target_city = get_city_ptr (target_tile->vtable->m45_Get_City_ID (target_tile));
	else {
		// Make sure not to set attacking_tile when the tile has an airfield and air units (but no land units) on
		// it. In that case we can't damage the airfield and if we try to anyway, the units will waste their moves
		// without attacking.
		int has_airfield = (target_tile->vtable->m42_Get_Overlays (target_tile, __, 0) & 0x20000000) != 0;
		attacking_tile = (! has_airfield) || (num_air_units_on_target_tile == 0);
	}

	Unit * next_up = this->Current_Unit;
	int i_next_attacker = 0;
	int anything_left_to_attack;
	int last_attack_didnt_happen;
	do {
		// If combat animations were enabled when the stack bombard operation started, reset the preference according to the state of the
		// shift key (down => skip animations, up => show them).
		if (init_prefs & P_ANIMATE_BATTLES) {
			if ((*p_GetAsyncKeyState) (VK_SHIFT) >> 8)
				*p_preferences &= ~P_ANIMATE_BATTLES;
			else
				*p_preferences |= P_ANIMATE_BATTLES;
		}

		int moves_before_bombard = next_up->Body.Moves;

		patch_Unit_bombard_tile (next_up, __, x, y);

		// Check if the last unit sent into battle actually did anything. If it didn't we should at least skip over
		// it to avoid an infinite loop, but actually the only time this should happen is if the player is not at
		// war with the targetted civ and chose not to declare when prompted. In this case it's better to just stop
		// trying to attack so as to not spam the player with prompts.
		last_attack_didnt_happen = (next_up->Body.Moves == moves_before_bombard);

		if (! can_attack_this_turn (next_up)) {
			next_up = NULL;
			while ((i_next_attacker < count_attackers) && (next_up == NULL))
				next_up = get_unit_ptr (attackers[i_next_attacker++]);
		}
		
		if (attacking_units) {
			anything_left_to_attack = 0;
			for (int n = 0; n < count_targets; n++) {
				Unit * unit = get_unit_ptr (targets[n]);

				// Make sure this unit is still a valid target. Keep in mind it's possible for new units to be created during the
				// stack bombard operation if the attackers have lethal bombard and the enslave ability.
				if ((unit != NULL) && (unit->Body.X == x) && (unit->Body.Y == y) && (unit->Body.CivID != civ_id)) {

					if (can_damage_bombarding (attacker_type, unit, target_tile)) {
						anything_left_to_attack = 1;
						break;
					}
				}
			}
		} else if (target_city != NULL)
			anything_left_to_attack = (target_city->Body.Population.Size > 1) || has_any_destructible_improvements (target_city);
		else if (attacking_tile)
			anything_left_to_attack = has_any_destructible_overlays (target_tile, false);
		else
			anything_left_to_attack = 0;
	} while ((next_up != NULL) && anything_left_to_attack && (! last_attack_didnt_happen));

	is->sb_activated_by_button = 0;
	*p_preferences = init_prefs;
	this->GUI.Base.vtable->m73_call_m22_Draw ((Base_Form *)&this->GUI);
}

void
set_up_stack_bombard_buttons (Main_GUI * this)
{
	if (is_online_game () || (! is->current_config.enable_stack_bombard))
		return;

	init_stackable_command_buttons ();
	if (is->sc_img_state != IS_OK)
		return;

	// Find button that the original method set to (air) bombard, then find the next unused button after that.
	Command_Button * bombard_button = NULL, * free_button = NULL; {
		int i_bombard_button;
		for (int n = 0; n < 42; n++)
			if (((this->Unit_Command_Buttons[n].Button.Base_Data.Status2 & 1) != 0) &&
			    (this->Unit_Command_Buttons[n].Command == UCV_Bombard || this->Unit_Command_Buttons[n].Command == UCV_Bombing)) {
				bombard_button = &this->Unit_Command_Buttons[n];
				i_bombard_button = n;
				break;
			}
		if (bombard_button != NULL)
			for (int n = i_bombard_button + 1; n < 42; n++)
				if ((this->Unit_Command_Buttons[n].Button.Base_Data.Status2 & 1) == 0) {
					free_button = &this->Unit_Command_Buttons[n];
					break;
				}
	}

	if ((bombard_button == NULL) || (free_button == NULL))
		return;

	// Set up free button for stack bombard
	free_button->Command   = bombard_button->Command;
	free_button->field_6D8 = bombard_button->field_6D8;
	struct sc_button_image_set * img_set  =
		(bombard_button->Command == UCV_Bombing) ? &is->sc_button_image_sets[SC_BOMB] : &is->sc_button_image_sets[SC_BOMBARD];
	for (int n = 0; n < 4; n++)
		free_button->Button.Images[n] = &img_set->imgs[n];
	free_button->Button.field_664 = bombard_button->Button.field_664;
	// FUN_005559E0 is also called in the original code. I don't know what it actually does but I'm pretty sure it doesn't
	// matter for our purposes.
	Button_set_tooltip (&free_button->Button, __, is->c3x_labels[CL_SB_TOOLTIP]);
	free_button->Button.field_5FC[13] = bombard_button->Button.field_5FC[13];
	free_button->Button.vtable->m01_Show_Enabled ((Base_Form *)&free_button->Button, __, 0);
}

void
set_up_stack_worker_buttons (Main_GUI * this)
{
	if ((((*p_GetAsyncKeyState) (VK_CONTROL)) >> 8 == 0) ||  // (control key is not down OR
	    (! is->current_config.enable_stack_unit_commands) || // stack worker commands not enabled OR
	    is_online_game ()) // is online game
		return;

	init_stackable_command_buttons ();
	if (is->sc_img_state != IS_OK)
		return;

	// For each unit command button
	for (int n = 0; n < 42; n++) {
		Command_Button * cb = &this->Unit_Command_Buttons[n];

		// If it's enabled and not a bombard button (those are handled in the function above)
		if (((cb->Button.Base_Data.Status2 & 1) != 0) &&
		    (cb->Command != UCV_Bombard) && (cb->Command != UCV_Bombing)) {

			// Find the stackable worker command that this button controls, if there is one, and check that
			// the button isn't already showing the stack image for that command. Note: this check is important
			// b/c this function gets called repeatedly while the CTRL key is held down.
			for (int sc = 0; sc < COUNT_STACKABLE_COMMANDS; sc++)
				if ((cb->Command == sc_button_infos[sc].command) &&
				    (cb->Button.Images[0] != &is->sc_button_image_sets[sc].imgs[0])) {

					// Replace the button's image with the stack image. Disabling & re-enabling and
					// clearing field_5FC[13] are all necessary to trigger a redraw.
					cb->Button.vtable->m02_Show_Disabled ((Base_Form *)&cb->Button);
					for (int k = 0; k < 4; k++)
						cb->Button.Images[k] = &is->sc_button_image_sets[sc].imgs[k];
					cb->Button.field_5FC[13] = 0;
					cb->Button.vtable->m01_Show_Enabled ((Base_Form *)&cb->Button, __, 0);

					break;
				}
		}
	}
}

CityLocValidity __fastcall patch_Map_check_city_location (Map * this, int edx, int tile_x, int tile_y, int civ_id, bool check_for_city_on_tile);

void __fastcall
patch_Main_GUI_set_up_unit_command_buttons (Main_GUI * this)
{
	Main_GUI_set_up_unit_command_buttons (this);
	set_up_stack_bombard_buttons (this);
	set_up_stack_worker_buttons (this);

	// If the minimum city separation is increased, then gray out the found city button if we're too close to another city.
	if ((is->current_config.minimum_city_separation > 1) && (p_main_screen_form->Current_Unit != NULL) && (is->disabled_command_img_state == IS_OK)) {
		Unit_Body * selected_unit = &p_main_screen_form->Current_Unit->Body;

		// For each unit command button
		for (int n = 0; n < 42; n++) {
			Command_Button * cb = &this->Unit_Command_Buttons[n];

			// If it's enabled, set to city founding, and the current city location is too close to another city
			if (((cb->Button.Base_Data.Status2 & 1) != 0) &&
			    (cb->Command == UCV_Build_City) &&
			    (patch_Map_check_city_location (&p_bic_data->Map, __, selected_unit->X, selected_unit->Y, selected_unit->CivID, false) == CLV_CITY_TOO_CLOSE)) {

				// Replace the button's image with the disabled image, as in set_up_stack_worker_buttons.
				cb->Button.vtable->m02_Show_Disabled ((Base_Form *)&cb->Button);
				for (int k = 0; k < 3; k++)
					cb->Button.Images[k] = &is->disabled_build_city_button_img;
				cb->Button.field_5FC[13] = 0;

				char tooltip[200]; {
					memset (tooltip, 0, sizeof tooltip);
					char * label = is->c3x_labels[CL_CITY_TOO_CLOSE_BUTTON_TOOLTIP],
					     * to_replace = "$NUM0",
					     * replace_location = strstr (label, to_replace);
					if (replace_location != NULL)
						snprintf (tooltip, sizeof tooltip, "%.*s%d%s", replace_location - label, label, is->current_config.minimum_city_separation, replace_location + strlen (to_replace));
					else
						snprintf (tooltip, sizeof tooltip, "%s", label);
					tooltip[(sizeof tooltip) - 1] = '\0';
				}
				Button_set_tooltip (&cb->Button, __, tooltip);

				cb->Button.vtable->m01_Show_Enabled ((Base_Form *)&cb->Button, __, 0);

			}
		}
	}
}

void
check_happiness_at_end_of_turn ()
{
	int num_unhappy_cities = 0;
	City * first_unhappy_city = NULL;
	FOR_CITIES_OF (coi, p_main_screen_form->Player_CivID) {
		City_recompute_happiness (coi.city);
		int num_happy = 0, num_unhappy = 0;
		FOR_CITIZENS_IN (ci, coi.city) {
			num_happy   += ci.ctzn->Body.Mood == CMT_Happy;
			num_unhappy += ci.ctzn->Body.Mood == CMT_Unhappy;
		}
		if (num_unhappy > num_happy) {
			num_unhappy_cities++;
			if (first_unhappy_city == NULL)
				first_unhappy_city = coi.city;
		}
	}

	if (first_unhappy_city != NULL) {
		PopupForm * popup = get_popup_form ();
		set_popup_str_param (0, first_unhappy_city->Body.CityName, -1, -1);
		if (num_unhappy_cities > 1)
			set_popup_int_param (1, num_unhappy_cities - 1);
		char * key = (num_unhappy_cities > 1) ? "C3X_DISORDER_WARNING_MULTIPLE" : "C3X_DISORDER_WARNING_ONE";
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, key, -1, 0, 0, 0);
		int response = patch_show_popup (popup, __, 0, 0);

		if (response == 2) { // zoom to city
			p_main_screen_form->turn_end_flag = 1;
			City_zoom_to (first_unhappy_city, __, 0);
		} else if (response == 1) // just cancel turn end
			p_main_screen_form->turn_end_flag = 1;
		// else do nothing, let turn end
			
	}
}

void
do_trade_scroll (DiploForm * diplo, int forward)
{
	int increment = forward ? 1 : -1;
	int id = -1;
	for (int n = (diplo->other_party_civ_id + increment) & 31; n != diplo->other_party_civ_id; n = (n + increment) & 31)
		if ((n != 0) && // if N is not barbs AND
		    (n != p_main_screen_form->Player_CivID) && // N is not the player's AND
		    (*p_player_bits & (1U << n)) && // N belongs to an active player AND
		    (leaders[p_main_screen_form->Player_CivID].Contacts[n] & 1) && // N has contact with the player AND
		    Leader_ai_would_meet_with (&leaders[n], __, p_main_screen_form->Player_CivID)) { // AI is willing to meet
			id = n;
			break;
		}

	if (id >= 0) {
		is->trade_screen_scroll_to_id = id;
		DiploForm_close (diplo);
		// Note extra code needs to get run here if the other player is not an AI
	}
}

void __fastcall
patch_DiploForm_m82_handle_key_event (DiploForm * this, int edx, int virtual_key_code, int is_down)
{
	if (is->eligible_for_trade_scroll &&
	    (this->mode == 2) &&
	    ((virtual_key_code == VK_LEFT) || (virtual_key_code == VK_RIGHT)) &&
	    (! is_down))
		do_trade_scroll (this, virtual_key_code == VK_RIGHT);
	else
		DiploForm_m82_handle_key_event (this, __, virtual_key_code, is_down);
}

int __fastcall
patch_DiploForm_m68_Show_Dialog (DiploForm * this, int edx, int param_1, void * param_2, void * param_3)
{
	if (is->open_diplo_form_straight_to_trade) {
		is->open_diplo_form_straight_to_trade = 0;

		// Done by the base game but not necessary as far as I can tell
		// void (__cdecl * FUN_00537700) (int) = 0x537700;
		// FUN_00537700 (0x15);
		// this->field_E9C[0] = this->field_E9C[0] + 1;

		// Set diplo screen mode to two-way trade negotation
		this->mode = 2;

		// Set AI's diplo message to something like "what did you have in mind"
		int iVar35 = DiploForm_set_their_message (this, __, DM_AI_PROPOSAL_RESPONSE, 0, 0x1A8);
		this->field_1390[0] = DM_AI_PROPOSAL_RESPONSE;
		this->field_1390[1] = 0;
		this->field_1390[2] = iVar35;

		// Reset player message options. This will replace the propose deal, declare war, and leave options with the usual "nevermind" option
		// that appears for negotiations.
		DiploForm_reset_our_message_choices (this);

		// Done by the base game but not necessary as far as I can tell
		// void (__fastcall * FUN_004C89A0) (void *) = 0x4C89A0;
		// FUN_004C89A0 (&this->field_1BF4[0]);

	}

	return DiploForm_m68_Show_Dialog (this, __, param_1, param_2, param_3);
}

void __fastcall
patch_DiploForm_do_diplomacy (DiploForm * this, int edx, int diplo_message, int param_2, int civ_id, int do_not_request_audience, int war_negotiation, int disallow_proposal, TradeOfferList * our_offers, TradeOfferList * their_offers)
{
	is->open_diplo_form_straight_to_trade = 0;
	is->trade_screen_scroll_to_id = -1;

	// Trade screen scroll is disabled in online games b/c there's extra synchronization we'd need to do to open or close the diplo screen with
	// a human player.
	is->eligible_for_trade_scroll = is->current_config.enable_trade_screen_scroll && (! is_online_game ());

	if (is->eligible_for_trade_scroll && (is->trade_scroll_button_state == IS_UNINITED))
		init_trade_scroll_buttons (this);

	WITH_PAUSE_FOR_POPUP {
		DiploForm_do_diplomacy (this, __, diplo_message, param_2, civ_id, do_not_request_audience, war_negotiation, disallow_proposal, our_offers, their_offers);

		while (is->trade_screen_scroll_to_id >= 0) {
			int scroll_to_id = is->trade_screen_scroll_to_id;
			is->trade_screen_scroll_to_id = -1;
			is->open_diplo_form_straight_to_trade = 1;
			DiploForm_do_diplomacy (this, __, DM_AI_COUNTER, 0, scroll_to_id, 1, 0, 0, NULL, NULL);
		}
	}

	is->open_diplo_form_straight_to_trade = 0;
	is->eligible_for_trade_scroll = 0;
}

void __fastcall
patch_DiploForm_m22_Draw (DiploForm * this)
{
	if (is->trade_scroll_button_state == IS_OK) {
		Button * left  = is->trade_scroll_button_left,
		       * right = is->trade_scroll_button_right;
		if (is->eligible_for_trade_scroll && (this->mode == 2)) {
			left ->vtable->m01_Show_Enabled ((Base_Form *)left , __, 0);
			right->vtable->m01_Show_Enabled ((Base_Form *)right, __, 0);
			left ->vtable->m73_call_m22_Draw ((Base_Form *)left);
			right->vtable->m73_call_m22_Draw ((Base_Form *)right);
		} else {
			left ->vtable->m02_Show_Disabled ((Base_Form *)left);
			right->vtable->m02_Show_Disabled ((Base_Form *)right);
		}
	}

	DiploForm_m22_Draw (this);
}

void
intercept_end_of_turn ()
{
	if (is->current_config.enable_disorder_warning) {
		check_happiness_at_end_of_turn ();
		if (p_main_screen_form->turn_end_flag == 1) // Check if player cancelled turn ending in the disorder warning popup
			return;
	}

	// Clear things that don't apply across turns
	is->have_job_and_loc_to_skip = 0;
}

bool
is_worker_or_settler_command (int unit_command_value)
{
	return (unit_command_value & 0x20000000) ||
		((unit_command_value >= UCV_Build_Remote_Colony) && (unit_command_value <= UCV_Auto_Save_Tiles));
}

bool __fastcall
patch_Unit_can_upgrade (Unit * this)
{
	bool base = Unit_can_upgrade (this);
	int available;
	City * city = city_at (this->Body.X, this->Body.Y);
	if (base &&
	    (city != NULL) &&
	    get_available_unit_count (&leaders[this->Body.CivID], City_get_upgraded_type_id (city, __, this->Body.UnitTypeID), &available) &&
	    (available <= 0))
		return false;
	else
		return base;
}

bool __fastcall
patch_Unit_can_perform_command (Unit * this, int edx, int unit_command_value)
{
	if (is->current_config.disable_worker_automation &&
	    (this->Body.CivID == p_main_screen_form->Player_CivID) &&
	    (unit_command_value == UCV_Automate))
		return false;
	else if (is->current_config.disallow_land_units_from_affecting_water_tiles &&
		 is_worker_or_settler_command (unit_command_value)) {
		Tile * tile = tile_at (this->Body.X, this->Body.Y);
		enum UnitTypeClasses class = p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class;
		return ((class != UTC_Land) || (! tile->vtable->m35_Check_Is_Water (tile))) &&
			Unit_can_perform_command (this, __, unit_command_value);
	} else
		return Unit_can_perform_command (this, __, unit_command_value);
}

bool __fastcall
patch_Unit_can_do_worker_command_for_button_setup (Unit * this, int edx, int unit_command_value)
{
	bool base = patch_Unit_can_perform_command (this, __, unit_command_value);

	// If the command is to build a city and it can't be done because another city is already too close, and the minimum separation was changed
	// from its standard value, then return true here so that the build city button will be added anyway. We'll gray it out later. Check that the
	// grayed out button image is initialized now so we don't activate the build city button then find out later we can't gray it out.
	if ((! base) &&
	    (unit_command_value == UCV_Build_City) &&
	    (is->current_config.minimum_city_separation > 1) &&
	    (patch_Map_check_city_location (&p_bic_data->Map, __, this->Body.X, this->Body.Y, this->Body.CivID, false) == CLV_CITY_TOO_CLOSE) &&
	    (init_disabled_command_buttons (), is->disabled_command_img_state == IS_OK))
		return true;

	else
		return base;
}

int
compare_helpers (void const * vp_a, void const * vp_b)
{
	Unit * a = get_unit_ptr (*(int *)vp_a),
	     * b = get_unit_ptr (*(int *)vp_b);
	if ((a != NULL) && (b != NULL)) {
		// Compute how many movement points each has left (ML = moves left)
		int ml_a = Unit_get_max_move_points (a) - a->Body.Moves,
		    ml_b = Unit_get_max_move_points (b) - b->Body.Moves;

		// Whichever one has more MP left comes first in the array
		if      (ml_a > ml_b) return  1;
		else if (ml_b > ml_a) return -1;
		else 		      return  0;
	} else
		// If at least one of the unit ids is invalid, might as well sort it later in the array
		return (a != NULL) ? -1 : ((b != NULL) ? 1 : 0);
}

void
issue_stack_worker_command (Unit * unit, int command)
{
	Tile * tile = tile_at (unit->Body.X, unit->Body.Y);
	int unit_type_id = unit->Body.UnitTypeID;
	int unit_id = unit->Body.ID;

	// Put together a list of helpers and store it on the memo. Helpers are just other workers on the same tile that can be issued the same command.
	clear_memo ();
	FOR_UNITS_ON (uti, tile)
		if ((uti.id != unit_id) &&
		    (uti.unit->Body.Container_Unit < 0) &&
		    (uti.unit->Body.UnitState == 0) &&
		    (uti.unit->Body.Moves < Unit_get_max_move_points (uti.unit))) {
			// check if the clicked command is among worker actions that this unit type can perform
			int actions = p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID].Worker_Actions;
			int command_without_category = command & 0x0FFFFFFF;
			if ((actions & command_without_category) == command_without_category)
				memoize (uti.id);
		}
	
	// Sort the list of helpers so that the ones with the fewest remaining movement points are listed first.
	qsort (is->memo, is->memo_len, sizeof is->memo[0], compare_helpers);

	Unit * next_up = unit;
	int i_next_helper = 0;
	int last_action_didnt_happen;
	do {
		int state_before_action = next_up->Body.UnitState;
		Main_Screen_Form_issue_command (p_main_screen_form, __, command, next_up);
		last_action_didnt_happen = (next_up->Body.UnitState == state_before_action);

		// Call this update function to cause the worker to actually perform the action. Otherwise
		// it only gets queued, the worker keeps is movement points, and the action doesn't get done
		// until the interturn.
		if (! last_action_didnt_happen)
			next_up->vtable->update_while_active (next_up);

		next_up = NULL;
		while ((i_next_helper < is->memo_len) && (next_up == NULL))
			next_up = get_unit_ptr (is->memo[i_next_helper++]);
	} while ((next_up != NULL) && (! last_action_didnt_happen));
}

void
issue_stack_unit_mgmt_command (Unit * unit, int command)
{
	Tile * tile = tile_at (unit->Body.X, unit->Body.Y);
	int unit_type_id = unit->Body.UnitTypeID;
	int unit_id = unit->Body.ID;

	PopupForm * popup = get_popup_form ();

	clear_memo ();

	if (command == UCV_Fortify) {
		// This probably won't work for online games since "fortify all" does additional work in that case. See Main_Screen_Form::fortify_all.
		// I don't like how this method doesn't place units in the fortified pose. One workaround is so use
		// Main_Screen_Form::issue_fortify_command, but that plays the entire fortify animation for each unit which is a major annoyance for
		// large stacks. The base game's "fortify all" function also doesn't set the pose so I don't see any easy way to fix this.
		FOR_UNITS_ON (uti, tile)
			if ((uti.unit->Body.UnitTypeID == unit_type_id) &&
			    (uti.unit->Body.Container_Unit < 0) &&
			    (uti.unit->Body.UnitState == 0) &&
			    (uti.unit->Body.CivID == unit->Body.CivID) &&
			    (uti.unit->Body.Moves < Unit_get_max_move_points (uti.unit)))
				Unit_set_state (uti.unit, __, UnitState_Fortifying);

	} else if (command == UCV_Upgrade_Unit) {
		int our_treasury = leaders[unit->Body.CivID].Gold_Encoded + leaders[unit->Body.CivID].Gold_Decrement;

		// If the unit type we're upgrading to is limited, find out how many we can add. Keep that in "available". If the type is not limited,
		// leave available set to INT_MAX.
		int available = INT_MAX; {
			City * city;
			int upgrade_id;
			if ((is->current_config.unit_limits.len > 0) &&
			    patch_Unit_can_perform_command (unit, __, UCV_Upgrade_Unit) &&
			    (NULL != (city = city_at (unit->Body.X, unit->Body.Y))) &&
			    (0 < (upgrade_id = City_get_upgraded_type_id (city, __, unit_type_id))))
				get_available_unit_count (&leaders[unit->Body.CivID], upgrade_id, &available);
		}

		int cost = 0;
		FOR_UNITS_ON (uti, tile)
			if ((available > 0) &&
			    (uti.unit->Body.UnitTypeID == unit_type_id) &&
			    (uti.unit->Body.Container_Unit < 0) &&
			    (uti.unit->Body.UnitState == 0) &&
			    patch_Unit_can_perform_command (uti.unit, __, UCV_Upgrade_Unit)) {
				cost += Unit_get_upgrade_cost (uti.unit);
				available--;
				memoize (uti.id);
			}

		if (cost <= our_treasury) {
			set_popup_str_param (0, p_bic_data->UnitTypes[unit_type_id].Name, -1, -1);
			set_popup_int_param (0, is->memo_len);
			set_popup_int_param (1, cost);
			popup->vtable->set_text_key_and_flags (popup, __, script_dot_txt_file_path, "UPGRADE_ALL", -1, 0, 0, 0);
			if (patch_show_popup (popup, __, 0, 0) == 0)
				for (int n = 0; n < is->memo_len; n++) {
					Unit * to_upgrade = get_unit_ptr (is->memo[n]);
					if (to_upgrade != NULL)
						Unit_upgrade (to_upgrade, __, false);
				}

		} else {
			set_popup_int_param (0, cost);
			int param_5 = is_online_game () ? 0x4000 : 0; // As in base code
			popup->vtable->set_text_key_and_flags (popup, __, script_dot_txt_file_path, "NO_GOLD_TO_UPGRADE_ALL", -1, 0, param_5, 0);
			patch_show_popup (popup, __, 0, 0);
		}

	} else if (command == UCV_Disband) {
		FOR_UNITS_ON (uti, tile)
			if ((uti.unit->Body.UnitTypeID == unit_type_id) &&
			    (uti.unit->Body.Container_Unit < 0) &&
			    (uti.unit->Body.UnitState == 0) &&
			    (uti.unit->Body.Moves < Unit_get_max_move_points (uti.unit)))
				memoize (uti.id);

		if (is->memo_len > 0) {
			set_popup_int_param (0, is->memo_len);
			popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_CONFIRM_STACK_DISBAND", -1, 0, 0, 0);
			if (patch_show_popup (popup, __, 0, 0) == 0) {
				for (int n = 0; n < is->memo_len; n++) {
					Unit * to_disband = get_unit_ptr (is->memo[n]);
					if (to_disband)
						Unit_disband (to_disband);
				}
			}
		}
	}
}

void __fastcall
patch_Main_GUI_handle_button_press (Main_GUI * this, int edx, int button_id)
{
	// Set SB flag according to case (2)
	if (button_id < 42) {
		if ((is->sc_img_state == IS_OK) &&
		    ((this->Unit_Command_Buttons[button_id].Button.Images[0] == &is->sc_button_image_sets[SC_BOMBARD].imgs[0]) ||
		     (this->Unit_Command_Buttons[button_id].Button.Images[0] == &is->sc_button_image_sets[SC_BOMB   ].imgs[0])))
			is->sb_activated_by_button = 1;
		else
			is->sb_activated_by_button = 0;
	}

	int command = this->Unit_Command_Buttons[button_id].Command;

	struct sc_button_info const * stack_button_info; {
		stack_button_info = NULL;
		if (button_id < 42) // If button pressed was a unit command button
			for (int n = 0; n < COUNT_STACKABLE_COMMANDS; n++)
				if (command == sc_button_infos[n].command) {
					stack_button_info = &sc_button_infos[n];
					break;
				}
	}

	if ((stack_button_info == NULL) || // If there's no stack command for the pressed button OR
	    (! is->current_config.enable_stack_unit_commands) || // stack unit commands are not enabled OR
	    (((*p_GetAsyncKeyState) (VK_CONTROL)) >> 8 == 0) || // CTRL key is not down OR
	    (p_main_screen_form->Current_Unit == NULL) || // no unit is selected OR
	    is_online_game ()) { // is online game
		Main_GUI_handle_button_press (this, __, button_id);
		return;
	}

	enum stackable_command_kind kind = stack_button_info->kind;
	if ((kind == SCK_TERRAFORM) || (kind == SCK_UNIT_MGMT)) {
		// Replicate behavior of function we're replacing
		clear_something_1 ();
		Timer_clear (&this->timer_1);

		if (kind == SCK_TERRAFORM)
			issue_stack_worker_command (p_main_screen_form->Current_Unit, stack_button_info->command);
		else if (kind == SCK_UNIT_MGMT)
			issue_stack_unit_mgmt_command (p_main_screen_form->Current_Unit, stack_button_info->command);
	} else
		Main_GUI_handle_button_press (this, __, button_id);
}

bool
is_command_button_active (Main_GUI * main_gui, enum Unit_Command_Values command)
{
	Command_Button * buttons = main_gui->Unit_Command_Buttons;
	for (int n = 0; n < 42; n++)
		if (((buttons[n].Button.Base_Data.Status2 & 1) != 0) && (buttons[n].Command == command))
			return true;
	return false;
}

int __fastcall
patch_Main_Screen_Form_handle_key_down (Main_Screen_Form * this, int edx, int char_code, int virtual_key_code)
{
	// Set SB flag according to case (4)
	int precision_strike_is_available = is_command_button_active (&this->GUI, UCV_Precision_Bombing);
	if ((virtual_key_code == VK_B) || (precision_strike_is_available && (virtual_key_code == VK_P)))
		is->sb_activated_by_button = 0;

	if ((virtual_key_code & 0xFF) == VK_CONTROL)
		set_up_stack_worker_buttons (&this->GUI);

	char original_turn_end_flag = this->turn_end_flag;
	int tr = Main_Screen_Form_handle_key_down (this, __, char_code, virtual_key_code);
	if ((original_turn_end_flag == 1) && (this->turn_end_flag == 0))
		intercept_end_of_turn ();

	return tr;
}

int
patch_handle_cursor_change_in_jgl ()
{
	// Set SB flag according to case (3) and the annoying state
	if ((is->sb_activated_by_button != 2) &&
	    (p_main_screen_form->Mode_Action != UMA_Bombard) &&
	    (p_main_screen_form->Mode_Action != UMA_Air_Bombard))
		is->sb_activated_by_button = 0;

	return handle_cursor_change_in_jgl ();
}

void __fastcall
patch_Main_Screen_Form_handle_left_click_on_map_1 (Main_Screen_Form * this, int edx, int param_1, int param_2)
{
	if (is->sb_activated_by_button == 1)
		is->sb_activated_by_button = 2;
	Main_Screen_Form_handle_left_click_on_map_1 (this, __, param_1, param_2);
	is->sb_activated_by_button = 0;
}


void __fastcall 
patch_Main_GUI_handle_click_in_status_panel (Main_GUI * this, int edx, int mouse_x, int mouse_y)
{
	char original_turn_end_flag = p_main_screen_form->turn_end_flag;
	Main_GUI_handle_click_in_status_panel (this, __, mouse_x, mouse_y);
	if ((original_turn_end_flag == 1) && (p_main_screen_form->turn_end_flag == 0))
		intercept_end_of_turn ();
}

// Gets effective shields generated per turn, including civil engineers, disorder, and anarchy.
int
get_city_production_rate (City * city, enum City_Order_Types order_type, int order_id)
{
	int in_disorder = city->Body.Status & CSF_Civil_Disorder,
	    in_anarchy = p_bic_data->Governments[leaders[city->Body.CivID].GovernmentType].b_Transition_Type,
	    getting_tile_shields = (! in_disorder) && (! in_anarchy);

	if (order_type == COT_Improvement) {
		int building_wealth = (p_bic_data->Improvements[order_id].ImprovementFlags & ITF_Capitalization) != 0;
		int specialist_shields = 0;
		FOR_CITIZENS_IN (ci, city)
			if ((ci.ctzn->Body.field_20[0] & 0xFF) == 0) // I don't know what is check is for but it's done in the base code
				specialist_shields += p_bic_data->CitizenTypes[ci.ctzn->Body.WorkerType].Construction;
		return (getting_tile_shields ? city->Body.ProductionIncome : 0) + ((! building_wealth) ? specialist_shields : 0);
	} else if ((order_type == COT_Unit) && getting_tile_shields)
		return city->Body.ProductionIncome;
	else
		return 0;
}

void __fastcall
patch_City_Form_open (City_Form * this, int edx, City * city, int param_2)
{
	WITH_PAUSE_FOR_POPUP {
		City_Form_open (this, __, city, param_2);
	}
}

void __fastcall
patch_City_Form_draw (City_Form * this)
{
	// Recompute city form production rect location every time because the config might have changed. Doing it here is also easier than
	// patching the constructor.
	int form_top = (p_bic_data->ScreenHeight - this->Background_Image.Height) / 2;
	this->Production_Storage_Indicator.top = form_top + 621 + (is->current_config.show_detailed_city_production_info ? 34 : 0);

	City_Form_draw (this);

	if (is->current_config.show_detailed_city_production_info) {
		City * city = this->CurrentCity;
		int order_type = city->Body.Order_Type,
		    order_id = city->Body.Order_ID,
		    order_progress = City_get_order_progress (city),
		    order_cost = City_get_order_cost (city),
		    prod_rate = get_city_production_rate (city, order_type, order_id),
		    building_wealth = (order_type == COT_Improvement) && ((p_bic_data->Improvements[order_id].ImprovementFlags & ITF_Capitalization) != 0);

		int turns_left, surplus; {
			if (prod_rate > 0) {
				turns_left = (order_cost - order_progress) / prod_rate;
				if ((order_cost - order_progress) % prod_rate != 0)
					turns_left++;
				if (turns_left < 1)
					turns_left = 1;
				surplus = (turns_left * prod_rate) - (order_cost - order_progress);
			} else {
				turns_left = 9999;
				surplus = 0;
			}
		}

		char line1[100]; {
			if (prod_rate > 0) {
				if (! building_wealth)
					snprintf (line1, sizeof line1, "%s %d %s", this->Labels.To_Build, turns_left, (turns_left == 1) ? this->Labels.Single_Turn : this->Labels.Multiple_Turns);
				else
					snprintf (line1, sizeof line1, "%s", is->c3x_labels[CL_NEVER_COMPLETES]);
			} else
				snprintf (line1, sizeof line1, "%s", is->c3x_labels[CL_HALTED]);
			line1[(sizeof line1) - 1] = '\0';
		}

		char line2[100]; {
			if (! building_wealth) {
				int percent_complete = order_cost > 0 ? ((10000 * order_progress) / order_cost + 50) / 100 : 100;
				snprintf (line2, sizeof line2, "%d / %d (%d%s)", order_progress, order_cost, percent_complete, this->Labels.Percent);
			} else
				snprintf (line2, sizeof line2, "---");
			line2[(sizeof line2) - 1] = '\0';
		}

		char line3[100]; {
			if ((! building_wealth) && (prod_rate > 0)) {
				int s_per, s_rem; {
					if (turns_left > 1) {
						s_per = surplus / turns_left;
						s_rem = surplus % turns_left;
					} else {
						s_per = surplus;
						s_rem = 0;
					}
				}
				char * s_lab = is->c3x_labels[CL_SURPLUS];
				if      ((s_per != 0) && (s_rem != 0)) snprintf (line3, sizeof line3, "%s: %d + %d %s", s_lab, s_rem, s_per, this->Labels.Per_Turn);
				else if ((s_per == 0) && (s_rem != 0)) snprintf (line3, sizeof line3, "%s: %d",         s_lab, s_rem);
				else if ((s_per != 0) && (s_rem == 0)) snprintf (line3, sizeof line3, "%s: %d %s",      s_lab, s_per, this->Labels.Per_Turn);
				else                                   snprintf (line3, sizeof line3, "%s", is->c3x_labels[CL_SURPLUS_NONE]);
			} else
				snprintf (line3, sizeof line3, "%s", is->c3x_labels[CL_SURPLUS_NA]);
			line3[(sizeof line3) - 1] = '\0';
		}

		Object_66C3FC * font = get_font (10, FSF_NONE);
		int left = this->Production_Storage_Indicator.left,
		    top = this->Production_Storage_Indicator.top,
		    width = this->Production_Storage_Indicator.right - left;
		PCX_Image_draw_centered_text (&this->Base.Data.Canvas, __, font, line1, left, top - 42, width, strlen (line1));
		PCX_Image_draw_centered_text (&this->Base.Data.Canvas, __, font, line2, left, top - 28, width, strlen (line2));
		PCX_Image_draw_centered_text (&this->Base.Data.Canvas, __, font, line3, left, top - 14, width, strlen (line3));
	}
}

void __fastcall
patch_City_Form_print_production_info (City_Form *this, int edx, String256 * out_strs, int str_capacity)
{
	City_Form_print_production_info (this, __, out_strs, str_capacity);
	if (is->current_config.show_detailed_city_production_info)
		out_strs[1].S[0] = '\0';
}

// Returns the ID of the civ this move is trespassing against, or 0 if it's not trespassing.
int
check_trespassing (int civ_id, Tile * from, Tile * to)
{
	int from_territory_id = from->vtable->m38_Get_Territory_OwnerID (from),
	    to_territory_id   = to  ->vtable->m38_Get_Territory_OwnerID (to);
	if ((civ_id > 0) &&
	    (to_territory_id != civ_id) &&
	    (to_territory_id > 0) &&
	    (to_territory_id != from_territory_id) &&
	    (! leaders[civ_id].At_War[to_territory_id]) &&
	    ((leaders[civ_id].Relation_Treaties[to_territory_id] & 2) == 0)) // Check right of passage
		return to_territory_id;
	else
		return 0;
}

bool
is_allowed_to_trespass (Unit * unit)
{
	int type_id = unit->Body.UnitTypeID;
	if ((type_id >= 0) && (type_id < p_bic_data->UnitTypeCount)) {
		UnitType * type = &p_bic_data->UnitTypes[type_id];
		return UnitType_has_ability (type, __, UTA_Hidden_Nationality) || UnitType_has_ability (type, __, UTA_Invisible);
	} else
		return false;
}

AdjacentMoveValidity __fastcall
patch_Unit_can_move_to_adjacent_tile (Unit * this, int edx, int neighbor_index, int param_2)
{
	AdjacentMoveValidity base_validity = Unit_can_move_to_adjacent_tile (this, __, neighbor_index, param_2);

	// Apply trespassing restriction
	if (is->current_config.disallow_trespassing && (base_validity == AMV_OK)) {
		Tile * from = tile_at (this->Body.X, this->Body.Y);
		int nx, ny;
		get_neighbor_coords (&p_bic_data->Map, this->Body.X, this->Body.Y, neighbor_index, &nx, &ny);
		int trespasses_against_civ_id = check_trespassing (this->Body.CivID, from, tile_at (nx, ny));
		if ((trespasses_against_civ_id > 0) && (! is_allowed_to_trespass (this)))
			// The tile might be occupied by a unit belonging to a civ other than the one that owns the territory (against whom we'd be
			// trespassing). In this case we must forbid the move entirely since TRIGGERS_WAR will not stop it if we're at war with the
			// occupying civ. This fixes a bug where units could trespass by attacking an enemy unit across a border. They would then get
			// stuck halfway between tiles if they won.
			return (trespasses_against_civ_id == get_tile_occupier_id (nx, ny, this->Body.CivID, false)) ? AMV_TRIGGERS_WAR : AMV_CANNOT_PASS_BETWEEN;
	}

	return base_validity;
}

int __fastcall
patch_Trade_Net_get_movement_cost (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, unsigned flags, int neighbor_index, Trade_Net_Distance_Info * dist_info)
{
	if (is->is_computing_city_connections && // if this call came while rebuilding the trade network AND
	    (is->tnx_init_state == IS_OK) && (is->tnx_cache != NULL) && // Trade Net X is set up AND
	    (unit == NULL) && ((flags == 0x1009) || (flags == 0x9))) // this call can be accelerated by TNX
	    return is->get_move_cost_for_sea_trade (this, is->tnx_cache, from_x, from_y, to_x, to_y, civ_id, flags, neighbor_index, dist_info);

	int const base_cost = Trade_Net_get_movement_cost (this, __, from_x, from_y, to_x, to_y, unit, civ_id, flags, neighbor_index, dist_info);

	// Apply trespassing restriction
	if (is->current_config.disallow_trespassing &&
	    check_trespassing (civ_id, tile_at (from_x, from_y), tile_at (to_x, to_y)) &&
	    ((unit == NULL) || (! is_allowed_to_trespass (unit))))
		return -1;

	// Adjust movement cost to enforce limited railroad movement
	if ((is->current_config.limit_railroad_movement > 0) && (is->saved_road_movement_rate > 0)) {
		if ((unit != NULL) && (base_cost == 0)) { // Railroad move
			int type_moves_available = Unit_get_max_move_points (unit) / p_bic_data->General.RoadsMovementRate;
			return type_moves_available * is->railroad_mp_cost_per_move;
		} else if (base_cost == 1) // Road move
			return is->road_mp_cost;
	}

	return base_cost;
}

int __fastcall
patch_Trade_Net_set_unit_path (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, int flags, int * out_path_length_in_mp)
{
	int tr = Trade_Net_set_unit_path (this, __, from_x, from_y, to_x, to_y, unit, civ_id, flags, out_path_length_in_mp);

	// We might have to correct the path length returned by the base game's pathfinder. This occurs when railroad movement is limited and the
	// unit's total MP exceeds what can be stored in a one-byte integer. The cause of the incorrect length is that the pathfinder internally
	// stores the remaining moves at each node of the search in a single byte. This correction fixes the bug (reported several times) that ETAs
	// shown in the interface are wrong.
	if ((is->current_config.limit_railroad_movement > 0) && // if railroad movement is limited AND
	    (tr > 0) && (out_path_length_in_mp != NULL) && // path was found AND caller wants to know its length AND
	    (unit != NULL) && (Unit_get_max_move_points (unit) > 255)) { // an actual unit was used AND that unit's total MP overflows a uint8

		// First memoize the cost of taking each step along the path. This must be done separately because the pathfinder's internal data only
		// lets us traverse the path backwards.
		{
			// Must pass cached "distance info" to Trade_Net::get_movement_cost. Trade_Net stores two sets of cached data, which one was
			// used depends on the flags. I believe one is intended to be used for city trade connections and the other for unit movement.
			Trade_Net_Distance_Info * dist_info = (flags & 1) ? this->Data2 : this->Data4;

			clear_memo ();
			int x = to_x, y = to_y;
			do {
				// "flags & 1" again determines whether Data2 or Data4 was used.
				enum direction dir = Trade_Net_get_direction_from_internal_map (this, __, x, y, flags & 1);
				if (dir == DIR_ZERO)
					break;

				int prev_x, prev_y; {
					int dx, dy;
					neighbor_index_to_diff (dir, &dx, &dy);
					prev_x = x + dx; prev_y = y + dy;
					wrap_tile_coords (&p_bic_data->Map, &prev_x, &prev_y);
				}

				memoize (patch_Trade_Net_get_movement_cost (this, __, prev_x, prev_y, x, y, unit, civ_id, flags, reverse_dir (dir), dist_info));
				x = prev_x; y = prev_y;
			} while (! ((x == from_x) && (y == from_y)));
		}

		// Now walk the path forwards tracking how much MP the unit would spend. We must be aware that the unit can't spend more MP than it
		// has. For example, if a unit with 1 move walks onto an unimproved mountain, that effectively costs only 1 move, not 3.
		int mp_remaining = Unit_get_max_move_points (unit) - unit->Body.Moves,
		    mp_spent = 0;
		for (int n = is->memo_len - 1; n >= 0; n--) {
			int cost = is->memo[n];
			if (cost < mp_remaining) {
				mp_spent += cost;
				mp_remaining -= cost;
			} else {
				mp_spent += mp_remaining;
				mp_remaining = Unit_get_max_move_points (unit);
			}
		}
		*out_path_length_in_mp = mp_spent;
	}

	return tr;
}

int __fastcall
patch_Trade_Net_set_unit_path_to_find_sea_route (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, int flags, int * out_path_length_in_mp)
{
	// Accelerate this call with TNX if possible
	if ((is->tnx_init_state == IS_OK) && (is->tnx_cache != NULL)) {
		bool route_exists = is->try_drawing_sea_trade_route (this, is->tnx_cache, from_x, from_y, to_x, to_y, civ_id, flags);
		return route_exists ? 1 : 0;
	} else
		return Trade_Net_set_unit_path (this, __, from_x, from_y, to_x, to_y, unit, civ_id, flags, out_path_length_in_mp);
}

// Renames this leader and their civ based on their era. Era-specific names come from the config file. If this leader has a custom name set by the
// human player, this method does nothing.
void
apply_era_specific_names (Leader * leader)
{
	int leader_bit = 1 << leader->ID;
	Race * race = &p_bic_data->Races[leader->RaceID];

	struct replaceable_name {
		char * base_name;
		int * tracking_bits;
		char * buf;
		int buf_size;
	} replaceable_names[] = {
		{race->vtable->GetAdjectiveName (race), &is->aliased_civ_adjective_bits  , leader->tribe_customization.civ_adjective  , sizeof leader->tribe_customization.civ_adjective},
		{race->vtable->GetSingularName (race) , &is->aliased_civ_noun_bits       , leader->tribe_customization.civ_noun       , sizeof leader->tribe_customization.civ_noun},
		{race->vtable->GetCountryName (race)  , &is->aliased_civ_formal_name_bits, leader->tribe_customization.civ_formal_name, sizeof leader->tribe_customization.civ_formal_name}
	};

	// Apply replacements to civ noun, adjective, and formal name
	for (int n = 0; n < ARRAY_LEN (replaceable_names); n++) {
		struct replaceable_name * repl = &replaceable_names[n];
		if ((*repl->tracking_bits & leader_bit) || (repl->buf[0] == '\0')) {
			char * replacement = NULL;
			if (leader->Era < ERA_ALIAS_LIST_CAPACITY)
				// Search through the list of aliases in reverse order so that, if the same key was listed multiple times, the last
				// appearance overrides the earlier ones. This is important b/c when configs are loaded, their aliases get appended to
				// the list.
				for (int k = is->current_config.count_civ_era_alias_lists - 1; k >= 0; k--) {
					struct civ_era_alias_list * list = &is->current_config.civ_era_alias_lists[k];
					if (strcmp (list->key, repl->base_name) == 0) {
						replacement = list->aliases[leader->Era];
						break;
					}
				}
			if (replacement != NULL) {
				strncpy (repl->buf, replacement, repl->buf_size);
				repl->buf[repl->buf_size - 1] = '\0';
				*repl->tracking_bits |= leader_bit;
			} else {
				repl->buf[0] = '\0';
				*repl->tracking_bits &= ~leader_bit;
			}
		}
	}

	// Apply replacement to leader name, gender, and title
	if ((is->aliased_leader_name_bits & leader_bit) || (leader->tribe_customization.leader_name[0] == '\0')) {
		char * base_name = race->vtable->GetLeaderName (race);
		char * replacement_name = NULL;
		char * replacement_title = NULL;
		int replacement_gender; // Only used if replacement_name is
		if (leader->Era < ERA_ALIAS_LIST_CAPACITY)
			for (int k = is->current_config.count_leader_era_alias_lists - 1; k >= 0; k--) {
				struct leader_era_alias_list * list = &is->current_config.leader_era_alias_lists[k];
				if (strcmp (list->key, base_name) == 0) {
					replacement_name = list->aliases[leader->Era];
					replacement_title = list->titles[leader->Era];
					replacement_gender = (list->gender_bits >> leader->Era) & 1;
					break;
				}
			}
		if (replacement_name != NULL) {
			TribeCustomization * tc = &leader->tribe_customization;
			strncpy (tc->leader_name, replacement_name, sizeof tc->leader_name);
			tc->leader_name[(sizeof tc->leader_name) - 1] = '\0';
			tc->leader_gender = replacement_gender;
			is->aliased_leader_name_bits |= leader_bit;

			// If this replacement name has a special title and this player does not have a custom title set, replace the title.
			if ((replacement_title != NULL) && ((is->aliased_leader_title_bits & leader_bit) || (tc->leader_title[0] == '\0'))) {
				strncpy (tc->leader_title, replacement_title, sizeof tc->leader_title);
				tc->leader_title[(sizeof tc->leader_title) - 1] = '\0';
				is->aliased_leader_title_bits |= leader_bit;

			// If the current name has no title and the player's title was previously replaced, undo the replacement.
			} else if ((replacement_title == NULL) && (is->aliased_leader_title_bits & leader_bit)) {
				leader->tribe_customization.leader_title[0] = '\0';
				is->aliased_leader_title_bits &= ~leader_bit;
			}
		} else {
			leader->tribe_customization.leader_name[0] = '\0';
			// Don't need to clear custom leader gender since it's not used unless a custom name was set
			is->aliased_leader_name_bits &= ~leader_bit;

			// Remove title replacement if present
			if (is->aliased_leader_title_bits & leader_bit) {
				leader->tribe_customization.leader_title[0] = '\0';
				is->aliased_leader_title_bits &= ~leader_bit;
			}
		}
	}
}

int __cdecl
patch_do_save_game (char const * file_path, char param_2, GUID * guid)
{
	// Do not save the modified road movement rate, if it was modified to limit railroad movement
	int restore_rmr = (is->current_config.limit_railroad_movement > 0) && (is->saved_road_movement_rate > 0);
	int rmr;
	if (restore_rmr) {
		rmr = p_bic_data->General.RoadsMovementRate;
		p_bic_data->General.RoadsMovementRate = is->saved_road_movement_rate;
	}

	// Do not save the modified barb culture group ID
	int restore_barb_culture_group = is->current_config.enable_city_capture_by_barbarians && (is->saved_barb_culture_group >= 0);
	int barb_culture;
	if (restore_barb_culture_group) {
		barb_culture = p_bic_data->Races[leaders[0].RaceID].CultureGroupID;
		p_bic_data->Races[leaders[0].RaceID].CultureGroupID = is->saved_barb_culture_group;
	}

	// Do not save names that were replaced with era-specific versions
	for (int n = 0; n < 32; n++) {
		Leader * leader = &leaders[n];
		int leader_bit = 1 << leader->ID;
		if (is->aliased_civ_noun_bits        & leader_bit) leader->tribe_customization.civ_noun       [0] = '\0';
		if (is->aliased_civ_adjective_bits   & leader_bit) leader->tribe_customization.civ_adjective  [0] = '\0';
		if (is->aliased_civ_formal_name_bits & leader_bit) leader->tribe_customization.civ_formal_name[0] = '\0';
		if (is->aliased_leader_name_bits     & leader_bit) leader->tribe_customization.leader_name    [0] = '\0';
		if (is->aliased_leader_title_bits    & leader_bit) leader->tribe_customization.leader_title   [0] = '\0';
	}
	is->aliased_civ_noun_bits = is->aliased_civ_adjective_bits = is->aliased_civ_formal_name_bits = is->aliased_leader_name_bits = is->aliased_leader_title_bits = 0;

	int tr = do_save_game (file_path, param_2, guid);

	if (restore_rmr)
		p_bic_data->General.RoadsMovementRate = rmr;
	if (restore_barb_culture_group)
		p_bic_data->Races[leaders[0].RaceID].CultureGroupID = barb_culture;

	// Reapply era aliases
	for (int n = 0; n < 32; n++)
		if (*p_player_bits & (1 << n))
			apply_era_specific_names (&leaders[n]);

	return tr;
}

void
record_unit_type_alt_strategy (int type_id)
{
	int ai_strat_index; {
		int ai_strat_bits = p_bic_data->UnitTypes[type_id].AI_Strategy;
		if ((ai_strat_bits & ai_strat_bits - 1) != 0) // Sanity check: must only have one strat (one bit) set
			return;
		ai_strat_index = 0;
		while ((ai_strat_bits & 1) == 0) {
			ai_strat_index++;
			ai_strat_bits >>= 1;
		}
	}

	itable_insert (&is->unit_type_alt_strategies, type_id, ai_strat_index);
}

void
append_improv_id_to_list (struct improv_id_list * list, int id)
{
	reserve (sizeof list->items[0], (void **)&list->items, &list->capacity, list->count);
	list->items[list->count] = id;
	list->count += 1;
}

unsigned __fastcall
patch_load_scenario (void * this, int edx, char * param_1, unsigned * param_2)
{
	int ret_addr = ((int *)&param_1)[-1];

	// Destroy TNX cache from previous map. A new one will be created when needed.
	if ((is->tnx_init_state == IS_OK) && (is->tnx_cache != NULL)) {
		is->destroy_tnx_cache (is->tnx_cache);
		is->tnx_cache = NULL;
	}

	unsigned tr = load_scenario (this, __, param_1, param_2);

	// There are two cases when load_scenario is called that we don't want to run our own code. These are (1) when the scenario is loaded to
	// generate a preview (map, etc.) for the "Civ Content" or "Conquests" menu and (2) when the function is called recursively. The recursive
	// call is done, I believe, only when loading saves using the default rules. The game first tries to load custom rules then falls back on
	// loading the default rules. In any case, we want to ignore that call so we don't run the same code twice.
	if ((ret_addr == ADDR_LOAD_SCENARIO_PREVIEW_RETURN) || (ret_addr == ADDR_LOAD_SCENARIO_RESUME_SAVE_2_RETURN))
		return tr;

	reset_to_base_config ();
	load_config ("default.c3x_config.ini", 1);
	char * scenario_config_file_name = "scenario.c3x_config.ini";
	char * scenario_config_path = BIC_get_asset_path (p_bic_data, __, scenario_config_file_name, false);
	// BIC_get_asset_path returns the file name when it can't find the file
	if (0 != strcmp (scenario_config_file_name, scenario_config_path))
		load_config (scenario_config_path, 0);
	apply_machine_code_edits (&is->current_config);

	// Initialize Trade Net X
	if (is->current_config.enable_trade_net_x && (is->tnx_init_state == IS_UNINITED)) {
		char path[MAX_PATH];
		snprintf (path, sizeof path, "%s\\Trade Net X\\TradeNetX.dll", is->mod_rel_dir);
		path[(sizeof path) - 1] = '\0';
		is->trade_net_x = LoadLibraryA (path);
		if (is->trade_net_x != NULL) {
			is->set_exe_version                = (void *)(*p_GetProcAddress) (is->trade_net_x, "set_exe_version");
			is->create_tnx_cache               = (void *)(*p_GetProcAddress) (is->trade_net_x, "create_tnx_cache");
			is->destroy_tnx_cache              = (void *)(*p_GetProcAddress) (is->trade_net_x, "destroy_tnx_cache");
			is->set_up_before_building_network = (void *)(*p_GetProcAddress) (is->trade_net_x, "set_up_before_building_network");
			is->get_move_cost_for_sea_trade    = (void *)(*p_GetProcAddress) (is->trade_net_x, "get_move_cost_for_sea_trade");
			is->flood_fill_road_network        = (void *)(*p_GetProcAddress) (is->trade_net_x, "flood_fill_road_network");
			is->try_drawing_sea_trade_route    = (void *)(*p_GetProcAddress) (is->trade_net_x, "try_drawing_sea_trade_route");

			is->set_exe_version (exe_version_index);

			// Run tests
			if (0) {
				int (__stdcall * test) () = (void *)(*p_GetProcAddress) (is->trade_net_x, "test");
				int failed_test_count = test ();
				if (failed_test_count > 0)
					MessageBoxA (NULL, "Failed some tests in Trade Net X!", NULL, MB_ICONWARNING);
				else
					MessageBoxA (NULL, "All tests in Trade Net X passed.", "Success", MB_ICONINFORMATION);
			}

			is->tnx_init_state = IS_OK;
		} else {
			MessageBoxA (NULL, "Failed to load Trade Net X!", NULL, MB_ICONERROR);
			is->tnx_init_state = IS_INIT_FAILED;
		}

	// Deinitialize Trade Net X
	} else if ((! is->current_config.enable_trade_net_x) && (is->tnx_init_state == IS_OK)) {
		FreeLibrary (is->trade_net_x);
		is->trade_net_x = NULL;
		is->tnx_init_state = IS_UNINITED;
	}

	// This scenario might use different mod art assets than the old one
	deinit_stackable_command_buttons ();
	deinit_disabled_command_buttons ();
	deinit_trade_scroll_buttons ();
	deinit_unit_rcm_icons ();

	// Need to clear this since the resource count might have changed
	if (is->extra_available_resources != NULL) {
		free (is->extra_available_resources);
		is->extra_available_resources = NULL;
		is->extra_available_resources_capacity = 0;
	}

	// Similarly, these don't carry over between games
	for (int n = 0; n < 32; n++)
		is->interceptor_reset_lists[n].count = 0;
	is->replay_for_players = 0;
	table_deinit (&is->extra_defensive_bombards);

	// Clear unit type counts
	for (int n = 0; n < 32; n++)
		table_deinit (&is->unit_type_counts[n]);
	is->unit_type_count_init_bits = 0;

	// Load resources.pcx
	{
		PCX_Image * rs = is->resources_sheet;
		if (rs != NULL)
			rs->vtable->destruct (rs, __, 0);
		else
			rs = malloc (sizeof *rs);
		memset (rs, 0, sizeof *rs);
		PCX_Image_construct (rs);

		char * resources_pcx_path = BIC_get_asset_path (p_bic_data, __, "Art\\resources.pcx", true);
		PCX_Image_read_file (rs, __, resources_pcx_path, NULL, 0, 0x100, 2);
		is->resources_sheet = rs;
	}

	// Recreate table of alt strategies mapping duplicates to their strategies
	table_deinit (&is->unit_type_alt_strategies);
	for (int n = 0; n < p_bic_data->UnitTypeCount; n++) {
		int alt_for_id = p_bic_data->UnitTypes[n].alternate_strategy_for_id;
		if (alt_for_id >= 0) {
			record_unit_type_alt_strategy (n);
			record_unit_type_alt_strategy (alt_for_id); // Record the original too so we know it has alternatives
		}
	}

	// Recreate table of duplicates mapping unit types to the next duplicate
	table_deinit (&is->unit_type_duplicates);
	for (int n = 0; n < p_bic_data->UnitTypeCount; n++) {
		int alt_for_id = p_bic_data->UnitTypes[n].alternate_strategy_for_id;
		if (alt_for_id >= 0) {

			// Find the type ID of the last duplicate in the list. alt_for_id refers to the original unit that was duplicated possibly
			// multiple times. Begin searching there and, each time we find another duplicate, use its ID to continue the search. When
			// we're done last_dup_id will be set to whichever ID in the list of duplicates is not already associated with another.
			int last_dup_id = alt_for_id; {
				int next;
				while (itable_look_up (&is->unit_type_duplicates, last_dup_id, &next))
					last_dup_id = next;
			}

			// Add this unit type to the end of the list of duplicates
			itable_insert (&is->unit_type_duplicates, last_dup_id, n);
		}
	}

	// Convert charm-flagged units to using PTW targeting if necessary
	if (is->current_config.charm_flag_triggers_ptw_like_targeting) {
		struct c3x_config * cc = &is->current_config;

		for (int n = 0; n < p_bic_data->UnitTypeCount; n++) {
			UnitType * type = &p_bic_data->UnitTypes[n];
			if (type->Special_Actions & UCV_Charm_Bombard) {
				// Add this type ID to the PTW targeting list
				reserve (sizeof cc->ptw_arty_types[0], (void **)&cc->ptw_arty_types, &cc->ptw_arty_types_capacity, cc->count_ptw_arty_types);
				cc->ptw_arty_types[cc->count_ptw_arty_types] = n;
				cc->count_ptw_arty_types++;

				// Clear the charm flag, taking care not to clear the category bit. This is necessary for the PTW targeting to work
				// since the logic to implement it only applies to the non-charm code path. It wouldn't make sense to have both charm
				// attack and PTW targeting anyway, since charm attack already works that way vs cities.
				type->Special_Actions &= ~(0x00FFFFFF & UCV_Charm_Bombard);
			}
		}
	}

	// Pick out which resources are used as mill inputs
	if (is->mill_input_resource_bits)
		free (is->mill_input_resource_bits);
	is->mill_input_resource_bits = calloc (1, 1 + p_bic_data->ResourceTypeCount / 8);
	for (int n = 0; n < is->current_config.count_mills; n++) {
		struct mill * mill = &is->current_config.mills[n];
		for (int k = 0; k < 2; k++) {
			int resource_id = (&p_bic_data->Improvements[mill->improv_id].Resource1ID)[k];
			if ((resource_id >= 0) && (resource_id < p_bic_data->ResourceTypeCount)) {
				byte bit = 1 << (resource_id & 7);
				is->mill_input_resource_bits[resource_id>>3] |= bit;
			}
		}
	}

	// Recreate lists of water & air trade improvements
	is->water_trade_improvs   .count = 0;
	is->air_trade_improvs     .count = 0;
	is->combat_defense_improvs.count = 0;
	for (int n = 0; n < p_bic_data->ImprovementsCount; n++) {
		enum ImprovementTypeFlags flags = p_bic_data->Improvements[n].ImprovementFlags;
		if (flags & ITF_Allows_Water_Trade)                  append_improv_id_to_list (&is->water_trade_improvs   , n);
		if (flags & ITF_Allows_Air_Trade)                    append_improv_id_to_list (&is->air_trade_improvs     , n);
		if (p_bic_data->Improvements[n].Combat_Defence != 0) append_improv_id_to_list (&is->combat_defense_improvs, n);
	}

	// Set up for limiting railroad movement, if enabled
	if (is->current_config.limit_railroad_movement > 0) {
		int base_rmr = p_bic_data->General.RoadsMovementRate;
		int g = gcd (base_rmr, is->current_config.limit_railroad_movement); // Scale down all MP costs by this common divisor to help against
										    // overflow of 8-bit integers inside the pathfinder.
		is->saved_road_movement_rate = base_rmr;
		p_bic_data->General.RoadsMovementRate = base_rmr * is->current_config.limit_railroad_movement / g; // Full move in MP
		is->road_mp_cost = is->current_config.limit_railroad_movement / g;
		is->railroad_mp_cost_per_move = base_rmr / g;
	} else {
		is->saved_road_movement_rate = -1;
		is->road_mp_cost = 1;
		is->railroad_mp_cost_per_move = 0;
	}

	// If barb city capturing is enabled and the barbs have the non-existent "none" culture group (index -1), switch them to the first real
	// culture group. The "none" group produces corrupt graphics and crashes.
	int * barb_culture_group = &p_bic_data->Races[leaders[0].RaceID].CultureGroupID;
	if (is->current_config.enable_city_capture_by_barbarians && (*barb_culture_group < 0)) {
		is->saved_barb_culture_group = *barb_culture_group;
		*barb_culture_group = 0;
	} else
		is->saved_barb_culture_group = -1;

	// Clear old alias bits
	is->aliased_civ_noun_bits = is->aliased_civ_adjective_bits = is->aliased_civ_formal_name_bits = is->aliased_leader_name_bits = is->aliased_leader_title_bits = 0;

	return tr;
}

void __fastcall
patch_Leader_recompute_auto_improvements (Leader * this)
{
	is->leader_param_for_patch_get_wonder_city_id = this;
	Leader_recompute_auto_improvements (this);
}

int __fastcall
patch_get_wonder_city_id (void * this, int edx, int wonder_improvement_id)
{
	int ret_addr = ((int *)&wonder_improvement_id)[-1];
	if ((is->current_config.enable_free_buildings_from_small_wonders) && (ret_addr == ADDR_SMALL_WONDER_FREE_IMPROVS_RETURN)) {
		Leader * leader = is->leader_param_for_patch_get_wonder_city_id;
		Improvement * improv = &p_bic_data->Improvements[wonder_improvement_id];
		if ((improv->Characteristics & ITC_Small_Wonder) != 0) {
			// Need to check if Small_Wonders array is NULL b/c recompute_auto_improvements gets called with leaders that are absent/dead.
			return (leader->Small_Wonders != NULL) ? leader->Small_Wonders[wonder_improvement_id] : -1;
		}
	}
	return get_wonder_city_id (this, __, wonder_improvement_id);
}

int __fastcall
patch_Main_Screen_Form_handle_key_up (Main_Screen_Form * this, int edx, int virtual_key_code)
{
	if ((virtual_key_code & 0xFF) == VK_CONTROL)
		patch_Main_GUI_set_up_unit_command_buttons (&this->GUI);

	return Main_Screen_Form_handle_key_up (this, __, virtual_key_code);
}

char __fastcall
patch_Leader_can_do_worker_job (Leader * this, int edx, enum Worker_Jobs job, int tile_x, int tile_y, int ask_if_replacing)
{
	if ((p_main_screen_form->Player_CivID != this->ID) || (! is->current_config.skip_repeated_tile_improv_replacement_asks))
		return Leader_can_do_worker_job (this, __, job, tile_x, tile_y, ask_if_replacing);
	else if (is->have_job_and_loc_to_skip &&
		 (is->to_skip.job == job) && (is->to_skip.tile_x == tile_x) && (is->to_skip.tile_y == tile_y))
		return Leader_can_do_worker_job (this, __, job, tile_x, tile_y, 0);
	else {
		is->show_popup_was_called = 0;
		int tr = Leader_can_do_worker_job (this, __, job, tile_x, tile_y, ask_if_replacing);
		if (is->show_popup_was_called && tr) { // Check that the popup was shown and the playeer chose to replace
			is->to_skip = (struct worker_job_and_location) { .job = job, .tile_x = tile_x, .tile_y = tile_y };
			is->have_job_and_loc_to_skip = 1;
		}
		return tr;
	}
}

bool __fastcall
patch_Unit_can_hurry_production (Unit * this, int edx, City * city, bool exclude_cheap_improvements)
{
	if (is->current_config.allow_military_leaders_to_hurry_wonders && Unit_has_ability (this, __, UTA_Leader)) {
		LeaderKind actual_kind = this->Body.leader_kind;
		this->Body.leader_kind = LK_Scientific;
		bool tr = Unit_can_hurry_production (this, __, city, exclude_cheap_improvements);
		this->Body.leader_kind = actual_kind;
		return tr;
	} else
		return Unit_can_hurry_production (this, __, city, exclude_cheap_improvements);
}

bool
any_enemies_near (Leader const * me, int tile_x, int tile_y, enum UnitTypeClasses class, int num_tiles)
{
	bool tr = false;
	FOR_TILES_AROUND (tai, num_tiles, tile_x, tile_y) {
		int enemy_on_this_tile = 0;
		FOR_UNITS_ON (uti, tai.tile) {
			UnitType const * unit_type = &p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID];
			if (patch_Unit_is_visible_to_civ (uti.unit, __, me->ID, 0) &&
			    (((int)class < 0) || (unit_type->Unit_Class == class))) {
				if (me->At_War[uti.unit->Body.CivID]) {
					if ((unit_type->Defence > 0) || (unit_type->Attack > 0)) {
						enemy_on_this_tile = 1;
						break;
					}
				} else
					break;
			}
		}
		if (enemy_on_this_tile) {
			tr = true;
			break;
		}
	}
	return tr;
}

bool
any_enemies_near_unit (Unit * unit, int num_tiles)
{
	UnitType * unit_type = &p_bic_data->UnitTypes[unit->Body.UnitTypeID];
	return any_enemies_near (&leaders[unit->Body.CivID], unit->Body.X, unit->Body.Y, unit_type->Unit_Class, num_tiles);
}

void __fastcall
patch_Unit_ai_move_artillery (Unit * this)
{
	if ((! is->current_config.use_offensive_artillery_ai) ||
	    ((this->Body.UnitTypeID < 0) || (this->Body.UnitTypeID >= p_bic_data->UnitTypeCount))) // Check for invalid unit type id which appears sometimes, IDK why
		goto base_impl;

	Tile * on_tile = tile_at (this->Body.X, this->Body.Y);
	City * in_city = get_city_ptr (on_tile->vtable->m45_Get_City_ID (on_tile));
	UnitType const * this_type = &p_bic_data->UnitTypes[this->Body.UnitTypeID];
	int num_escorters_req = this->vtable->eval_escort_requirement (this);

	if ((in_city == NULL) || (count_escorters (this) >= num_escorters_req))
		goto base_impl;

	// Don't assign escort if there are any enemies around because in that case it might be a serious mistake to take a defender out of the city
	if (any_enemies_near_unit (this, 37))
		goto base_impl;

	// Find the strongest healthy defender the city has and assign that unit as an escort but make sure doing so doesn't leave the city
	// defenseless. I think picking the strongest defender is the right choice here because the artillery pair is more likely to come under
	// attack than a city and also leaving obsolete units in the city gives them a chance to upgrade.
	int num_defenders = 0;
	Unit * best_defender = NULL;
	int best_defender_strength = -1;
	FOR_UNITS_ON (uti, on_tile) {
		Unit_Body * body = &uti.unit->Body;
		UnitType * type = &p_bic_data->UnitTypes[body->UnitTypeID];
		if ((type->AI_Strategy & UTAI_Defence) &&
		    (! UnitType_has_ability (type, __, UTA_Immobile)) &&
		    (body->Damage == 0) &&
		    ((body->UnitState == 0) || (body->UnitState == UnitState_Fortifying)) &&
		    (body->escortee < 0)) {
			num_defenders++;
			int str = type->Defence * Unit_get_max_hp (uti.unit);
			if (str > best_defender_strength) {
				best_defender = uti.unit;
				best_defender_strength = str;
			}
		}
	}
	if ((num_defenders >= 2) && (best_defender != NULL)) {
		Unit_set_state (best_defender, __, 0);
		Unit_set_escortee (best_defender, __, this->Body.ID);
	}

base_impl:
	Unit_ai_move_artillery (this);

	// Recompute these since the unit might have moved
	on_tile = tile_at (this->Body.X, this->Body.Y);
	in_city = get_city_ptr (on_tile->vtable->m45_Get_City_ID (on_tile));

	// Load the unit into a transport for a naval invasion if it's just sitting in a city with nothing else to do
	if (is->current_config.use_offensive_artillery_ai &&
	    (in_city != NULL) &&
	    (this->Body.Moves == 0) &&
	    (this->Body.UnitState == UnitState_Fortifying) &&
	    (this->Body.Container_Unit < 0)) {
		Unit * transport = Unit_find_transport (this, __, this->Body.X, this->Body.Y);
		if (transport != NULL) {

			int transport_capacity = p_bic_data->UnitTypes[transport->Body.UnitTypeID].Transport_Capacity;
			int units_in_transport, arty_in_transport; {
				units_in_transport = arty_in_transport = 0;
				FOR_UNITS_ON (uti, on_tile)
					if (uti.unit->Body.Container_Unit == transport->Body.ID) {
						units_in_transport++;
						arty_in_transport += (p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID].AI_Strategy & UTAI_Artillery) != 0;
					}
			}

			// Check that there's space for the arty unit and an escort, and that the transport does not have more than one arty per three
			// spaces so we're less likely to assemble invasion forces composed of nothing but artillery.
			if ((units_in_transport + 2 <= transport_capacity) &&
			    (arty_in_transport < not_below (1, transport_capacity / 3))) {
				Unit_set_escortee (this, __, -1);
				Unit_load (this, __, transport);
			}
		}
	}
}

// Estimates the number of turns necessary to move the given unit to the given tile and writes it to *out_num_turns. Returns 0 if there is no path
// from the unit's current position to the given tile, 1 otherwise.
int
estimate_travel_time (Unit * unit, int to_tile_x, int to_tile_y, int * out_num_turns)
{
	int dist_in_mp;
	patch_Trade_Net_set_unit_path (p_trade_net, __, unit->Body.X, unit->Body.Y, to_tile_x, to_tile_y, unit, unit->Body.CivID, 1, &dist_in_mp);
	dist_in_mp += unit->Body.Moves; // Add MP already spent this turn to the distance
	int max_mp = Unit_get_max_move_points (unit);
	if ((dist_in_mp >= 0) && (max_mp > 0)) {
		*out_num_turns = dist_in_mp / max_mp;
		return 1;
	} else
		return 0; // No path or unit cannot move
}

City *
find_nearest_established_city (Unit * unit, int continent_id)
{
	int lowest_unattractiveness = INT_MAX;
	City * least_unattractive_city = NULL;
	FOR_CITIES_OF (coi,unit->Body.CivID) {
		Tile * city_tile = tile_at (coi.city->Body.X, coi.city->Body.Y);
		if (continent_id == city_tile->vtable->m46_Get_ContinentID (city_tile)) {
			int dist_in_turns;
			if (! estimate_travel_time (unit, coi.city->Body.X, coi.city->Body.Y, &dist_in_turns))
				continue;
			int unattractiveness = 10 * dist_in_turns;
			unattractiveness = (10 + unattractiveness) / (10 + coi.city->Body.Population.Size);
			if (coi.city->Body.CultureIncome > 0)
				unattractiveness /= 5;
			if (unattractiveness < lowest_unattractiveness) {
				lowest_unattractiveness = unattractiveness;
				least_unattractive_city = coi.city;
			}
		}
	}
	return least_unattractive_city;
}

void __fastcall
patch_Unit_ai_move_leader (Unit * this)
{
	if (! is->current_config.replace_leader_unit_ai) {
		Unit_ai_move_leader (this);
		return;
	}

	Tile * tile = tile_at (this->Body.X, this->Body.Y);
	int continent_id = tile->vtable->m46_Get_ContinentID (tile);

	// Disband the unit if it's on a continent with no cities of its civ. This is what the original logic does.
	if (leaders[this->Body.CivID].city_count_per_cont[continent_id] == 0) {
		Unit_disband (this);
		return;
	}

	// Flee if the unit is near an enemy without adequate escort
	int has_adequate_escort; {
		int escorter_count = 0;
		int any_healthy_escorters = 0;
		int index;
		for (int escorter_id = Unit_next_escorter_id (this, __, &index, true); escorter_id >= 0; escorter_id = Unit_next_escorter_id (this, __, &index, false)) {
			Unit * escorter = get_unit_ptr (escorter_id);
			if ((escorter != NULL) && (escorter->Body.X == this->Body.X) && (escorter->Body.Y == this->Body.Y)) {
				escorter_count++;
				int remaining_health = Unit_get_max_hp (escorter) - escorter->Body.Damage;
				any_healthy_escorters |= (remaining_health >= 3) || (escorter->Body.Damage == 0);
			}
		}
		has_adequate_escort = (escorter_count > 0) && any_healthy_escorters;
	}
	if ((! has_adequate_escort) && any_enemies_near_unit (this, 49)) {
		Unit_set_state (this, __, UnitState_Fleeing);
		bool done = this->vtable->work (this);
		if (done || (this->Body.UnitState != 0))
			return;
	}

	// Move along path if the unit already has one set
	byte next_move = Unit_pop_next_move_from_path (this);
	if (next_move > 0) {
		this->vtable->Move (this, __, next_move, 0);
		return;
	}

	// Start a science age if we can
	// It would be nice to compute a value for this and compare it to the option of rushing production but it's hard to come up with a valuation
	if (is->current_config.patch_science_age_bug && Unit_ai_can_start_science_age (this)) {
		Unit_start_science_age (this);
		return;
	}

	// Estimate the value of creating an army. First test if that's even a possiblity and if not leave the value at -1 so rushing production is
	// always preferable (note we can't use Unit_ai_can_form_army for this b/c it returns false for units not in a city). The value of forming
	// an army is "infinite" if we don't already have one and otherwise it depends on the army unit's shield cost +/- 25% for each point of
	// aggression divided by the number of armies already in the field.
	int num_armies = leaders[this->Body.CivID].Armies_Count;
	int form_army_value = -1;
	if ((this->Body.leader_kind & LK_Military) &&
	    ((num_armies + 1) * p_bic_data->General.ArmySupportCities <= leaders[this->Body.CivID].Cities_Count) &&
	    (p_bic_data->General.BuildArmyUnitID >= 0) &&
	    (p_bic_data->General.BuildArmyUnitID < p_bic_data->UnitTypeCount)) {
		if (num_armies < 1)
			form_army_value = INT_MAX;
		else {
			form_army_value = p_bic_data->UnitTypes[p_bic_data->General.BuildArmyUnitID].Cost;
			int aggression_level = p_bic_data->Races[leaders[this->Body.CivID].RaceID].AggressionLevel; // aggression level varies between -2 and +2
			form_army_value = (form_army_value * (4 + aggression_level)) / 4;
			if (num_armies > 1)
				form_army_value /= num_armies;
		}
	}

	// Estimate the value of rushing production in every city on this continent and remember the highest one
	City * best_rush_loc = NULL;
	int best_rush_value = -1;
	FOR_CITIES_OF (coi, this->Body.CivID) {
		City * city = coi.city;
		Tile * city_tile = tile_at (city->Body.X, city->Body.Y);
		if ((continent_id == city_tile->vtable->m46_Get_ContinentID (city_tile)) &&
		    patch_Unit_can_hurry_production (this, __, city, false)) {
			// Base value is equal to the number of shields rushing would save
			int value = City_get_order_cost (city) - City_get_order_progress (city) - city->Body.ProductionIncome;
			if (value <= 0)
				continue;

			// Consider distance: Reduce the value by the number of shields produced while the leader is en route to rush.
			int dist_in_turns;
			if (! estimate_travel_time (this, city->Body.X, city->Body.Y, &dist_in_turns))
				continue; // no path or unit cannot move
			value -= dist_in_turns * city->Body.ProductionIncome;
			if (value <= 0)
				continue;

			// Consider corruption: Scale down the value of rushing an improvement by the corruption rate of the city.
			// This is to reflect the fact that infrastructure is more valuable in less corrupt cities. But do not apply
			// this to wonders since their benefit is in most cases not lessened by local corruption.
			Improvement * improv = (city->Body.Order_Type == COT_Improvement) ? &p_bic_data->Improvements[city->Body.Order_ID] : NULL;
			int is_wonder = (improv != NULL) && (0 != (improv->Characteristics & (ITC_Small_Wonder | ITC_Wonder)));
			if ((improv != NULL) && (! is_wonder)) {
				int good_shields = city->Body.ProductionIncome;
				int corrupt_shields = city->Body.ProductionLoss;
				if (good_shields + corrupt_shields > 0)
					value = (value * good_shields) / (good_shields + corrupt_shields);
				else
					continue;
			}

			if ((value > 0) && (value > best_rush_value)) {
				best_rush_loc = city;
				best_rush_value = value;
			}
		}
	}

	// Hurry production or form an army depending on the estimated values of doing so. We might have to move to a (different) city if that's where
	// we want to rush production or if we want to form an army but aren't already in a city.
	City * in_city = get_city_ptr (tile->vtable->m45_Get_City_ID (tile));
	City * moving_to_city = NULL;
	if ((best_rush_loc != NULL) && (best_rush_value > form_army_value)) {
		if (best_rush_loc == in_city) {
			Unit_hurry_production (this);
			return;
		} else
			moving_to_city = best_rush_loc;
	} else if ((form_army_value > -1) && (Unit_ai_can_form_army (this))) {
		Unit_form_army (this);
		return;
	} else if (in_city == NULL) {
		// Nothing to do. Try to find a close, established city to move to & wait in.
		moving_to_city = find_nearest_established_city (this, continent_id);
	}
	if (moving_to_city) {
		int first_move = patch_Trade_Net_set_unit_path (p_trade_net, __, this->Body.X, this->Body.Y, moving_to_city->Body.X, moving_to_city->Body.Y, this, this->Body.CivID, 0x101, NULL);
		if (first_move > 0) {
			Unit_set_escortee (this, __, -1);
			this->vtable->Move (this, __, first_move, 0);
			return;
		}
	}

	// Nothing to do, nowhere to go, just fortify in place.
	Unit_set_escortee (this, __, -1);
	Unit_set_state (this, __, UnitState_Fortifying);
}

int
measure_strength_in_army (UnitType * type)
{
	return ((type->Attack + type->Defence) * (4 + type->Hit_Point_Bonus)) / 4;
}

bool __fastcall
patch_impl_ai_is_good_army_addition (Unit * this, int edx, Unit * candidate)
{
	if (! is->current_config.fix_ai_army_composition)
		return impl_ai_is_good_army_addition (this, __, candidate);

	UnitType * candidate_type = &p_bic_data->UnitTypes[candidate->Body.UnitTypeID];
	Tile * tile = tile_at (this->Body.X, this->Body.Y);

	if (((candidate_type->AI_Strategy & UTAI_Offence) == 0) ||
	    UnitType_has_ability (candidate_type, __, UTA_Hidden_Nationality))
		return false;

	int num_units_in_army = 0,
	    army_min_speed = INT_MAX,
	    army_min_strength = INT_MAX;
	FOR_UNITS_ON (uti, tile) {
		if (uti.unit->Body.Container_Unit == this->Body.ID) {
			num_units_in_army++;
			int movement = p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID].Movement;
			if (movement < army_min_speed)
				army_min_speed = movement;
			int member_strength = measure_strength_in_army (&p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID]);
			if (member_strength < army_min_strength)
				army_min_strength = member_strength;
		}
	}

	return (num_units_in_army == 0) ||
		((candidate_type->Movement >= army_min_speed) &&
		 (measure_strength_in_army (candidate_type) >= army_min_strength));
}

int
rate_artillery (UnitType * type)
{
	int tr = type->Attack + type->Defence + 2 * type->Bombard_Strength * type->FireRate;

	// include movement
	int moves = type->Movement;
	if (moves >= 2)
		tr = tr * (moves + 1) / 2;

	// include range
	int range = type->Bombard_Range;
	if (range >= 2)
		tr = tr * (range + 1) / 2;

	// include extra hp
	if (type->Hit_Point_Bonus > 0)
		tr = tr * (4 + type->Hit_Point_Bonus) / 4;

	return tr;
}

int
rate_bomber (UnitType * type)
{
	int tr = type->Bombard_Strength * type->FireRate + type->Defence;

	// include range
	tr = tr * (10 + type->OperationalRange) / 10;

	// include cost
	tr = (tr * 100) / (100 + type->Cost);

	// include abilities
	if (UnitType_has_ability (type, __, UTA_Blitz))
		tr = tr * (type->Movement + 1) / 2;
	if (UnitType_has_ability (type, __, UTA_Lethal_Land_Bombardment))
		tr = tr * 3 / 2;
	if (UnitType_has_ability (type, __, UTA_Lethal_Sea_Bombardment))
		tr = tr * 5 / 4;
	if (UnitType_has_ability (type, __, UTA_Stealth))
		tr = tr * 3 / 2;

	// include extra hp
	if (type->Hit_Point_Bonus > 0)
		tr = tr * (4 + type->Hit_Point_Bonus) / 4;

	return tr;
}

bool __fastcall
patch_City_can_build_unit (City * this, int edx, int unit_type_id, bool exclude_upgradable, int param_3, bool allow_kings)
{
	bool base = City_can_build_unit (this, __, unit_type_id, exclude_upgradable, param_3, allow_kings);

	// Apply building prereqs
	if (base) {
		int building_prereq;
		if (itable_look_up (&is->current_config.building_unit_prereqs, unit_type_id, &building_prereq)) {
			// If the prereq is an encoded building ID
			if (building_prereq & 1)
				return has_active_building (this, building_prereq >> 1);

			// Else it's a pointer to a list of building IDs
			else {
				int * list = (int *)building_prereq;
				for (int n = 0; n < MAX_BUILDING_PREREQS_FOR_UNIT; n++)
					if ((list[n] >= 0) && ! has_active_building (this, list[n]))
						return false;
			}
		}
	}

	// Apply unit type limit
	int available;
	if (get_available_unit_count (&leaders[this->Body.CivID], unit_type_id, &available) && (available <= 0))
		return false;

	return base;
}

void __fastcall
patch_City_ai_choose_production (City * this, int edx, City_Order * out)
{
	is->ai_considering_production_for_city = this;

	City_ai_choose_production (this, __, out);

	Leader * me = &leaders[this->Body.CivID];
	int arty_ratio = is->current_config.ai_build_artillery_ratio;
	int bomber_ratio = is->current_config.ai_build_bomber_ratio;

	// Check if AI-build-more-artillery mod option is activated and this city is building something that we might want to switch to artillery
	if ((arty_ratio > 0) &&
	    (out->OrderType == COT_Unit) &&
	    (out->OrderID >= 0) && (out->OrderID < p_bic_data->UnitTypeCount) &&
	    (p_bic_data->UnitTypes[out->OrderID].AI_Strategy & (UTAI_Offence | UTAI_Defence))) {

		// Check how many offense/defense/artillery units this AI has already built. We'll only force it to build arty if it has a minimum
		// of one defender per city, one attacker per two cities, and of course if it's below the ratio limit.
		int num_attackers = me->AI_Strategy_Unit_Counts[0],
		    num_defenders = me->AI_Strategy_Unit_Counts[1],
		    num_artillery = me->AI_Strategy_Unit_Counts[2];
		if ((num_attackers > me->Cities_Count / 2) &&
		    (num_defenders > me->Cities_Count) &&
		    (100*num_artillery < arty_ratio*(num_attackers + num_defenders))) {

			// Loop over all build options to determine the best artillery unit available. Record its attack power and also find & record
			// the highest attack power available from any offensive unit so we can compare them.
			int best_arty_type_id = -1,
			    best_arty_rating = -1,
			    best_arty_strength = -1,
			    best_attacker_strength = -1;
			for (int n = 0; n < p_bic_data->UnitTypeCount; n++) {
				UnitType * type = &p_bic_data->UnitTypes[n];
				if ((type->AI_Strategy & (UTAI_Artillery | UTAI_Offence)) &&
				    patch_City_can_build_unit (this, __, n, 1, 0, 0)) {
					if (type->AI_Strategy & UTAI_Artillery) {
						int this_rating = rate_artillery (&p_bic_data->UnitTypes[n]);
						if (this_rating > best_arty_rating) {
							best_arty_type_id = n;
							best_arty_rating = this_rating;
							best_arty_strength = type->Bombard_Strength * type->FireRate;
						}
					} else { // attacker
						int this_strength = (type->Attack * (4 + type->Hit_Point_Bonus)) / 4;
						if (this_strength > best_attacker_strength)
							best_attacker_strength = this_strength;
					}
				}
			}

			// Randomly switch city production to the artillery unit if we found one
			if (best_arty_type_id >= 0) {
				int chance = 12 * arty_ratio / 10;

				// Scale the chance of switching by the ratio of the attack power the artillery would provide to the attack power
				// of the strongest offensive unit. This way the AI will rapidly build artillery up to the ratio limit only when
				// artillery are its best way of dealing damage.
				// Some example numbers:
				// | Best attacker (str) | Best arty (str) | Chance w/ ratio = 20, damage% = 50
				// | Swordsman (3)       | Catapult (4)    | 16
				// | Knight (4)          | Trebuchet (6)   | 18
				// | Cavalry (6)         | Cannon (8)      | 16
				// | Cavalry (6)         | Artillery (24)  | 48
				// | Tank (16)           | Artillery (24)  | 18
				if (best_attacker_strength > 0)
					chance = (chance * best_arty_strength * is->current_config.ai_artillery_value_damage_percent) / (best_attacker_strength * 100);

				if (rand_int (p_rand_object, __, 100) < chance)
					out->OrderID = best_arty_type_id;
			}
		}

	} else if ((bomber_ratio > 0) &&
		   (out->OrderType == COT_Unit) &&
		   (out->OrderID >= 0) && (out->OrderID < p_bic_data->UnitTypeCount) &&
		   (p_bic_data->UnitTypes[out->OrderID].AI_Strategy & UTAI_Air_Defence)) {
		int num_fighters = me->AI_Strategy_Unit_Counts[7],
		    num_bombers = me->AI_Strategy_Unit_Counts[6];
		if (100 * num_bombers < bomber_ratio * num_fighters) {
			int best_bomber_type_id = -1,
			    best_bomber_rating = -1;
			for (int n = 0; n < p_bic_data->UnitTypeCount; n++) {
				UnitType * type = &p_bic_data->UnitTypes[n];
				if ((type->AI_Strategy & UTAI_Air_Bombard) &&
				    patch_City_can_build_unit (this, __, n, 1, 0, 0)) {
					int this_rating = rate_bomber (&p_bic_data->UnitTypes[n]);
					if (this_rating > best_bomber_rating) {
						best_bomber_type_id = n;
						best_bomber_rating = this_rating;
					}
				}
			}

			if (best_bomber_type_id >= 0) {
				int chance = 12 * bomber_ratio / 10;
				if (rand_int (p_rand_object, __, 100) < chance)
					out->OrderID = best_bomber_type_id;
			}
		}
	}
}

int __fastcall
patch_Unit_disembark_passengers (Unit * this, int edx, int tile_x, int tile_y)
{
	// Break any escort relationships if the escorting unit can't move onto the target tile. This prevents a freeze where the game continues
	// trying to disembark units because an escortee looks like it could be disembarked except it can't because it won't move if its escorter
	// can't be moved first.
	Tile * tile   = tile_at (this->Body.X, this->Body.Y),
	     * target = tile_at (tile_x      , tile_y);
	if (is->current_config.patch_blocked_disembark_freeze && (tile != NULL) && (target != NULL)) {
		enum SquareTypes target_terrain = target->vtable->m50_Get_Square_BaseType (target);
		FOR_UNITS_ON (uti, tile) {
			Unit * escortee = get_unit_ptr (uti.unit->Body.escortee);
			if (   (escortee != NULL)
			    && (uti.unit->Body.Container_Unit == this->Body.ID)
			    && (escortee->Body.Container_Unit == this->Body.ID)
			    && (   UnitType_has_ability (&p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID], __, UTA_Immobile)
				|| (   is->current_config.disallow_trespassing
				    && check_trespassing (uti.unit->Body.CivID, tile, target)
				    && ! is_allowed_to_trespass (uti.unit))
				|| Unit_is_terrain_impassable (uti.unit, __, target_terrain)))
				Unit_set_escortee (uti.unit, __, -1);
		}
	}

	return Unit_disembark_passengers (this, __, tile_x, tile_y);
}

// Returns whether or not the current deal being offered to the AI on the trade screen would be accepted. How advantageous the AI thinks the
// trade is for itself is stored in out_their_advantage if it's not NULL. This advantage is measured in gold, if it's positive it means the
// AI thinks it's gaining that much value from the trade and if it's negative it thinks it would be losing that much. I don't know what would
// happen if this function were called while the trade screen is not active.
bool
is_current_offer_acceptable (int * out_their_advantage)
{
	int their_id = p_diplo_form->other_party_civ_id;

	DiploMessage consideration = Leader_consider_trade (
		&leaders[their_id],
		__,
		&p_diplo_form->our_offer_lists[their_id],
		&p_diplo_form->their_offer_lists[their_id],
		p_main_screen_form->Player_CivID,
		0, true, 0, 0,
		out_their_advantage,
		NULL, NULL);

	return consideration == DM_AI_ACCEPT;
}

// Adds an offer of gold to the list and returns it. If one already exists in the list, returns a pointer to it.
TradeOffer *
offer_gold (TradeOfferList * list, int is_lump_sum, int * is_new_offer)
{
	if (list->length > 0)
		for (TradeOffer * offer = list->first; offer != NULL; offer = offer->next)
			if ((offer->kind == 7) && (offer->param_1 == is_lump_sum)) {
				*is_new_offer = 0;
				return offer;
			}

	TradeOffer * tr = new (sizeof *tr);
	*tr = (struct TradeOffer) {
		.vtable = p_trade_offer_vtable,
		.kind = 7, // TODO: Replace with enum
		.param_1 = is_lump_sum,
		.param_2 = 0,
		.next = NULL,
		.prev = NULL
	};

	if (list->length > 0) {
		tr->prev = list->last;
		list->last->next = tr;
		list->last = tr;
		list->length += 1;
	} else {
		list->last = list->first = tr;
		list->length = 1;
	}

	*is_new_offer = 1;
	return tr;
}

// Removes offer from list of offers but does not free it. Assumes offer is in the list.
void
remove_offer (TradeOfferList * list, TradeOffer * offer)
{
	if (list->length == 1) {
		list->first = list->last = NULL;
		list->length = 0;
	} else if (list->length > 1) {
		TradeOffer * prev = offer->prev, * next = offer->next;
		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;
		if (list->first == offer)
			list->first = next;
		if (list->last == offer)
			list->last = prev;
		list->length -= 1;
	}
	offer->prev = offer->next = NULL;
}

void __fastcall
patch_PopupForm_set_text_key_and_flags (PopupForm * this, int edx, char * script_path, char * text_key, int param_3, int param_4, int param_5, int param_6)
{
	int * p_stack = (int *)&script_path;
	int ret_addr = p_stack[-1];

	int is_initial_gold_trade   = (ret_addr == ADDR_SETUP_INITIAL_GOLD_ASK_RETURN) || (ret_addr == ADDR_SETUP_INITIAL_GOLD_OFFER_RETURN),
	    is_modifying_gold_trade = (ret_addr == ADDR_SETUP_MODIFY_GOLD_ASK_RETURN ) || (ret_addr == ADDR_SETUP_MODIFY_GOLD_OFFER_RETURN);

	// This function gets called from all over the place, check that it's being called to setup the set gold amount popup in the trade screen
	if (is->current_config.autofill_best_gold_amount_when_trading && (is_initial_gold_trade || is_modifying_gold_trade)) {
		int asking = (ret_addr == ADDR_SETUP_INITIAL_GOLD_ASK_RETURN) || (ret_addr == ADDR_SETUP_MODIFY_GOLD_ASK_RETURN);
		int is_lump_sum = is_initial_gold_trade ?
			p_stack[TRADE_GOLD_SETTER_IS_LUMP_SUM_OFFSET] : // Read this variable from the caller's frame
			is->modifying_gold_trade->param_1;

		int their_id = p_diplo_form->other_party_civ_id,
		    our_id = p_main_screen_form->Player_CivID;

		// This variable will store the result of the optimization process and it's what will be used as the final amount presented to the
		// player in the popup and left in the TradeOffer object (if applicable). Default to zero for new offers and the previous amount when
		// modifying an offer. When modifying, we also zero the amount on the table b/c the optimization process only works properly from that
		// starting point.
		int best_amount = 0;
		if (is_modifying_gold_trade) {
			best_amount = is->modifying_gold_trade->param_2;
			is->modifying_gold_trade->param_2 = 0;
		}

		int their_advantage;
		bool is_original_acceptable = is_current_offer_acceptable (&their_advantage);

		// if (we're asking for money on an acceptable trade and are offering something) OR (we're offering money on an unacceptable trade and
		// are asking for something)
		if ((   asking  &&    is_original_acceptable  && (p_diplo_form->our_offer_lists[their_id]  .length > 0)) ||
		    ((! asking) && (! is_original_acceptable) && (p_diplo_form->their_offer_lists[their_id].length > 0))) {

			TradeOfferList * offers = asking ? &p_diplo_form->their_offer_lists[their_id] : &p_diplo_form->our_offer_lists[their_id];
			int test_offer_is_new;
			TradeOffer * test_offer = offer_gold (offers, is_lump_sum, &test_offer_is_new);

			// When asking for gold, start at zero and work upwards. When offering gold it's more complicated. For lump sum
			// offers, start with our entire treasury and work downward. For GPT offers, start with an upper bound and work
			// downward. The upper bound depends on how much the AI thinks it's losing on the trade (in gold) divided by 20
			// (b/c 20 turn deal) with a lot of extra headroom just to make sure.
			int starting_amount; {
				if (asking)
					starting_amount = 0;
				else {
					if (is_lump_sum)
						starting_amount = leaders[our_id].Gold_Encoded + leaders[our_id].Gold_Decrement;
					else {
						int guess = not_below (0, 0 - their_advantage) / 20;
						starting_amount = 10 + guess * 2;
					}
				}
			}

			// Check if optimization is still possible. It's not if we're offering gold and our maximum offer is unacceptable
			test_offer->param_2 = starting_amount;
			if (asking || is_current_offer_acceptable (NULL)) {

				best_amount = starting_amount;
				for (int step_size = asking ? 1000 : -1000; step_size != 0; step_size /= 10) {
					test_offer->param_2 = best_amount;
					while (1) {
						test_offer->param_2 += step_size;
						if (test_offer->param_2 < 0)
							break;
						else if (is_current_offer_acceptable (NULL))
							best_amount = test_offer->param_2;
						else
							break;
					}
				}
			}

			// Annoying little edge case: The limitation on AIs not to trade more than their entire treasury is handled in the
			// interface not the trade evaluation logic so we have to limit our gold request here to their treasury (otherwise
			// the amount will default to how much they would pay if they had infinite money).
			int their_treasury = leaders[their_id].Gold_Encoded + leaders[their_id].Gold_Decrement;
			if (asking && is_lump_sum && (best_amount > their_treasury))
				best_amount = their_treasury;

			// Restore the trade table to its original state. Remove & free test_offer if it was newly created, otherwise put back its
			// original amount.
			if (test_offer_is_new) {
				remove_offer (offers, test_offer);
				test_offer->vtable->destruct (test_offer, __, 1);
			}

		// If we're offering gold on a trade that's already acceptable, the optimal amount is zero. We must handle this explicitly in case
		// we're modifying an amount already on the table. If we didn't, the above condition wouldn't optimize the amount and we'd default to
		// the original amount.
		} else if ((! asking) && is_original_acceptable)
			best_amount = 0;

		if (is_modifying_gold_trade)
			is->modifying_gold_trade->param_2 = best_amount;
		snprintf (is->ask_gold_default, sizeof is->ask_gold_default, "%d", best_amount);
		is->ask_gold_default[(sizeof is->ask_gold_default) - 1] = '\0';
		PopupForm_set_text_key_and_flags (this, __, script_path, text_key, param_3, (int)is->ask_gold_default, param_5, param_6);
	} else
		PopupForm_set_text_key_and_flags (this, __, script_path, text_key, param_3, param_4, param_5, param_6);
}

CityLocValidity __fastcall
patch_Map_check_city_location (Map *this, int edx, int tile_x, int tile_y, int civ_id, bool check_for_city_on_tile)
{
	int min_sep = is->current_config.minimum_city_separation;
	CityLocValidity base_result = Map_check_city_location (this, __, tile_x, tile_y, civ_id, check_for_city_on_tile);

	// If minimum separation is one, make no change
	if (min_sep == 1)
		return base_result;

	// If minimum separation is <= 0, ignore the CITY_TOO_CLOSE objection to city placement unless the location is next to a city belonging to
	// another civ and the settings forbid founding there.
	else if ((min_sep <= 0) && (base_result == CLV_CITY_TOO_CLOSE)) {
		if (is->current_config.disallow_founding_next_to_foreign_city)
			for (int n = 1; n <= 8; n++) {
				int x, y;
				get_neighbor_coords (&p_bic_data->Map, tile_x, tile_y, n, &x, &y);
				City * city = city_at (x, y);
				if ((city != NULL) && (city->Body.CivID != civ_id))
					return CLV_CITY_TOO_CLOSE;
			}
		return CLV_OK;

	// If we have an increased separation we might have to exclude some locations the base code allows.
	} else if ((min_sep > 1) && (base_result == CLV_OK)) {
		// Check tiles around (x, y) for a city. Because the base result is CLV_OK, we don't have to check neighboring tiles, just those at
		// distance 2, 3, ... up to (an including) the minimum separation
		for (int dist = 2; dist <= min_sep; dist++) {

			// vertices stores the unwrapped coords of the tiles at the vertices of the square of tiles at distance "dist" around
			// (tile_x, tile_y). The order of the vertices is north, east, south, west.
			struct vertex {
				int x, y;
			} vertices[4] = {
				{tile_x         , tile_y - 2*dist},
				{tile_x + 2*dist, tile_y         },
				{tile_x         , tile_y + 2*dist},
				{tile_x - 2*dist, tile_y         }
			};

			// neighbor index for direction of tiles along edge starting from each vertex
			// values correspond to directions: southeast, southwest, northwest, northeast
			int edge_dirs[4] = {3, 5, 7, 1};

			// Loop over verts and check tiles along their associated edges. The N vert is associated with the NE edge, the E vert with
			// the SE edge, etc.
			for (int vert = 0; vert < 4; vert++) {
				wrap_tile_coords (&p_bic_data->Map, &vertices[vert].x, &vertices[vert].y);
				int  dx, dy;
				neighbor_index_to_diff (edge_dirs[vert], &dx, &dy);
				for (int j = 0; j < 2*dist; j++) { // loop over tiles along this edge
					int cx = vertices[vert].x + j * dx,
					    cy = vertices[vert].y + j * dy;
					wrap_tile_coords (&p_bic_data->Map, &cx, &cy);
					if (city_at (cx, cy))
						return CLV_CITY_TOO_CLOSE;
				}
			}

		}
		return base_result;

	} else
		return base_result;
}

bool
is_zero_strength (UnitType * ut)
{
	return (ut->Attack == 0) && (ut->Defence == 0) && (ut->Bombard_Strength == 0) && (ut->Air_Defence == 0);
}

bool
uses_ptw_arty_targeting (Unit * u)
{
	int type_id = u->Body.UnitTypeID;
	for (int n = 0; n < is->current_config.count_ptw_arty_types; n++)
		if (type_id == is->current_config.ptw_arty_types[n])
			return true;
	return false;
}

bool
is_captured (Unit_Body * u)
{
	return (u->RaceID >= 0) && (u->CivID >= 0) && (u->CivID < 32) && leaders[u->CivID].RaceID != u->RaceID;
}

// Decides whether or not units "a" and "b" are duplicates, i.e., whether they are completely interchangeable.
bool
are_units_duplicate (Unit_Body * a, Unit_Body * b)
{
	UnitType * a_type = &p_bic_data->UnitTypes[a->UnitTypeID],
	         * b_type = &p_bic_data->UnitTypes[b->UnitTypeID];

	// a and b are duplicates if...
	return
		// ... they have the same type that is not [a leader OR army] AND not a transport AND ...
		(a_type == b_type) &&
		(! (UnitType_has_ability (a_type, __, UTA_Leader) || UnitType_has_ability (a_type, __, UTA_Army))) &&
		(a_type->Transport_Capacity == 0) &&

		// ... they've taken the same amount of damage AND used up the same number of moves AND ...
		(a->Damage == b->Damage) && (a->Moves == b->Moves) &&

		// ... they have matching statuses (has attacked this turn, etc.) AND states (is fortified, is doing worker action, etc.) AND charm status AND ...
		(a->Status == b->Status) && (a->UnitState == b->UnitState) && (a->charmed == b->charmed) &&

		// ... [they have the same experience level OR are both zero strength units] AND ...
		((a->Combat_Experience == b->Combat_Experience) || (is_zero_strength (a_type) && is_zero_strength (b_type))) &&

		// ... [they are both not contained in any unit OR they are both contained in the same unit] AND ...
		(((a->Container_Unit < 0) && (b->Container_Unit < 0)) || (a->Container_Unit == b->Container_Unit)) &&

		// ... neither one is carrying a princess AND ...
		((a->carrying_princess_of_race < 0) && (b->carrying_princess_of_race < 0)) &&

		// ... [they are both captured units OR are both native units].
		(! (is_captured (a) ^ is_captured (b)));
}

// A unit is "busy" if it's performing a multi-turn action that the player might not want to interrupt. This includes all worker actions,
// intercepting, and auto bombard. A unit is NOT considered busy if it's fortified, on a move command, automated, or exploring.
bool
is_unit_busy (Unit * unit)
{
	int state = unit->Body.UnitState;

	// If unit is foritified or has no set state
	if (state <= 1) return false;

	// If unit is mining, irrigating, building (rail)road, planting forest, clearing forest/wetlands/pollution, building airfield/radar
	// tower/outpost/barricade, or intercepting
	else if (state <= 0xF) return true;

	// If unit is on a move command
	else if (state == 0x10) return false;

	// If unit is (rail)roading to a tile, or building a colony
	else if (state <= 0x13) return true;

	// If unit is auto-bombarding or auto-bombing
	else if ((state == 0x1F) || (state == 0x20)) return true;

	else return false;
}

int __fastcall
patch_Context_Menu_add_item_and_set_color (Context_Menu * this, int edx, int item_id, char * text, int red)
{
	// Initialize the array of duplicate counts. The array is 0x100 items long because that's the maximum number of items a menu can have.
	if (is->current_config.group_units_on_right_click_menu &&
	    (is->unit_menu_duplicates == NULL)) {
		unsigned dups_size = 0x100 * sizeof is->unit_menu_duplicates[0];
		is->unit_menu_duplicates = malloc (dups_size);
		memset (is->unit_menu_duplicates, 0, dups_size);
	}

	// Check if this menu item is a valid unit and grab pointers to its info
	int unit_id;
	Unit_Body * unit_body;
	bool disable = false, put_icon = false;
	enum unit_rcm_icon icon = URCMI_UNMOVED_CAN_ATTACK;
	if ((0 <= (unit_id = item_id - (0x13 + p_bic_data->UnitTypeCount))) &&
	    (NULL != (unit_body = p_units->Units[unit_id].Unit)) &&
	    (unit_body->UnitTypeID >= 0) && (unit_body->UnitTypeID < p_bic_data->UnitTypeCount)) {

		if (is->current_config.group_units_on_right_click_menu) {
			// Loop over all existing menu items and check if any of them is a unit that's a duplicate of the one being added
			for (int n = 0; n < this->Item_Count; n++) {
				Context_Menu_Item * item = &this->Items[n];
				int dup_unit_id = this->Items[n].Menu_Item_ID - (0x13 + p_bic_data->UnitTypeCount);
				Unit_Body * dup_body;
				if ((dup_unit_id >= 0) &&
				    (NULL != (dup_body = p_units->Units[dup_unit_id].Unit)) &&
				    are_units_duplicate (unit_body, dup_body)) {
					// The new item is a duplicate of the nth. Mark that in the duplicate counts array and return without actually
					// adding the item. It doesn't matter what value we return because the caller doesn't use it.
					is->unit_menu_duplicates[n] += 1;
					return 0;
				}
			}
		}

		if (unit_body->CivID == p_main_screen_form->Player_CivID) {
			Unit * unit = (Unit *)((int)unit_body - offsetof (Unit, Body));
			UnitType * unit_type = &p_bic_data->UnitTypes[unit_body->UnitTypeID];
			bool no_moves_left = unit_body->Moves >= Unit_get_max_move_points (unit);
			if (no_moves_left && is->current_config.gray_out_units_on_menu_with_no_remaining_moves)
				disable = true;

			// Put an icon next to this unit if we're configured to do so and it's not in an army
			Unit * container;
			if (is->current_config.put_movement_icons_on_units_on_menu &&
			    ((NULL == (container = get_unit_ptr (unit_body->Container_Unit))) ||
			     (! UnitType_has_ability (&p_bic_data->UnitTypes[container->Body.UnitTypeID], __, UTA_Army)))) {
				put_icon = true;
				if (no_moves_left)
					icon = URCMI_CANT_MOVE;
				else {
					bool can_attack = can_attack_this_turn (unit) && ((unit_type->Attack > 0) || (unit_type->Bombard_Strength > 0));
					if (is_unit_busy (unit))
						icon = can_attack ? URCMI_BUSY_CAN_ATTACK : URCMI_BUSY_NO_ATTACK;
					else if (unit_body->Moves == 0)
						icon = can_attack ? URCMI_UNMOVED_CAN_ATTACK : URCMI_UNMOVED_NO_ATTACK;
					else
						icon = can_attack ? URCMI_MOVED_CAN_ATTACK : URCMI_MOVED_NO_ATTACK;
				}
			}
		}
	}

	int tr = Context_Menu_add_item_and_set_color (this, __, item_id, text, red);

	if (disable)
		Context_Menu_disable_item (this, __, item_id);

	if (put_icon) {
		init_unit_rcm_icons ();
		if (is->unit_rcm_icon_state == IS_OK)
			Context_Menu_put_image_on_item (this, __, item_id, &is->unit_rcm_icons[icon]);
	}

	return tr;
}

int __fastcall
patch_Context_Menu_open (Context_Menu * this, int edx, int x, int y, int param_3)
{
	int * p_stack = (int *)&x;
	int ret_addr = p_stack[-1];

	if (is->current_config.group_units_on_right_click_menu &&
	    (ret_addr == ADDR_OPEN_UNIT_MENU_RETURN) &&
	    (is->unit_menu_duplicates != NULL)) {

		// Change the menu text to include the duplicate counts. This is pretty straight forward except for a couple little issues: we must
		// use the game's internal malloc & free for compatibility with the base code and must call a function that widens the menu form,
		// as necessary, to accommodate the longer strings.
		for (int n = 0; n < this->Item_Count; n++)
			if (is->unit_menu_duplicates[n] > 0) {
				Context_Menu_Item * item = &this->Items[n];
				unsigned new_text_len = strlen (item->Text) + 20;
				char * new_text = civ_prog_malloc (new_text_len);

				// Print entry text including dup count to new_text. Biggest complication here is that we want to print the dup count
				// after any leading spaces to preserve indentation.
				{
					int num_spaces = 0;
					while (item->Text[num_spaces] == ' ')
						num_spaces++;
					snprintf (new_text, new_text_len, "%.*s%dx %s", num_spaces, item->Text, is->unit_menu_duplicates[n] + 1, &item->Text[num_spaces]);
					new_text[new_text_len - 1] = '\0';
				}

				civ_prog_free (item->Text);
				item->Text = new_text;
				Context_Menu_widen_for_text (this, __, new_text);
			}

		// Clear the duplicate counts
		memset (is->unit_menu_duplicates, 0, 0x100 * sizeof is->unit_menu_duplicates[0]);
	}

	return Context_Menu_open (this, __, x, y, param_3);
}

bool
is_pop_unit (UnitType const * type)
{
	int join_city_action = UCV_Join_City & 0x0FFFFFFF; // To get the join city action code, use the command value and mask out the top 4 category bits
	int noncombat = (type->Attack | type->Defence | type->Bombard_Strength) == 0;
	return noncombat &&
		(type->PopulationCost > 0) &&
		(type->Worker_Actions == join_city_action);
}

void
ai_move_pop_unit (Unit * this)
{
	int type_id = this->Body.UnitTypeID;
	Tile * tile = tile_at (this->Body.X, this->Body.Y);
	UnitType * type = &p_bic_data->UnitTypes[type_id];

	int continent_id = tile->vtable->m46_Get_ContinentID (tile);

	// This part of the code follows how the replacement leader AI works. Basically it disbands the unit if it's on a continent with no
	// friendly cities, then flees if it's in danger, then moves it along a set path if it has one.
	if (leaders[this->Body.CivID].city_count_per_cont[continent_id] == 0) {
		Unit_disband (this);
		return;
	}
	if (any_enemies_near_unit (this, 49)) {
		Unit_set_state (this, __, UnitState_Fleeing);
		bool done = this->vtable->work (this);
		if (done || (this->Body.UnitState != 0))
			return;
	}
	byte next_move = Unit_pop_next_move_from_path (this);
	if (next_move > 0) {
		this->vtable->Move (this, __, next_move, 0);
		return;
	}

	// Find the best city to join
	City * best_join_loc = NULL;
	int best_join_value = -1;
	FOR_CITIES_OF (coi, this->Body.CivID) {
		City * city = coi.city;
		Tile * city_tile = tile_at (city->Body.X, city->Body.Y);
		if (continent_id == city_tile->vtable->m46_Get_ContinentID (city_tile)) {

			// Skip this city if it can't support another citizen
			if ((city->Body.FoodIncome <= 0) ||
			    (City_requires_improvement_to_grow (city) > -1))
				continue;

			int value = 100;

			// Consider distance.
			int dist_in_turns;
			if (! estimate_travel_time (this, city->Body.X, city->Body.Y, &dist_in_turns))
				continue; // No path or unit cannot move
			value = (value * 10) / (10 + dist_in_turns);

			// Scale value by city corruption rate.
			int good_shields    = city->Body.ProductionIncome,
			    corrupt_shields = city->Body.ProductionLoss;
			if (good_shields + corrupt_shields > 0)
				value = (value * good_shields) / (good_shields + corrupt_shields);
			else
				continue;

			if (value > best_join_value) {
				best_join_loc = city;
				best_join_value = value;
			}
		}
	}

	// Join city if we're already in the city we want to join, otherwise move to that city. If we couldn't find a city to join, go to the
	// nearest established city and wait.
	City * in_city = get_city_ptr (tile->vtable->m45_Get_City_ID (tile));
	City * moving_to_city = NULL;
	if (best_join_loc != NULL) {
		if ((best_join_loc == in_city) &&
		    patch_Unit_can_perform_command (this, __, UCV_Join_City)) {
			Unit_join_city (this, __, in_city);
			return;
		} else
			moving_to_city = best_join_loc;
	} else if (in_city == NULL)
		moving_to_city = find_nearest_established_city (this, continent_id);

	if (moving_to_city) {
		int first_move = patch_Trade_Net_set_unit_path (p_trade_net, __, this->Body.X, this->Body.Y, moving_to_city->Body.X, moving_to_city->Body.Y, this, this->Body.CivID, 0x101, NULL);
		if (first_move > 0) {
			Unit_set_escortee (this, __, -1);
			this->vtable->Move (this, __, first_move, 0);
			return;
		}
	}

	// Nothing to do, nowhere to go, just fortify in place.
	Unit_set_escortee (this, __, -1);
	Unit_set_state (this, __, UnitState_Fortifying);
}

void __fastcall
patch_Unit_ai_move_terraformer (Unit * this)
{
	int type_id = this->Body.UnitTypeID;
	Tile * tile = tile_at (this->Body.X, this->Body.Y);
	if (is->current_config.enable_pop_unit_ai &&
	    (tile != NULL) && (tile != p_null_tile) &&
	    (type_id >= 0) && (type_id < p_bic_data->UnitTypeCount) &&
	    is_pop_unit (&p_bic_data->UnitTypes[type_id])) {
		ai_move_pop_unit (this);
		return;
	}

	Unit_ai_move_terraformer (this);
}

int __stdcall
patch_get_anarchy_length (int leader_id)
{
	int base = get_anarchy_length (leader_id);
	int multiplier = is->current_config.anarchy_length_percent;
	if (multiplier != 100) {
		// Anarchy cannot be less than 2 turns or you'll get an instant government switch. Only let that happen when the percent multiplier is
		// set below zero, indicating a desire for no anarchy. Otherwise we can't reduce anarchy below 2 turns.
		if (multiplier < 0)
			return 1;
		else if (base <= 2)
			return base;
		else
			return not_below (2, rand_div (base * multiplier, 100));
	} else
		return base;
}

bool __fastcall
patch_Unit_check_king_ability_while_spawning (Unit * this, int edx, enum UnitTypeAbilities a)
{
	if (is->current_config.dont_give_king_names_in_non_regicide_games &&
	    ((*p_toggleable_rules & (TR_REGICIDE | TR_MASS_REGICIDE)) == 0))
		return false;
	else
		return Unit_has_ability (this, __, a);
}

int __fastcall
patch_Map_compute_neighbor_index_for_pass_between (Map * this, int edx, int x_home, int y_home, int x_neigh, int y_neigh, int lim)
{
	if (is->current_config.enable_land_sea_intersections)
		return 0;
	else
		return Map_compute_neighbor_index (this, __, x_home, y_home, x_neigh, y_neigh, lim);
}

// This call replacement used to be part of improvement perfuming but now its only purpose is to set ai_considering_order when
// ai_choose_production is looping over improvements.
bool __fastcall
patch_Improvement_has_wonder_com_bonus_for_ai_prod (Improvement * this, int edx, enum ImprovementTypeWonderFeatures flag)
{
	is->ai_considering_order.OrderID = this - p_bic_data->Improvements;
	is->ai_considering_order.OrderType = COT_Improvement;
	return Improvement_has_wonder_flag (this, __, flag);
}

// Similarly, this one sets the var when looping over unit types.
bool __fastcall
patch_UnitType_has_strat_0_for_ai_prod (UnitType * this, int edx, byte n)
{
	is->ai_considering_order.OrderID = this - p_bic_data->UnitTypes;
	is->ai_considering_order.OrderType = COT_Unit;
	return UnitType_has_ai_strategy (this, __, n);
}

int
compare_ai_prod_valuations (void const * vp_a, void const * vp_b)
{
	struct ai_prod_valuation const * a = vp_a,
		                       * b = vp_b;
	if      (a->point_value > b->point_value) return -1;
	else if (b->point_value > a->point_value) return  1;
	else                                      return  0;
}

void
rank_ai_production_options (City * city)
{
	is->count_ai_prod_valuations = 0;
	City_Order unused;
	patch_City_ai_choose_production (city, __, &unused); // records valuations in is->ai_prod_valuations
	qsort (is->ai_prod_valuations, is->count_ai_prod_valuations, sizeof is->ai_prod_valuations[0], compare_ai_prod_valuations);
}

void __fastcall
patch_City_Form_m82_handle_key_event (City_Form * this, int edx, int virtual_key_code, int is_down)
{
	if (is->current_config.enable_ai_production_ranking &&
	    (~*p_human_player_bits & (1 << this->CurrentCity->Body.CivID)) &&
	    (virtual_key_code == VK_P) && is_down) {
		rank_ai_production_options (this->CurrentCity);
		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_AI_PROD_RANKING", -1, 0, 0, 0);
		char s[200];
		for (int n = 0; n < is->count_ai_prod_valuations; n++) {
			struct ai_prod_valuation const * val = &is->ai_prod_valuations[n];
			char * name = (val->order_type == COT_Improvement) ? p_bic_data->Improvements[val->order_id].Name.S : p_bic_data->UnitTypes[val->order_id].Name;

			int show_strategy = -1;
			if (val->order_type == COT_Unit)
				itable_look_up (&is->unit_type_alt_strategies, val->order_id, &show_strategy);

			if ((show_strategy >= 0) && (show_strategy <= CL_LAST_UNIT_STRAT - CL_FIRST_UNIT_STRAT))
				snprintf (s, sizeof s, "^%d   %s (%s)", val->point_value, name, is->c3x_labels[CL_FIRST_UNIT_STRAT + show_strategy]);
			else
				snprintf (s, sizeof s, "^%d   %s", val->point_value, name);
			s[(sizeof s) - 1] = '\0';
			PopupForm_add_text (popup, __, s, 0);
		}
		patch_show_popup (popup, __, 0, 0);
	}

	City_Form_m82_handle_key_event (this, __, virtual_key_code, is_down);
}

bool
can_harvest_shields_from_forest (Tile * tile)
{
	int flags = tile->vtable->m43_Get_field_30 (tile);
	return (flags & 0x10000000) == 0;
}

void __fastcall
patch_open_tile_info (void * this, int edx, int mouse_x, int mouse_y, int civ_id)
{
	int tx, ty;
	if (is->current_config.show_detailed_tile_info &&
	    (! Main_Screen_Form_get_tile_coords_under_mouse (p_main_screen_form, __, mouse_x, mouse_y, &tx, &ty))) {
		is->viewing_tile_info_x = tx;
		is->viewing_tile_info_y = ty;
	} else
		is->viewing_tile_info_x = is->viewing_tile_info_y = -1;

	return open_tile_info (this, __, mouse_x, mouse_y, civ_id);
}

bool
is_explored (Tile * tile, int civ_id)
{
	unsigned explored_bits = tile->Body.Fog_Of_War;
	int in_debug_mode = (*p_debug_mode_bits & 8) != 0; // checking bit 3 here b/c that's how resource visibility is checked in open_tile_info
	if (in_debug_mode || (explored_bits & (1 << civ_id)))
		return true;
	else if (is->current_config.share_visibility_in_hoseat && // if shared hotseat vis is enabled AND
		 (*p_is_offline_mp_game && ! *p_is_pbem_game) && // is hotseat game AND
		 ((1 << civ_id) & *p_human_player_bits) && // "civ_id" is a human player AND
		 (explored_bits & *p_human_player_bits)) // any human player has visibility on the tile
		return true;
	else
		return false;
}

// On the GOG executable, this function intercepts the call to draw the "No information available" text on a unexplored (black) tile. In that case we
// replace that text with the coords. But on the Steam EXE this func also intercepts the call that draws the resource name on the visible tile info
// box b/c the call site is reused via a jump. So we must check that the tile is actually unexplored before replacing the text so that the resource
// name doesn't get overwritten.
int __fastcall
patch_PCX_Image_draw_no_tile_info (PCX_Image * this, int edx, char * str, int x, int y, int str_len)
{
	Tile * tile = tile_at (is->viewing_tile_info_x, is->viewing_tile_info_y);
	if ((tile != p_null_tile) && ! is_explored (tile, p_main_screen_form->Player_CivID)) {
		char s[100];
		snprintf (s, sizeof s, "(%d, %d)", is->viewing_tile_info_x, is->viewing_tile_info_y);
		s[(sizeof s) - 1] = '\0';
		return PCX_Image_draw_text (this, __, s, x, y, strlen (s));
	} else
		return PCX_Image_draw_text (this, __, str, x, y, str_len);
}

int __fastcall
patch_PCX_Image_draw_tile_info_terrain (PCX_Image * this, int edx, char * str, int x, int y, int str_len)
{
	Tile * tile = tile_at (is->viewing_tile_info_x, is->viewing_tile_info_y);
	if (tile != p_null_tile) {
		// Draw tile coords on line below terrain name
		char s[200];
		snprintf (s, sizeof s, "(%d, %d)", is->viewing_tile_info_x, is->viewing_tile_info_y);
		PCX_Image_draw_text (this, __, s, x, y + 14, strlen (s));

		if ((is->city_loc_display_perspective >= 0) &&
		    ((1 << is->city_loc_display_perspective) & *p_player_bits)) {
			int eval = ai_eval_city_location (is->viewing_tile_info_x, is->viewing_tile_info_y, is->city_loc_display_perspective, false, NULL);
			if (eval > 0) {
				snprintf (s, sizeof s, "%d", eval - 1000000);
				PCX_Image_draw_text (this, __, s, x + 95, y, strlen (s));
			}
		}

		// If tile has been chopped, indicate that to the right of the terrain name
		if (! can_harvest_shields_from_forest (tile))
			PCX_Image_draw_text (this, __, is->c3x_labels[CL_CHOPPED], x + 145, y, strlen (is->c3x_labels[CL_CHOPPED]));
	}
	return PCX_Image_draw_text (this, __, str, x, y, str_len);
}

int __fastcall
patch_City_compute_corrupted_yield (City * this, int edx, int gross_yield, bool is_production)
{
	Leader * leader = &leaders[this->Body.CivID];

	if (is->current_config.zero_corruption_when_off &&
	    (p_bic_data->Governments[leader->GovernmentType].CorruptionAndWaste == CWT_Off))
		return 0;

	int tr = City_compute_corrupted_yield (this, __, gross_yield, is_production);

	if (is->current_config.promote_forbidden_palace_decorruption) {
		int actual_capital_id = leader->CapitalID;
		for (int improv_id = 0; improv_id < p_bic_data->ImprovementsCount; improv_id++) {
			Improvement * improv = &p_bic_data->Improvements[improv_id];
			City * fp_city;
			if ((improv->SmallWonderFlags & ITSW_Reduces_Corruption) &&
			    (NULL != (fp_city = get_city_ptr (leader->Small_Wonders[improv_id])))) {
				leader->CapitalID = fp_city->Body.ID;
				int fp_corrupted_yield = City_compute_corrupted_yield (this, __, gross_yield, is_production);
				if (fp_corrupted_yield < tr)
					tr = fp_corrupted_yield;
			}
		}
		leader->CapitalID = actual_capital_id;
	}

	return tr;
}

void __fastcall
patch_Map_Renderer_m19_Draw_Tile_by_XY_and_Flags (Map_Renderer * this, int edx, int param_1, int pixel_x, int pixel_y, Map_Renderer * map_renderer, int param_5, int tile_x, int tile_y, int param_8)
{
	Map_Renderer_m19_Draw_Tile_by_XY_and_Flags (this, __, param_1, pixel_x, pixel_y, map_renderer, param_5, tile_x, tile_y, param_8);

	Map * map = &p_bic_data->Map;
	if ((is->city_loc_display_perspective >= 0) &&
	    (! map->vtable->m10_Get_Map_Zoom (map)) && // Turn off display when zoomed out. Need another set of highlight images for that.
	    ((1 << is->city_loc_display_perspective) & *p_player_bits) &&
	    (((tile_x + tile_y) % 2) == 0)) { // Replicate a check from the base game code. Without this we'd be drawing additional tiles half-way off the grid.

		init_tile_highlights ();
		if (is->tile_highlight_state == IS_OK) {
			int eval = ai_eval_city_location (tile_x, tile_y, is->city_loc_display_perspective, false, NULL);
			if (eval > 0) {
				int step_size = 10;
				int midpoint = (COUNT_TILE_HIGHLIGHTS % 2 == 0) ? 1000000 : (1000000 - step_size/2);
				int grade = (eval >= midpoint) ? (eval - midpoint) / step_size : (eval - midpoint) / step_size - 1;
				int i_highlight = clamp (0, COUNT_TILE_HIGHLIGHTS - 1, COUNT_TILE_HIGHLIGHTS/2 + grade);
				Tile_Image_Info_draw_on_map (&is->tile_highlights[i_highlight], __, this, pixel_x, pixel_y, 1, 1, 1, 0);
			}
		}
	}
}

void __fastcall
patch_Main_Screen_Form_m82_handle_key_event (Main_Screen_Form * this, int edx, int virtual_key_code, int is_down)
{
	char s[200];
	if (is->current_config.enable_ai_city_location_desirability_display &&
	    (virtual_key_code == VK_L) && is_down &&
	    (! (is_command_button_active (&this->GUI, UCV_Load) || is_command_button_active (&this->GUI, UCV_Unload))) &&
	    (*p_player_bits != 0)) { // Player bits all zero indicates we aren't currently in a game. Need to check for this because UI events on the
		                     // main menu also pass through this function.
		int is_debug_mode = (*p_debug_mode_bits & 4) != 0; // This is how the check is done in open_tile_info. Actually there are two debug
		                                                   // mode bits (4 and 8) and I don't know what the difference is.
		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_CITY_LOC_HIGHLIGHTS", -1, 0, 0x40, 0);
		snprintf (s, sizeof s, " %s", is->c3x_labels[CL_OFF]);
		s[(sizeof s) - 1] = '\0';
		PopupSelection_add_item (&popup->selection, __, s, 0);
		for (int n = 1; n < 32; n++)
			if ((*p_player_bits & (1 << n)) &&
			    (is_debug_mode || (n == p_main_screen_form->Player_CivID))) {
				Race * race = &p_bic_data->Races[leaders[n].RaceID];
				snprintf (s, sizeof s, " (%d) %s", n, race->vtable->GetLeaderName (race));
				s[(sizeof s) - 1] = '\0';
				PopupSelection_add_item (&popup->selection, __, s, n);
			}
		int sel = patch_show_popup (popup, __, 0, 0);
		if (sel >= 0) { // -1 indicates popup was closed without making a selection
			is->city_loc_display_perspective = (sel >= 1) ? sel : -1;
			this->vtable->m73_call_m22_Draw ((Base_Form *)this); // Trigger map redraw
		}
	}
	Main_Screen_Form_m82_handle_key_event (this, __, virtual_key_code, is_down);
}

int __fastcall
patch_Unit_get_move_points_after_airdrop (Unit * this)
{
	return is->current_config.dont_end_units_turn_after_airdrop ? this->Body.Moves : Unit_get_max_move_points (this);
}

int __fastcall
patch_Unit_get_move_points_after_set_to_intercept (Unit * this)
{
	return is->current_config.patch_intercept_lost_turn_bug ? this->Body.Moves : Unit_get_max_move_points (this);
}

void __cdecl
activate_mod_info_button (int control_id)
{
	PopupForm * popup = get_popup_form ();
	popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_INFO", -1, 0, 0, 0);
	char s[500];
	char version_letter = 'A' + MOD_VERSION%100;

	if (MOD_PREVIEW_VERSION == 0)
		snprintf (s, sizeof s, "%s: %d%c", is->c3x_labels[CL_VERSION], MOD_VERSION/100, MOD_VERSION%100 != 0 ? version_letter : ' ');
	else
		snprintf (s, sizeof s, "%s: %d%c%s %d", is->c3x_labels[CL_VERSION], MOD_VERSION/100, MOD_VERSION%100 != 0 ? version_letter : ' ', is->c3x_labels[CL_PREVIEW], MOD_PREVIEW_VERSION);
	s[(sizeof s) - 1] = '\0';
	PopupForm_add_text (popup, __, s, 0);

	snprintf (s, sizeof s, "^%s:", is->c3x_labels[CL_CONFIG_FILES_LOADED]);
	s[(sizeof s) - 1] = '\0';
	PopupForm_add_text (popup, __, s, 0);

	int n = 1;
	for (struct loaded_config_name * lcn = is->loaded_config_names; lcn != NULL; lcn = lcn->next) {
		snprintf (s, sizeof s, "^  %d. %s", n, lcn->name);
		s[(sizeof s) - 1] = '\0';
		n++;
		PopupForm_add_text (popup, __, s, 0);
	}

	patch_show_popup (popup, __, 0, 0);
}

int __fastcall
patch_Parameters_Form_m68_Show_Dialog (Parameters_Form * this, int edx, int param_1, void * param_2, void * param_3)
{
	init_mod_info_button_images ();

	// "b" is the mod info button. It's created each time the preferences form (or "parameters" form as Antal called it) is opened and destroyed
	// every time the form is closed. This is the easiest way since it matches how the base game works. At first I tried creating the button once
	// but then it will only appear once since its attachment to the prefs form is lost when the form is destroyed on closure. I tried reattaching
	// the button to each newly created form but wasn't able to make that work.
	Button * b = NULL;
	if (is->mod_info_button_images_state == IS_OK) {
		b = malloc (sizeof *b);
		Button_construct (b);

		Button_initialize (b, __,
				   is->c3x_labels[CL_MOD_INFO_BUTTON_TEXT], // text
				   MOD_INFO_BUTTON_ID, // control ID
				   (p_bic_data->ScreenWidth - 1024) / 2 + 891, // location x
				   (p_bic_data->ScreenHeight - 768) / 2 + 31,  // location y
				   MOD_INFO_BUTTON_WIDTH, MOD_INFO_BUTTON_HEIGHT, // width, height
				   (Base_Form *)this, // parent
				   0); // ?

		for (int n = 0; n < 3; n++)
			b->Images[n] = &is->mod_info_button_images[n];
		PCX_Image_set_font (&b->Base_Data.Canvas, __, get_font (15, FSF_NONE), 0, 0, 0);
		b->activation_handler = &activate_mod_info_button;

		// Need to draw once manually or the button won't look right
		b->vtable->m73_call_m22_Draw ((Base_Form *)b);
	}

	int tr = Parameters_Form_m68_Show_Dialog (this, __, param_1, param_2, param_3);

	if (b != NULL) {
		b->vtable->destruct ((Base_Form *)b, __, 0);
		free (b);
	}

	return tr;
}

bool __fastcall
patch_City_cycle_specialist_type (City * this, int edx, int mouse_x, int mouse_y, Citizen * citizen, City_Form * city_form)
{
	int specialist_count = 0; {
		Leader * city_owner = &leaders[this->Body.CivID];
		for (int n = 0; n < p_bic_data->CitizenTypeCount; n++)
			specialist_count += (n != p_bic_data->default_citizen_type) &&
				Leader_has_tech (city_owner, __, p_bic_data->CitizenTypes[n].RequireID);
	}
	int shift_down = (*p_GetAsyncKeyState) (VK_SHIFT) >> 8;
	int original_worker_type = citizen->WorkerType;

	// The return value of this function is not actually used by either of the two original callers.
	bool tr = City_cycle_specialist_type (this, __, mouse_x, mouse_y, citizen, city_form);

	// Cycle all the way around back to the previous specialist type, if appropriate.
	// If the worker type was not changed after the first call to cycle_specialist_type, that indicates that the player was asked to disable
	// governor management and chose not to. Do not try to cycle backwards in that case or else we'll spam the player with more popups.
	if (is->current_config.reverse_specialist_order_with_shift &&
	    shift_down && (specialist_count > 2) && (citizen->WorkerType != original_worker_type)) {
		for (int n = 0; n < specialist_count - 2; n++)
			City_cycle_specialist_type (this, __, mouse_x, mouse_y, citizen, city_form);
	}

	return tr;
}

int __fastcall
patch_City_get_pollution_from_pop (City * this)
{
	if (! is->current_config.enable_negative_pop_pollution)
		return City_get_pollution_from_pop (this);

	int base_pollution = this->Body.Population.Size - p_bic_data->General.MaximumSize_City;
	if (base_pollution <= 0)
		return 0;

	// Consider improvements
	int net_pollution = base_pollution;
	int any_cleaning_improvs = 0;
	for (int n = 0; n < p_bic_data->ImprovementsCount; n++) {
		Improvement * improv = &p_bic_data->Improvements[n];
		if ((improv->ImprovementFlags & ITF_Removes_Population_Pollution) && has_active_building (this, n)) {
			any_cleaning_improvs = 1;
			if (improv->Pollution < 0)
				net_pollution += improv->Pollution;
		}
	}

	if (net_pollution <= 0)
		return 0;
	else if (any_cleaning_improvs)
		return 1;
	else
		return net_pollution;
}

// The original version of this function in the base game contains a duplicate of the logic that computes the total pollution from buildings and
// pop. By re-implementing it to use the get_pollution_from_* functions, we ensure that our changes to get_pollution_from_pop will be accounted for.
// Note: This function is called from two places, one is the city screen and the other I'm not sure about. The pollution spawning logic works by
// calling the get_pollution_from_* funcs directly.
int __fastcall
patch_City_get_total_pollution (City * this)
{
	return City_get_pollution_from_buildings (this) + patch_City_get_pollution_from_pop (this);
}

void remove_extra_palaces (City * city, City * excluded_destination);

void __fastcall
patch_City_add_or_remove_improvement (City * this, int edx, int improv_id, int add, bool param_3)
{
	int init_maintenance = this->Body.Improvements_Maintenance;
	Improvement * improv = &p_bic_data->Improvements[improv_id];

	// The enable_negative_pop_pollution feature changes the rules so that improvements flagged as removing pop pollution and having a negative
	// pollution amount contribute to the city's pop pollution instead of building pollution. Here we make sure that such improvements do not
	// contribute to building pollution by temporarily zeroing out their pollution stat when they're added to or removed from a city.
	if (is->current_config.enable_negative_pop_pollution &&
	    (improv->ImprovementFlags & ITF_Removes_Population_Pollution) &&
	    (improv->Pollution < 0)) {
		int saved_pollution_amount = improv->Pollution;
		improv->Pollution = 0;
		City_add_or_remove_improvement (this, __, improv_id, add, param_3);
		improv->Pollution = saved_pollution_amount;
	} else
		City_add_or_remove_improvement (this, __, improv_id, add, param_3);

	// Update things in case we've added or removed a mill. This is only necessary while in-game. If the game is still loading the scenario, it
	// will recompute trade & resources after it's done.
	if (! is->is_placing_scenario_things) {
		// Collect info about this mill, if in fact the added or removed improvement is a mill. If it's not, all these vars will be left
		// false. "generates_input" tracks whether or not the mill generates a resource that's an input for another mill.
		bool is_non_local_mill, is_yielding_mill, generates_input; {
			is_non_local_mill = is_yielding_mill = generates_input = false;
			for (int n = 0; n < is->current_config.count_mills; n++) {
				struct mill * mill = &is->current_config.mills[n];
				if (mill->improv_id == improv_id) {
					is_non_local_mill |= (mill->flags & MF_LOCAL) == 0;
					is_yielding_mill |= (mill->flags & MF_YIELDS) != 0;
					generates_input |= (is->mill_input_resource_bits[mill->resource_id >> 3] & (1 << (mill->resource_id & 7))) != 0;
				}
			}
		}

		// If the mill generates a resource that's added to the trade network or it's generating an input (potentially used locally to
		// generate a traded resource) then rebuild the resource network. This is not necessary if the improvement also affects trade routes
		// since the base method will have already done this recomputation.
		if ((is_non_local_mill || generates_input) &&
		    ((improv->ImprovementFlags & ITF_Allows_Water_Trade) == 0) &&
		    ((improv->ImprovementFlags & ITF_Allows_Air_Trade)   == 0) &&
		    ((improv->WonderFlags      & ITW_Safe_Sea_Travel)    == 0))
			patch_Trade_Net_recompute_resources (p_trade_net, __, 0);

		// If the mill adds yields or might be a link in a resource production chain that does, recompute yields in the city.
		if (is_yielding_mill || generates_input)
			City_recompute_yields_and_happiness (this);
	}

	// Adding or removing an obsolete improvement should not change the total maintenance since obsolete improvs shouldn't cost maintenance. In
	// the base game, they usually do since the game doesn't update maintenance costs when buildings are obsoleted, but with that bug patched we
	// can enforce the correct behavior.
	if (is->current_config.patch_maintenance_persisting_for_obsolete_buildings &&
	    (improv->ObsoleteID >= 0) && Leader_has_tech (&leaders[this->Body.CivID], __, improv->ObsoleteID))
		this->Body.Improvements_Maintenance = init_maintenance;

	// If AI MCS is enabled and we've added the Palace to a city, then remove any extra palaces from that city. If we're in the process of
	// capturing a city, it's necessary to exclude that city as a possible destination for removed extra palaces.
	if ((is->current_config.ai_multi_city_start > 1) &&
	    (! is->is_placing_scenario_things) &&
	    add && (improv->ImprovementFlags & ITF_Center_of_Empire) &&
	    ((*p_human_player_bits & (1 << this->Body.CivID)) == 0))
		remove_extra_palaces (this, is->currently_capturing_city);
}

void __fastcall
patch_Fighter_begin (Fighter * this, int edx, Unit * attacker, int attack_direction, Unit * defender)
{
	Fighter_begin (this, __, attacker, attack_direction, defender);

	// Apply override of retreat eligibility
	// Must use this->defender instead of the defender argument since the argument is often NULL, in which case Fighter_begin finds a defender on
	// the target tile and stores it in this->defender. Also must check that against NULL since Fighter_begin might fail to find a defender.
	enum UnitTypeClasses class = p_bic_data->UnitTypes[this->attacker->Body.UnitTypeID].Unit_Class;
	if ((this->defender != NULL) && ((class == UTC_Land) || (class == UTC_Sea))) {
		enum retreat_rules retreat_rules = (class == UTC_Land) ? is->current_config.land_retreat_rules : is->current_config.sea_retreat_rules;
		if (retreat_rules != RR_STANDARD) {
			if (retreat_rules == RR_NONE)
				this->attacker_eligible_to_retreat = this->defender_eligible_to_retreat = 0;
			else if (retreat_rules == RR_ALL_UNITS)
				this->attacker_eligible_to_retreat = this->defender_eligible_to_retreat = 1;
			else if (retreat_rules == RR_IF_FASTER) {
				int diff = Unit_get_max_move_points (this->attacker) - Unit_get_max_move_points (this->defender);
				this->attacker_eligible_to_retreat = diff > 0;
				this->defender_eligible_to_retreat = diff < 0;
			}
			this->defender_eligible_to_retreat &= city_at (this->defender_location_x, this->defender_location_y) == NULL;
		}
	}
}

void __fastcall
patch_Unit_despawn (Unit * this, int edx, int civ_id_responsible, byte param_2, byte param_3, byte param_4, byte param_5, byte param_6, byte param_7)
{
	int owner_id = this->Body.CivID;
	int type_id = this->Body.UnitTypeID;

	// Clear extra DBs used by this unit
	int extra_dbs;
	if (itable_look_up (&is->extra_defensive_bombards, this->Body.ID, &extra_dbs) && (extra_dbs != 0))
		itable_insert (&is->extra_defensive_bombards, this->Body.ID, 0);

	Unit_despawn (this, __, civ_id_responsible, param_2, param_3, param_4, param_5, param_6, param_7);

	change_unit_type_count (&leaders[owner_id], type_id, -1);
}

void __fastcall
patch_Map_Renderer_m71_Draw_Tiles (Map_Renderer * this, int edx, int param_1, int param_2, int param_3)
{
	// Restore the tile count if it was saved by recompute_resources. This is necessary because the Draw_Tiles method loops over all tiles.
	if (is->saved_tile_count >= 0) {
		p_bic_data->Map.TileCount = is->saved_tile_count;
		is->saved_tile_count = -1;
	}

	Map_Renderer_m71_Draw_Tiles (this, __, param_1, param_2, param_3);
}

// Returns whether or not city has an "extra palace", a concept used by the AI multi-city start. Extra palaces are small wonders that reduce
// corruption (e.g. forbidden palace) that are listed under the ai_multi_start_extra_palaces config option.
bool
has_extra_palace (City * city)
{
	for (int n = 0; n < is->current_config.count_ai_multi_start_extra_palaces; n++) {
		int improv_id = is->current_config.ai_multi_start_extra_palaces[n];
		Improvement * improv = &p_bic_data->Improvements[improv_id];
		if ((improv->Characteristics & ITC_Small_Wonder) &&
		    (improv->SmallWonderFlags & ITSW_Reduces_Corruption) &&
		    (leaders[city->Body.CivID].Small_Wonders[improv_id] == city->Body.ID)) {
			return true;
		}
	}
	return false;
}

// Removes any extra palaces from this city to other cities owned by the same player (if possible). Does not check that the city belongs to an AI or
// that AI MCS is enabled.
void
remove_extra_palaces (City * city, City * excluded_destination)
{
	Leader * leader = &leaders[city->Body.CivID];
	int extra_palace_lost;
	do {
		// Search for an extra palace in the city and delete it. Remember its ID so we can put it elsewhere.
		extra_palace_lost = -1;
		for (int n = 0; n < is->current_config.count_ai_multi_start_extra_palaces; n++) {
			int improv_id = is->current_config.ai_multi_start_extra_palaces[n];
			Improvement * improv = &p_bic_data->Improvements[improv_id];
			if ((improv->Characteristics & ITC_Small_Wonder) &&
			    (improv->SmallWonderFlags & ITSW_Reduces_Corruption) &&
			    (leader->Small_Wonders[improv_id] == city->Body.ID)) {
				patch_City_add_or_remove_improvement (city, __, improv_id, 0, false);
				extra_palace_lost = improv_id;
				break;
			}
		}

		// Replace the lost extra palace like what happens to the real palace
		if (extra_palace_lost >= 0) {
			int best_rating = -1;
			City * best_location = NULL;
			FOR_CITIES_OF (coi, leader->ID) {
				City * candidate = coi.city;
				if ((candidate != city) &&
				    (candidate->Body.ID != leader->CapitalID) &&
				    (candidate != excluded_destination) &&
				    ! has_extra_palace (candidate)) {

					// Rate this candidate as a possible (extra) palace location. The criteria we use to rate it are identical to
					// what the base game uses to find a new location for the palace.
					int rating = 0;
					rating += candidate->Body.Population.Size;
					rating += count_units_at (candidate->Body.X, candidate->Body.Y, UF_4, -1, 0, -1);
					rating += 2 * City_count_citizens_of_race (candidate, __, leader->RaceID);
					FOR_TILES_AROUND (tai, 0x121, candidate->Body.X, candidate->Body.Y) {
						if (tai.n == 0)
							continue;
						City * neighbor = get_city_ptr (tai.tile->CityID);
						if ((neighbor != NULL) && (neighbor != city) && (neighbor->Body.CivID == leader->ID)) {
							int size = neighbor->Body.Population.Size;
							if      (size > p_bic_data->General.MaximumSize_City) rating += 3;
							else if (size > p_bic_data->General.MaximumSize_Town) rating += 2;
							else                                                  rating += 1;
						}
					}

					if (rating > best_rating) {
						best_rating = rating;
						best_location = candidate;
					}
				}
			}

			if (best_location != NULL)
				City_add_or_remove_improvement (best_location, __, extra_palace_lost, 1, false);
		}
	} while (extra_palace_lost >= 0);
}

void
on_gain_city (Leader * leader, City * city, enum city_gain_reason reason)
{
	// Handle extra palaces for AI multi-city start
	if (((*p_human_player_bits & (1<<leader->ID)) == 0) && // If leader is an AI AND
	    (is->current_config.ai_multi_city_start > 1) && // AI multi-city start is enabled AND
	    (leader->Cities_Count > 1) && // city is not the only one the AI has (i.e. it's not the capital) AND
	    (reason != CGR_PLACED_FOR_SCENARIO) && (reason != CGR_PLACED_FOR_AI_MULTI_CITY_START)) { // city was not placed before the game started

		// Find an extra palace that this player does not already have
		int free_extra_palace = -1;
		for (int n = 0; n < is->current_config.count_ai_multi_start_extra_palaces; n++) {
			int improv_id = is->current_config.ai_multi_start_extra_palaces[n];
			Improvement * improv = &p_bic_data->Improvements[improv_id];
			if ((improv->Characteristics & ITC_Small_Wonder) &&
			    (improv->SmallWonderFlags & ITSW_Reduces_Corruption) &&
			    (NULL == get_city_ptr (leader->Small_Wonders[improv_id]))) {
				free_extra_palace = improv_id;
				break;
			}
		}

		// Place that extra palace here
		if (free_extra_palace >= 0)
			City_add_or_remove_improvement (city, __, free_extra_palace, 1, false);
	}
}

void
on_lose_city (Leader * leader, City * city, enum city_loss_reason reason)
{
	// If leader is an AI and AI MCS is enabled, remove any extra palaces the city has
	if (((*p_human_player_bits & (1<<leader->ID)) == 0) &&
	    (is->current_config.ai_multi_city_start > 1))
		remove_extra_palaces (city, NULL);
}

// Returns -1 if the location is unusable, 0-9 if it's usable but doesn't satisfy all criteria, and 10 if it couldn't be better
int
eval_starting_location (Map * map, int const * alt_starting_locs, int alt_starting_loc_count, int tile_x, int tile_y, int civ_id)
{
	Tile * tile = tile_at (tile_x, tile_y);
	if ((tile != p_null_tile) &&
	    (patch_Map_check_city_location (map, __, tile_x, tile_y, civ_id, true) == CLV_OK) &&
	    (tile->vtable->m15_Check_Goody_Hut (tile, __, 0) == 0) &&
	    (tile->vtable->m40_get_TileUnit_ID (tile) == -1)) {
		int tr = 0;

		// Avoid this location if it's too close to another starting location. If it's much too close, it's ruled out entirely.
		int closest_dist = INT_MAX;
		for (int n = 1; n <= map->Civ_Count; n++)
			if (map->Starting_Locations[n] >= 0) {
				int other_x, other_y;
				tile_index_to_coords (map, map->Starting_Locations[n], &other_x, &other_y);
				int dist = Map_get_x_dist (map, __, tile_x, other_x) + Map_get_y_dist (map, __, tile_y, other_y);
				if (dist < closest_dist)
					closest_dist = dist;
			}
		for (int n = 0; n < alt_starting_loc_count; n++)
			if (alt_starting_locs[n] >= 0) {
				int other_x, other_y;
				tile_index_to_coords (map, alt_starting_locs[n], &other_x, &other_y);
				int dist = Map_get_x_dist (map, __, tile_x, other_x) + Map_get_y_dist (map, __, tile_y, other_y);
				if (dist < closest_dist)
					closest_dist = dist;
			}
		if (closest_dist < map->Civ_Distance/3)
			return -1;
		else if (closest_dist >= 2*map->Civ_Distance/3)
			tr += 1;

		// Avoid tiny islands
		// tr += map->vtable->m33_Get_Continent (map, __, tile->ContinentID)->Body.TileCount >= 20;

		// Avoid garbage terrain, e.g. all desert or tundra
		int break_even_food_tiles = 0;
		FOR_TILES_AROUND (tai, 21, tile_x, tile_y) {
			if (tai.n == 0)
				continue; // Skip tile that would be covered by the city
			int x, y;
			tai_get_coords (&tai, &x, &y);
			int tile_food = Map_calc_food_yield_at (map, __, x, y, tai.tile->vtable->m50_Get_Square_BaseType (tai.tile), civ_id, 0, NULL);
			int food_required = p_bic_data->General.FoodPerCitizen + (tai.tile->vtable->m35_Check_Is_Water (tai.tile) ? 1 : 0);
			break_even_food_tiles += tile_food >= food_required;
		}
		tr += break_even_food_tiles >= 2;

		// Avoid wasting a food bonus
		tr += (tile->ResourceType < 0) || (tile->ResourceType >= p_bic_data->ResourceTypeCount) ||
			(p_bic_data->ResourceTypes[tile->ResourceType].Food == 0);

		int max_score = 3;
		return (10*tr)/max_score;
	} else
		return -1;
}

City *
create_starter_city (Map * map, int civ_id, int tile_index)
{
	int x, y;
	tile_index_to_coords (map, tile_index, &x, &y);
	City * tr = Leader_create_city (&leaders[civ_id], __, x, y, leaders[civ_id].RaceID, -1, NULL, true);
	if (tr != NULL)
		on_gain_city (&leaders[civ_id], tr, CGR_PLACED_FOR_AI_MULTI_CITY_START);
	return tr;
}

void
set_up_ai_multi_city_start (Map * map, int city_count)
{
	// Set of bits determining which players are eligible for the two-city start
	int eligibility_bits = 0,
	    count_eligible_civs = 0;
	for (int n = 1; n < 32; n++)
		if ((*p_player_bits & 1<<n) && // if there is an active player in this slot AND
		    ((*p_human_player_bits & 1<<n) == 0) && // that player is an AI AND
		    (leaders[n].Cities_Count == 0) && // does not have any cities AND
		    (tile_at_index (map, map->Starting_Locations[n]) != p_null_tile)) { // has a valid starting location
			eligibility_bits |= 1 << n;
			count_eligible_civs++;
		}

	if ((city_count < 1) || (count_eligible_civs == 0)) // if we have nothing to do
		return;

	char load_text[50];
	snprintf (load_text, sizeof load_text, "%s...", is->c3x_labels[CL_CREATING_CITIES]);
	load_text[(sizeof load_text) - 1] = '\0';
	Main_GUI_label_loading_bar (&p_main_screen_form->GUI, __, 1, load_text);

	// Generate alternate sets of starting locations. We need one set for each extra city we're going to create per player. Each set only needs to
	// include starting locations for eligible players, all others will be left as -1.
	int alt_starting_loc_count = 32 * (city_count - 1);
	int * alt_starting_locs = malloc (alt_starting_loc_count * sizeof *alt_starting_locs); {
		for (int n = 0; n < alt_starting_loc_count; n++)
			alt_starting_locs[n] = -1;

		for (int i_loc = 0; i_loc < alt_starting_loc_count; i_loc++) {
			int civ_id = i_loc % 32;
			if ((civ_id != 0) && (eligibility_bits & 1<<civ_id)) {
				int best_loc_val   = -1,
				    best_loc_index = -1;

				for (int try = 0; try < is->current_config.max_tries_to_place_fp_city; try++) {
					int i_loc = rand_int (p_rand_object, __, map->TileCount);
					int x_loc, y_loc;
					tile_index_to_coords (map, i_loc, &x_loc, &y_loc);
					if ((x_loc >= 0) && (y_loc >= 0)) {
						int val = eval_starting_location (map, alt_starting_locs, alt_starting_loc_count, x_loc, y_loc, civ_id);
						if (val >= 10) {
							best_loc_index = i_loc;
							break;
						} else if (val > best_loc_val) {
							best_loc_val = val;
							best_loc_index = i_loc;
						}
					}
				}

				if (best_loc_index >= 0)
					alt_starting_locs[i_loc] = best_loc_index;
			}
		}
	}

	int count_cities_created = 0;
	int count_eligible_civs_handled = 0;
	for (int civ_id = 1; civ_id < 32; civ_id++)
		if (eligibility_bits & 1<<civ_id) {
			int sloc = map->Starting_Locations[civ_id];

			// Create the first starting city for the AI. This one is its capital and is located at its actual starting
			// location. Afterward, delete its starting settler so it's as if the settler founded the city.
			{
				Unit * starting_settler = NULL;
				FOR_UNITS_ON (tai, tile_at_index (map, sloc)) {
					if (p_bic_data->UnitTypes[tai.unit->Body.UnitTypeID].AI_Strategy & UTAI_Settle) {
						starting_settler = tai.unit;
						break;
					}
				}

				create_starter_city (map, civ_id, sloc);
				count_cities_created++;

				if (starting_settler != NULL)
					patch_Unit_despawn (starting_settler, __, 0, 1, 0, 0, 0, 0, 0);

			}

			// Memoize all of the AI's starting units
			clear_memo ();
			FOR_UNITS_ON (tai, tile_at_index (map, sloc))
				memoize ((int)tai.unit);

			int extra_city_count = 0;
			for (int i_city = 1; i_city < city_count; i_city++) {
				int loc = alt_starting_locs[(i_city-1)*32 + civ_id];
				City * city;
				if ((loc >= 0) &&
				    (NULL != (city = create_starter_city (map, civ_id, loc)))) {
					count_cities_created++;
					extra_city_count++;

					// Spawn palace substitute in new city
					if (extra_city_count-1 < is->current_config.count_ai_multi_start_extra_palaces)
						patch_City_add_or_remove_improvement (city, __, is->current_config.ai_multi_start_extra_palaces[extra_city_count - 1], 1, true);

					// Move starting units over to the new city. Do this by moving every Nth unit where N is the number of extra
					// cities we've founded so far plus one. ("Extra" cities are the ones created after the capital.) E.g., if
					// this is the 1st city after the capital, move every 2nd unit to it. If it's the 2nd, move every 3rd, etc.
					for (int n = 0; n < is->memo_len; n++)
						if (n % (extra_city_count+1) == extra_city_count)
							patch_Unit_move ((Unit *)is->memo[n], __, city->Body.X, city->Body.Y);

				}
			}

			// Update progress report
			count_eligible_civs_handled++;
			int progress = 100 * count_eligible_civs_handled / count_eligible_civs;
			snprintf (load_text, sizeof load_text, "%s %d%%", is->c3x_labels[CL_CREATING_CITIES], progress);
			load_text[(sizeof load_text) - 1] = '\0';
			Main_GUI_label_loading_bar (&p_main_screen_form->GUI, __, 1, load_text);

		}

	free (alt_starting_locs);

	// Sanity check
	int any_adjacent_cities = 0; {
		if (p_cities->Cities != NULL)
			for (int city_index = 0; (city_index <= p_cities->LastIndex) && ! any_adjacent_cities; city_index++) {
				City * city = get_city_ptr (city_index);
				if (city != NULL)
					FOR_TILES_AROUND (tai, 9, city->Body.X, city->Body.Y)
						if ((tai.n > 0) && (NULL != get_city_ptr (tai.tile->vtable->m45_Get_City_ID (tai.tile)))) {
							any_adjacent_cities = 1;
							break;
						}
			}
	}
	int any_missing_fp_cities = (count_cities_created < city_count * count_eligible_civs);
	if (any_adjacent_cities || any_missing_fp_cities) {
		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_WARNING", -1, 0, 0, 0);
		char s[100];
		snprintf (s, sizeof s, "^%s", is->c3x_labels[CL_MCS_FAILED_SANITY_CHECK]);
		s[(sizeof s) - 1] = '\0';
		PopupForm_add_text (popup, __, s, 0);
		if (any_adjacent_cities) {
			snprintf (s, sizeof s, "^%s", is->c3x_labels[CL_MCS_ADJACENT_CITIES]);
			s[(sizeof s) - 1] = '\0';
			PopupForm_add_text (popup, __, s, 0);
		}
		if (any_missing_fp_cities) {
			snprintf (s, sizeof s, "^%s", is->c3x_labels[CL_MCS_MISSING_CITIES]);
			s[(sizeof s) - 1] = '\0';
			PopupForm_add_text (popup, __, s, 0);
		}
		patch_show_popup (popup, __, 0, 0);
	}
}

void __fastcall
patch_Map_process_after_placing (Map * this, int edx, bool param_1)
{
	if ((is->current_config.ai_multi_city_start > 0) && (*p_current_turn_no == 0))
		set_up_ai_multi_city_start (this, is->current_config.ai_multi_city_start);
	Map_process_after_placing (this, __, param_1);
}

int __fastcall
patch_City_get_net_commerce (City * this, int edx, int kind, bool include_science_age)
{
	int base = City_get_net_commerce (this, __, kind, include_science_age);

	if ((kind == 1) && // beakers, as opposed to 2 which is gold
	    (is->current_config.ai_research_multiplier != 100) &&
	    ((*p_human_player_bits & 1<<this->Body.CivID) == 0))
		return (base * is->current_config.ai_research_multiplier + 50) / 100;
	else
		return base;
}

void __fastcall
adjust_sliders_preproduction (Leader * this)
{
	// Replicate the behavior of the original code for AI players. (apply_machine_code_edits overwrites an equivalent branch & call in the
	// original code with a call to this method.)
	if ((*p_human_player_bits & 1<<this->ID) == 0)
		this->vtable->ai_adjust_sliders (this);

	// If human player would go bankrupt, try reducing their research spending to avoid that
	else if (is->current_config.cut_research_spending_to_avoid_bankruptcy) {
		int treasury = this->Gold_Encoded + this->Gold_Decrement,
		    reduced_spending = 0;
		while ((this->science_slider > 0) && (treasury + Leader_compute_income (this) < 0)) {
			this->science_slider -= 1;
			this->gold_slider += 1;
			Leader_recompute_economy (this);
			reduced_spending = 1;
		}
		if (reduced_spending && (this->ID == p_main_screen_form->Player_CivID)) {
			PopupForm * popup = get_popup_form ();
			popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_FORCE_CUT_RESEARCH_SPENDING", -1, 0, 0, 0);
			patch_show_popup (popup, __, 0, 0);
		}
	}
}

int __fastcall
patch_Leader_count_maintenance_free_units (Leader * this)
{
	if (is->current_config.extra_unit_maintenance_per_shields <= 0)
		return Leader_count_maintenance_free_units (this);
	else {
		int tr = 0;
		for (int n = 0; n <= p_units->LastIndex; n++) {
			Unit * unit = get_unit_ptr (n);
			if ((unit != NULL) && (unit->Body.CivID == this->ID)) {
				UnitType * type = &p_bic_data->UnitTypes[unit->Body.UnitTypeID];

				// If this is a free unit
				if ((unit->Body.RaceID != this->RaceID) || (type->b_Not_King == 0))
					tr++;

				// Otherwise, reduce the free unit count by however many times this unit's cost is above the threshold for extra
				// maintenace. It's alright if the resulting free unit count is negative since all callers of this function subtract
				// its return value from the total unit count to obtain the count of units that must be paid for.
				else
					tr -= type->Cost / is->current_config.extra_unit_maintenance_per_shields;
			}
		}
		return tr;
	}
}

int __fastcall
patch_Leader_sum_unit_maintenance (Leader * this, int edx, int government_id)
{
	if (is->current_config.extra_unit_maintenance_per_shields <= 0)
		return Leader_sum_unit_maintenance (this, __, government_id);
	else if (this->Cities_Count > 0) {
		int maint_free_count = patch_Leader_count_maintenance_free_units (this);
		int cost_per_unit, base_free_count;
		get_unit_support_info (this->ID, government_id, &cost_per_unit, &base_free_count);
		int ai_free_count; {
			if (*p_human_player_bits & 1<<this->ID)
				ai_free_count = 0;
			else {
				Difficulty_Level * difficulty = &p_bic_data->DifficultyLevels[*p_game_difficulty];
				ai_free_count = difficulty->Bonus_For_Each_City * this->Cities_Count + difficulty->Additional_Free_Support;
			}
		}
		return not_below (0, (this->Unit_Count - base_free_count - ai_free_count - maint_free_count) * cost_per_unit);
	} else
		return 0;
}

int
sum_improvements_maintenance_to_pay (Leader * leader, int govt_id)
{
	if (is->current_config.aggressively_penalize_bankruptcy)
		// We're going to replace the logic that charges maintenance, and the replacement will charge both unit & building maintenance b/c
		// they're intertwined under the new rules. Return zero here so the base game code doesn't charge any building maintenance on its own.
		return 0;
	else
		return Leader_sum_improvements_maintenance (leader, __, govt_id);
}

// The sum_improvements_maintenance function is called in two places as part of the randomization of whether improv or unit maintenance gets paid
// first. Redirect both of these calls to one function of our own.
int __fastcall patch_Leader_sum_improv_maintenance_to_pay_1 (Leader * this, int edx, int govt_id) { return sum_improvements_maintenance_to_pay (this, govt_id); }
int __fastcall patch_Leader_sum_improv_maintenance_to_pay_2 (Leader * this, int edx, int govt_id) { return sum_improvements_maintenance_to_pay (this, govt_id); }

int
compare_buildings_to_sell (void const * a, void const * b)
{
	int maint_a = (*(int const *)a >> 26) & 31,
	    maint_b = (*(int const *)b >> 26) & 31;
	return maint_b - maint_a;
}

int
compare_cities_by_production (void const * vp_id_a, void const * vp_id_b)
{
	City * a = get_city_ptr (*(int const *)vp_id_a),
	     * b = get_city_ptr (*(int const *)vp_id_b);
	return a->Body.ProductionIncome - b->Body.ProductionIncome;
}

void
charge_maintenance_with_aggressive_penalties (Leader * leader)
{
	int cost_per_unit;
	get_unit_support_info (leader->ID, leader->GovernmentType, &cost_per_unit, NULL);

	int improv_cost = Leader_sum_improvements_maintenance (leader, __, leader->GovernmentType);

	int unit_cost = 0; {
		if (leader->Cities_Count > 0) // Players with no cities don't pay unit maintenance, per the original game rules
			if (cost_per_unit > 0) {
				int count_free_units = Leader_get_free_unit_count (leader, __, leader->GovernmentType) + patch_Leader_count_maintenance_free_units (leader);
				unit_cost += not_below (0, (leader->Unit_Count - count_free_units) * cost_per_unit);
			}
	}

	int treasury = leader->Gold_Encoded + leader->Gold_Decrement;
	if (improv_cost + unit_cost > treasury) {
		clear_memo ();

		// Memoize all buildings this player owns that we might potentially sell. B/c the memo can only contain ints, we must pack the
		// maintenance cost, improv ID, and city ID all into one int. The city ID is stored in the lowest 13 bits, then the improv ID in the
		// next 13, and finally the maintenance amount in the 5 above those.
		FOR_CITIES_OF (coi, leader->ID)
			for (int n = 0; n < p_bic_data->ImprovementsCount; n++) {
				Improvement * improv = &p_bic_data->Improvements[n];

				int unsellable_flags =
					ITF_Center_of_Empire |
					ITF_50_Luxury_Output |
					ITF_50_Tax_Output |
					ITF_Reduces_Corruption |
					ITF_Increases_Luxury_Trade |
					ITF_Allows_City_Level_2 |
					ITF_Allows_City_Level_3 |
					ITF_Capitalization |
					ITF_Allows_Water_Trade |
					ITF_Allows_Air_Trade |
					ITF_Increases_Shields_In_Water |
					ITF_Increases_Food_In_Water |
					ITF_Increases_Trade_In_Water;

				// Only sell improvements that aren't contributing gold, even indirectly through e.g. happiness or boosting shield
				// production for Wealth
				int sellable =
					((improv->Characteristics & (ITC_Small_Wonder | ITC_Wonder)) == 0) &&
					((improv->ImprovementFlags & unsellable_flags) == 0) &&
					(improv->Happy_Faces_All <= 0) && (improv->Happy_Faces <= 0) &&
					(improv->Production <= 0);

				if (sellable && City_has_improvement (coi.city, __, n, 0)) {
					int maint = City_get_improvement_maintenance (coi.city, __, n);
					if (maint > 0)
						memoize ((not_above (31, maint) << 26) | (n << 13) | coi.city_id);
				}
			}

		// Sort the list of buildings so the highest maintenance ones come first
		qsort (is->memo, is->memo_len, sizeof is->memo[0], compare_buildings_to_sell);

		// Sell buildings until we can cover maintenance costs or until we run out of ones to sell
		int count_sold = 0;
		while ((improv_cost + unit_cost > treasury) && (count_sold < is->memo_len)) {
			int improv_id = ((1<<13) - 1) & (is->memo[count_sold] >> 13),
			    city_id   = ((1<<13) - 1) &  is->memo[count_sold];
			City * city = get_city_ptr (city_id);
			improv_cost -= City_get_improvement_maintenance (city, __, improv_id);
			City_sell_improvement (city, __, improv_id, false);
			treasury = leader->Gold_Encoded + leader->Gold_Decrement;
			count_sold++;
		}

		// Show popup informing the player that their buildings were force sold
		if ((leader->ID == p_main_screen_form->Player_CivID) && ! is_online_game ()) {
			PopupForm * popup = get_popup_form ();
			if (count_sold == 1) {
				int improv_id = ((1<<13) - 1) & (is->memo[0] >> 13),
				    city_id   = ((1<<13) - 1) &  is->memo[0];
				set_popup_str_param (0, get_city_ptr (city_id)->Body.CityName     , -1, -1);
				set_popup_str_param (1, p_bic_data->Improvements[improv_id].Name.S, -1, -1);
				popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_FORCE_SOLD_SINGLE_IMPROV", -1, 0, 0, 0);
				patch_show_popup (popup, __, 0, 0);
			} else if (count_sold > 1) {
				set_popup_int_param (0, count_sold);
				popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_FORCE_SOLD_MULTIPLE_IMPROVS", -1, 0, 0, 0);

				// Add list of sold improvements to popup
				for (int n = 0; n < count_sold; n++) {
					int improv_id = ((1<<13) - 1) & (is->memo[n] >> 13),
					    city_id   = ((1<<13) - 1) &  is->memo[n];
					char s[200];
					snprintf (s, sizeof s, "^  %s in %s",
						  p_bic_data->Improvements[improv_id].Name.S,
						  get_city_ptr (city_id)->Body.CityName);
					s[(sizeof s) - 1] = '\0';
					PopupForm_add_text (popup, __, s, 0);
				}

				patch_show_popup (popup, __, 0, 0);
			}
		}

		// If the player still can't afford maintenance, disband all the units they can't afford
		int count_disbanded = 0;
		Unit * to_disband;
		char first_disbanded_name[32];
		while ((improv_cost + unit_cost > treasury) &&
		       (unit_cost > 0) &&
		       (NULL != (to_disband = leader->vtable->find_unsupported_unit (leader)))) {
			if (count_disbanded == 0) {
				char const * name_src = (to_disband->Body.Custom_Name.S[0] == '\0')
					? p_bic_data->UnitTypes[to_disband->Body.UnitTypeID].Name
					: to_disband->Body.Custom_Name.S;
				strncpy (first_disbanded_name, name_src, sizeof first_disbanded_name);
				first_disbanded_name[(sizeof first_disbanded_name) - 1] = '\0';
			}
			Unit_disband (to_disband);
			count_disbanded++;
			unit_cost -= cost_per_unit;
		}

		// Show popup informing the player that their units were disbanded
		if (leader->ID == p_main_screen_form->Player_CivID) {
			PopupForm * popup = get_popup_form ();
			if (count_disbanded == 1) {
				set_popup_str_param (0, first_disbanded_name, -1, -1);
				int online_flag = is_online_game () ? 0x4000 : 0;
				popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_FORCE_DISBANDED_SINGLE_UNIT", -1, 0, online_flag, 0);
				patch_show_popup (popup, __, 0, 0);
			} else if ((count_disbanded > 1) && ! is_online_game ()) {
				set_popup_int_param (0, count_disbanded);
				popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_FORCE_DISBANDED_MULTIPLE_UNITS", -1, 0, 0, 0);
				patch_show_popup (popup, __, 0, 0);
			}
		}

		// If the player still can't afford maintenance, even after all that, start switching their cities to Wealth
		int wealth_income = 0;
		if (improv_cost + unit_cost > treasury + wealth_income) {
			// Memoize all cities not already building wealth and sort by production (lowest first)
			clear_memo ();
			FOR_CITIES_OF (coi, leader->ID)
				if ((coi.city->Body.Status & CSF_Capitalization) == 0)
					memoize (coi.city_id);
			qsort (is->memo, is->memo_len, sizeof is->memo[0], compare_cities_by_production);

			int wealth_improv_id = -1; {
				for (int n = 0; n < p_bic_data->ImprovementsCount; n++)
					if (p_bic_data->Improvements[n].ImprovementFlags & ITF_Capitalization) {
						wealth_improv_id = n;
						break;
					}
			}

			if (wealth_improv_id >= 0) {
				int n = 0,
				    switched_any = 0;

				while ((n < is->memo_len) && (improv_cost + unit_cost > treasury + wealth_income)) {
					City * city = get_city_ptr (is->memo[n]);
					City_set_production (city, __, COT_Improvement, wealth_improv_id, false);
					switched_any = 1;
					wealth_income += City_get_income_from_wealth_build (city);
					n++;
				}

				if (switched_any && (leader->ID == p_main_screen_form->Player_CivID)) {
					PopupForm * popup = get_popup_form ();
					Improvement * wealth = &p_bic_data->Improvements[wealth_improv_id];
					set_popup_str_param (0, wealth->Name.S, -1, -1);
					set_popup_str_param (1, wealth->CivilopediaEntry.S, -1, -1);
					popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_FORCE_BUILD_WEALTH", -1, 0, 0, 0);
					patch_show_popup (popup, __, 0, 0);
				}
			}
		}
	}

	Leader_set_treasury (leader, __, treasury - improv_cost - unit_cost);
}

void __fastcall
patch_Leader_pay_unit_maintenance (Leader * this)
{
	if (! is->current_config.aggressively_penalize_bankruptcy)
		Leader_pay_unit_maintenance (this);
	else
		charge_maintenance_with_aggressive_penalties (this);
}

void __fastcall
patch_Main_Screen_Form_show_wltk_message (Main_Screen_Form * this, int edx, int tile_x, int tile_y, char * text_key, bool pause)
{
	patch_Main_Screen_Form_show_map_message (this, __, tile_x, tile_y, text_key, is->current_config.dont_pause_for_love_the_king_messages ? false : pause);
}

void __fastcall
patch_Main_Screen_Form_show_wltk_ended_message (Main_Screen_Form * this, int edx, int tile_x, int tile_y, char * text_key, bool pause)
{
	patch_Main_Screen_Form_show_map_message (this, __, tile_x, tile_y, text_key, is->current_config.dont_pause_for_love_the_king_messages ? false : pause);
}

char __fastcall
patch_Tile_has_city_for_agri_penalty_exception (Tile * this)
{
	return is->current_config.no_penalty_exception_for_agri_fresh_water_city_tiles ? 0 : Tile_has_city (this);
}

int
show_razing_popup (void * popup_object, int popup_param_1, int popup_param_2, int razing_option)
{
	int response = patch_show_popup (popup_object, __, popup_param_1, popup_param_2);
	if (is->current_config.prevent_razing_by_players && (response == razing_option)) {
		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_CANT_RAZE", -1, 0, 0, 0);
		patch_show_popup (popup, __, 0, 0);
		return 0;
	}
	return response;
}

int __fastcall patch_show_popup_option_1_razes (void *this, int edx, int param_1, int param_2) { return show_razing_popup (this, param_1, param_2, 1); }
int __fastcall patch_show_popup_option_2_razes (void *this, int edx, int param_1, int param_2) { return show_razing_popup (this, param_1, param_2, 2); }

int __fastcall
patch_Context_Menu_add_abandon_city (Context_Menu * this, int edx, int item_id, char * text, bool checkbox, Tile_Image_Info * image)
{
	if (is->current_config.prevent_razing_by_players)
		return 0; // Return value is ignored by the caller
	else
		return Context_Menu_add_item (this, __, item_id, text, checkbox, image);
}

char *
check_pedia_upgrades_to_ptr (TextBuffer * this, char * str)
{
	Civilopedia_Form * pedia = p_civilopedia_form;
	UnitType * unit_type = NULL;
	if (is->current_config.indicate_non_upgradability_in_pedia &&
	    (pedia->Current_Article_ID >= 0) && (pedia->Current_Article_ID <= pedia->Max_Article_ID) &&
	    (NULL != (unit_type = pedia->Articles[pedia->Current_Article_ID]->unit_type)) &&
	    ((unit_type->Special_Actions & UCV_Upgrade_Unit) == 0))
		return is->c3x_labels[CL_OBSOLETED_BY];
	else
		return TextBuffer_check_ptr (this, __, str);
}

char * __fastcall patch_TextBuffer_check_pedia_upgrades_to_ptr_1 (TextBuffer * this, int edx, char * str) { return check_pedia_upgrades_to_ptr (this, str); }
char * __fastcall patch_TextBuffer_check_pedia_upgrades_to_ptr_2 (TextBuffer * this, int edx, char * str) { return check_pedia_upgrades_to_ptr (this, str); }

bool __fastcall
patch_Unit_select_stealth_attack_target (Unit * this, int edx, int target_civ_id, int x, int y, bool allow_popup, Unit ** out_selected_target)
{
	is->added_any_stealth_target = 0;
	return Unit_select_stealth_attack_target (this, __, target_civ_id, x, y, allow_popup, out_selected_target);
}

bool __fastcall
patch_Unit_can_stealth_attack (Unit * this, int edx, Unit * target)
{
	bool tr = Unit_can_stealth_attack (this, __, target);

	// If we're selecting a target for stealth attack via bombardment, we must filter out candidates we can't damage that way
	if (tr && is->selecting_stealth_target_for_bombard &&
	    ! can_damage_bombarding (&p_bic_data->UnitTypes[this->Body.UnitTypeID], target, tile_at (target->Body.X, target->Body.Y)))
		return false;

	else
		return tr;
}

int __fastcall
patch_Tile_check_water_for_stealth_attack (Tile * this)
{
	// When stealth attack bombard is enabled, remove a special rule inside can_stealth_attack that prevents land units from stealth attacking
	// onto sea tiles. This allows land artillery to stealth attack naval units.
	return is->current_config.enable_stealth_attack_via_bombardment ? 0 : this->vtable->m35_Check_Is_Water (this);
}

int __fastcall
patch_PopupSelection_add_stealth_attack_target (PopupSelection * this, int edx, char * text, int value)
{
	if (is->current_config.include_stealth_attack_cancel_option && (! is->added_any_stealth_target)) {
		PopupSelection_add_item (this, __, is->c3x_labels[CL_NO_STEALTH_ATTACK], -1);
		is->added_any_stealth_target = 1;
	}
	return PopupSelection_add_item (this, __, text, value);
}

void __fastcall
patch_Unit_perform_air_recon (Unit * this, int edx, int x, int y)
{
	int moves_plus_one = this->Body.Moves + p_bic_data->General.RoadsMovementRate;

	bool was_intercepted = false;
	if (is->current_config.intercept_recon_missions) {
		// Temporarily add vision on the target tile so the game plays the animation if the unit is show down by ground AA
		Tile_Body * tile = &tile_at (x, y)->Body;
		int saved_vis = tile->Visibility;
		tile->Visibility |= 1 << this->Body.CivID;
		was_intercepted = Unit_try_flying_over_tile (this, __, x, y);
		tile->Visibility = saved_vis;
	}

	if (! was_intercepted) {
		Unit_perform_air_recon (this, __, x, y);
		if (is->current_config.charge_one_move_for_recon_and_interception)
			this->Body.Moves = moves_plus_one;
	}
}

int __fastcall
patch_Unit_get_interceptor_max_moves (Unit * this)
{
	// Stop fighters from intercepting multiple times per turn without blitz
	if (is->current_config.charge_one_move_for_recon_and_interception &&
	    (this->Body.Status & USF_USED_ATTACK != 0) && ! UnitType_has_ability (&p_bic_data->UnitTypes[this->Body.UnitTypeID], __, UTA_Blitz))
		return 0;

	else
		return Unit_get_max_move_points (this);
}

int __fastcall
patch_Unit_get_moves_after_interception (Unit * this)
{
	if (is->current_config.charge_one_move_for_recon_and_interception) {
		this->Body.Status |= USF_USED_ATTACK; // Set status bit indicating that the interceptor has attacked this turn
		return this->Body.Moves + p_bic_data->General.RoadsMovementRate;
	} else
		return Unit_get_max_move_points (this);
}

void __fastcall
patch_Unit_set_state_after_interception (Unit * this, int edx, int new_state)
{
	if (! is->current_config.charge_one_move_for_recon_and_interception)
		Unit_set_state (this, __, new_state);

	// If fighters are supposed to be able to intercept multiple times per turn, then we can't knock them out of the interception state as soon as
	// they intercept something like in the base game. Instead, record this interception event so that we can clear their state at the start of
	// their next turn.
	else {
		struct interceptor_reset_list * irl = &is->interceptor_reset_lists[this->Body.CivID];
		reserve (sizeof irl->items[0], (void **)&irl->items, &irl->capacity, irl->count);
		irl->items[irl->count++] = (struct interception) {
			.unit_id = this->Body.ID,
			.x = this->Body.X,
			.y = this->Body.Y
		};
	}
}

void __fastcall
patch_Leader_begin_unit_turns (Leader * this)
{
	// Reset the states of all fighters that performed an interception on the previous turn.
	struct interceptor_reset_list * irl = &is->interceptor_reset_lists[this->ID];
	for (int n = 0; n < irl->count; n++) {
		struct interception * record = &irl->items[n];
		Unit * interceptor = get_unit_ptr (record->unit_id);
		if ((interceptor != NULL) &&
		    (interceptor->Body.CivID == this->ID) &&
		    (interceptor->Body.X == record->x) &&
		    (interceptor->Body.Y == record->y) &&
		    (interceptor->Body.UnitState == UnitState_Intercept))
			Unit_set_state (interceptor, __, 0);
	}
	irl->count = 0;

	// Reset all extra defensive bombard uses, if necessary
	if (is->extra_defensive_bombards.len > 0)
		for (int n = 0; n <= p_units->LastIndex; n++) {
			Unit * unit = get_unit_ptr (n);
			if ((unit != NULL) && (unit->Body.CivID == this->ID)) {
				int unused;
				if (itable_look_up (&is->extra_defensive_bombards, n, &unused))
					itable_insert (&is->extra_defensive_bombards, n, 0);
			}
		}

	Leader_begin_unit_turns (this);
}

Unit * __fastcall
patch_Fighter_find_actual_bombard_defender (Fighter * this, int edx, Unit * bombarder, int tile_x, int tile_y, int bombarder_civ_id, bool land_lethal, bool sea_lethal)
{
	if (is->bombard_stealth_target == NULL)
		return Fighter_find_defender_against_bombardment (this, __, bombarder, tile_x, tile_y, bombarder_civ_id, land_lethal, sea_lethal);
	else
		return is->bombard_stealth_target;
}

bool __fastcall
patch_Unit_try_flying_for_precision_strike (Unit * this, int edx, int x, int y)
{
	if (is->current_config.polish_non_air_precision_striking &&
	    (p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class != UTC_Air))
		// This method returns -1 when some kind of error occurs. In that case, return true implying the unit was shot down so the caller
		// doesn't do anything more. Otherwise, return false so it goes ahead and applies damage.
		return Unit_play_bombard_fire_animation (this, __, x, y) == -1;
	else
		return Unit_try_flying_over_tile (this, __, x, y);
}

void __fastcall
patch_Unit_play_bombing_anim_for_precision_strike (Unit * this, int edx, int x, int y)
{
	// For non-air units we don't play the bombard animation here (do it above instead) since it can fail, for whatever reason.
	if ((! is->current_config.polish_non_air_precision_striking) ||
	    (p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class == UTC_Air))
		Unit_play_bombing_animation (this, __, x, y);
}

int __fastcall
patch_Unit_play_anim_for_bombard_tile (Unit * this, int edx, int x, int y)
{
	Unit * stealth_attack_target = NULL;
	if (is->current_config.enable_stealth_attack_via_bombardment &&
	    (! is_online_game ()) &&
	    patch_Leader_is_tile_visible (&leaders[p_main_screen_form->Player_CivID], __, x, y)) {
		bool land_lethal = Unit_has_ability (this, __, UTA_Lethal_Land_Bombardment),
		     sea_lethal  = Unit_has_ability (this, __, UTA_Lethal_Sea_Bombardment);
		Unit * defender = Fighter_find_defender_against_bombardment (&p_bic_data->fighter, __, this, x, y, this->Body.CivID, land_lethal, sea_lethal);
		if (defender != NULL) {
			Unit * target;
			is->selecting_stealth_target_for_bombard = 1;
			bool got_one = patch_Unit_select_stealth_attack_target (this, __, defender->Body.CivID, x, y, true, &target);
			is->selecting_stealth_target_for_bombard = 0;
			if (got_one)
				is->bombard_stealth_target = target;
		}
	}

	return Unit_play_bombard_fire_animation (this, __, x, y);
}

void __fastcall
patch_Main_Screen_Form_issue_precision_strike_cmd (Main_Screen_Form * this, int edx, Unit * unit)
{
	UnitType * type = &p_bic_data->UnitTypes[unit->Body.UnitTypeID];
	if ((! is->current_config.polish_non_air_precision_striking) || (type->Unit_Class == UTC_Air))
		Main_Screen_Form_issue_precision_strike_cmd (this, __, unit);
	else {
		// issue_precision_strike_cmd will use the unit type's operational range. To make it use bombard range instead, place that value in
		// the operational range field temporarily. Conveniently, it's only necessary to do this temporary switch once, here, because the main
		// screen form stores a copy of the range for its own use and the method to actually perform the strike doesn't check the range.
		int saved_op_range = type->OperationalRange;
		type->OperationalRange = type->Bombard_Range;
		Main_Screen_Form_issue_precision_strike_cmd (this, __, unit);
		type->OperationalRange = saved_op_range;
	}
}

int __fastcall
patch_rand_bombard_target (void * this, int edx, int lim)
{
	// If we have a bombard stealth attack target set then return 2 so that the bombard damage will be applied to units not pop or buildings.
	return (is->bombard_stealth_target == NULL) ? rand_int (this, __, lim) : 2;
}

int __fastcall
patch_rand_int_to_dodge_city_aa (void * this, int edx, int lim)
{
	int tr = rand_int (this, __, lim);
	is->result_of_roll_to_dodge_city_aa = tr;
	return tr;
}

int __fastcall
patch_Unit_get_defense_to_dodge_city_aa (Unit * this)
{
	int defense = Unit_get_defense_strength (this);
	if (is->current_config.show_message_after_dodging_sam &&
	    (defense > is->result_of_roll_to_dodge_city_aa) &&
	    (this->Body.CivID == p_main_screen_form->Player_CivID))
		show_map_specific_text (this->Body.X, this->Body.Y, is->c3x_labels[CL_DODGED_SAM], 0);
	return defense;
}

int __fastcall
patch_Unit_get_defense_to_find_bombard_defender (Unit * this)
{
	if (is->current_config.immunize_aircraft_against_bombardment &&
	    (p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class == UTC_Air))
		return 0; // The caller is filtering out candidates with zero defense strength
	else
		return Unit_get_defense_strength (this);
}

int __cdecl
patch_get_WindowsFileBox_from_ini (LPCSTR key, int param_2, int param_3)
{
	// If the file path has already been determined, then avoid using the Windows file picker. This makes the later code to insert the path easier
	// since we only have to intercept the opening of the civ-style file picker instead of both that and the Windows one.
	if (is->load_file_path_override	== NULL)
		return get_int_from_conquests_ini (key, param_2, param_3);
	else
		return 0;
}

char const * __fastcall
patch_do_open_load_game_file_picker (void * this)
{
	if (is->load_file_path_override != NULL) {
		char const * tr = is->load_file_path_override;
		is->load_file_path_override = NULL;
		return tr;
	} else
		return open_load_game_file_picker (this);
}

int __fastcall
patch_show_intro_after_load_popup (void * this, int edx, int param_1, int param_2)
{
	if (! is->suppress_intro_after_load_popup)
		return patch_show_popup (this, __, param_1, param_2);
	else {
		is->suppress_intro_after_load_popup = 0;
		return 0;
	}
}

void * __cdecl
patch_do_load_game (char * param_1)
{
	void * tr = do_load_game (param_1);

	if (is->current_config.restore_unit_directions_on_game_load && (p_units->Units != NULL))
		for (int n = 0; n <= p_units->LastIndex; n++) {
			Unit * unit = get_unit_ptr (n);
			if ((unit != NULL) && (unit->Body.UnitState != UnitState_Fortifying)) {
				if (Map_in_range (&p_bic_data->Map, __, unit->Body.X, unit->Body.Y) &&
				    Map_in_range (&p_bic_data->Map, __, unit->Body.PrevMoveX, unit->Body.PrevMoveY)) {
					int dx = unit->Body.X - unit->Body.PrevMoveX, dy = unit->Body.Y - unit->Body.PrevMoveY;
					int dir = -1;
					if      ((dx ==  1) && (dy == -1)) dir = DIR_NE;
					else if ((dx ==  2) && (dy ==  0)) dir = DIR_E;
					else if ((dx ==  1) && (dy ==  1)) dir = DIR_SE;
					else if ((dx ==  0) && (dy ==  2)) dir = DIR_S;
					else if ((dx == -1) && (dy ==  1)) dir = DIR_SW;
					else if ((dx == -2) && (dy ==  0)) dir = DIR_W;
					else if ((dx == -1) && (dy == -1)) dir = DIR_NW;
					else if ((dx ==  0) && (dy == -2)) dir = DIR_N;
					if (dir >= 0)
						unit->Body.Animation.summary.direction = dir;
				}
			}
		}

	// Apply era aliases
	for (int n = 0; n < 32; n++)
		if (*p_player_bits & (1 << n))
			apply_era_specific_names (&leaders[n]);

	if (is->current_config.apply_grid_ini_setting_on_game_load) {
		int grid_on = get_int_from_conquests_ini ("GridOn", 0, 0);
		Map_Renderer * mr = &p_bic_data->Map.Renderer;
		if (grid_on && ! mr->MapGrid_Flag)
			mr->vtable->m68_Toggle_Grid (mr);
	}

	return tr;
}

void *
load_game_ex (char const * file_path, int suppress_intro_popup)
{
	is->suppress_intro_after_load_popup = suppress_intro_popup;
	is->load_file_path_override = file_path;
	return patch_do_load_game (NULL);
}

int __fastcall
patch_show_movement_phase_popup (void * this, int edx, int param_1, int param_2)
{
	int tr = patch_show_popup (this, __, param_1, param_2);

	int player_civ_id = p_main_screen_form->Player_CivID;
	int replay_for_players = is->replay_for_players; // Store this b/c it gets reset on game load
	if (replay_for_players & 1<<player_civ_id) {
		is->showing_hotseat_replay = true;

		patch_do_save_game (hotseat_resume_save_path, 1, 0);
		load_game_ex (hotseat_replay_save_path, 1);
		p_main_screen_form->Player_CivID = player_civ_id;

		// Re-enable the GUI so the minimap is visible during the replay. We must also reset the minimap & redraw it so that it reflects the
		// player we just seated (above) instead of leftover data from the last player.
		Main_GUI * main_gui = &p_main_screen_form->GUI;
		main_gui->is_enabled = 1;
		Navigator_Data_reset (&main_gui->Navigator_Data);
		main_gui->Base.vtable->m73_call_m22_Draw ((Base_Form *)main_gui);

		perform_interturn ();
		load_game_ex (hotseat_resume_save_path, 1);
		p_main_screen_form->is_now_loading_game = 0;

		// Restore the replay_for_players variable b/c it gets cleared when loading a game. Also mask out the bit for the player we just
		// showed the replay to.
		is->replay_for_players = replay_for_players & ~(1<<player_civ_id);

		is->showing_hotseat_replay = false;
	}

	return tr;
}

// Returns a set of player bits containing only those players that are human and can see at least one AI unit. For speed and simplicity, does not
// account for units' invisibility ability, units are considered visible as long as they're on a visible tile.
int
find_human_players_seeing_ai_units ()
{
	int tr = 0;
	Map * map = &p_bic_data->Map;
	if (map->Tiles != NULL)
		for (int n_tile = 0; n_tile < map->TileCount; n_tile++) {
			Tile * tile = map->Tiles[n_tile];
			Tile_Body * body = &tile->Body;
			int human_vis_bits = (body->FOWStatus | body->V3 | body->Visibility | body->field_D0_Visibility) & *p_human_player_bits;
			if (human_vis_bits != 0) // If any human players can see this tile
				for (int n_player = 0; n_player < 32; n_player++)
					if (human_vis_bits & 1<<n_player) {
						int unused;
						int unit_id = TileUnits_TileUnitID_to_UnitID (p_tile_units, __, tile->TileUnitID, &unused);
						Unit * unit = get_unit_ptr (unit_id);
						if ((unit != NULL) &&
						    ((*p_human_player_bits & 1<<unit->Body.CivID) == 0)) {
							tr |= 1<<n_player;
							if (tr == *p_player_bits)
								return tr;
						}
					}
		}
	return tr;
}

void __cdecl
patch_perform_interturn_in_main_loop ()
{
	int save_replay = is->current_config.replay_ai_moves_in_hotseat_games &&
		(*p_is_offline_mp_game && ! *p_is_pbem_game); // offline MP but not PBEM => we're in a hotseat game
	int ai_unit_vis_before;
	if (save_replay) {
		ai_unit_vis_before = find_human_players_seeing_ai_units ();
		int toggleable_rules = *p_toggleable_rules;
		*p_toggleable_rules |= TR_PRESERVE_RANDOM_SEED; // Make sure preserve random seed is on for the replay save
		patch_do_save_game (hotseat_replay_save_path, 1, 0);
		*p_toggleable_rules = toggleable_rules;
	}

	is->players_saw_ai_unit = 0; // Clear bits. After perform_interturn, each set bit will indicate a player that has seen an AI unit move
	long long ts_before;
	QueryPerformanceCounter ((LARGE_INTEGER *)&ts_before);
	is->time_spent_paused_during_popup = 0;
	is->time_spent_computing_city_connections = 0;
	is->count_calls_to_recompute_city_connections = 0;
	unsigned saved_prefs = *p_preferences;
	if (is->current_config.measure_turn_times)
		*p_preferences &= ~(P_ANIMATE_BATTLES | P_SHOW_FRIEND_MOVES | P_SHOW_ENEMY_MOVES);

	perform_interturn ();

	if (is->current_config.measure_turn_times) {
		long long ts_after;
		QueryPerformanceCounter ((LARGE_INTEGER *)&ts_after);
		long long perf_freq;
		QueryPerformanceFrequency ((LARGE_INTEGER *)&perf_freq);
		int turn_time_in_ms = 1000 * (ts_after - ts_before - is->time_spent_paused_during_popup) / perf_freq;
		int city_con_time_in_ms = 1000 * is->time_spent_computing_city_connections / perf_freq;
		int road_time_in_ms = 1000 * is->time_spent_filling_roads / perf_freq;

		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_INFO", -1, 0, 0, 0);
		char msg[1000];

		struct c3x_opt {
			bool is_active;
			char * name;
		} opts[] = {{is->current_config.optimize_improvement_loops, "improv. loops"},
			    {is->current_config.enable_trade_net_x && (is->tnx_init_state == IS_OK), "trade net x"}};
		char opt_list[1000];
		memset (opt_list, 0, sizeof opt_list);
		strncpy (opt_list, "^C3X optimizations: ", sizeof opt_list);
		bool any_active_opts = false;
		for (int n = 0; n < ARRAY_LEN (opts); n++)
			if (opts[n].is_active) {
				char * cursor = &opt_list[strlen (opt_list)];
				snprintf (cursor, opt_list + (sizeof opt_list) - cursor, "%s%s", any_active_opts ? ", " : "", opts[n].name);
				any_active_opts = true;
			}
		if (! any_active_opts) {
			char * cursor = &opt_list[strlen (opt_list)];
			strncpy (cursor, "None", opt_list + (sizeof opt_list) - cursor);
		}
		PopupForm_add_text (popup, __, (char *)opt_list, false);

		snprintf (msg, sizeof msg, "^Turn time: %d.%03d sec", turn_time_in_ms/1000, turn_time_in_ms%1000);
		PopupForm_add_text (popup, __, (char *)msg, false);
		snprintf (msg, sizeof msg, "^  Recomputing city connections: %d.%03d sec (%d calls)",
			  city_con_time_in_ms/1000, city_con_time_in_ms%1000,
			  is->count_calls_to_recompute_city_connections);
		PopupForm_add_text (popup, __, (char *)msg, false);
		snprintf (msg, sizeof msg, "^    Flood filling road network: %d.%03d sec",
			  road_time_in_ms/1000, road_time_in_ms%1000);
		PopupForm_add_text (popup, __, (char *)msg, false);
		patch_show_popup (popup, __, 0, 0);

		*p_preferences = saved_prefs;
	}

	if (save_replay) {
		int last_human_player_bit = 0; {
			for (int n = 0; n < 32; n++)
				if (*p_human_player_bits & 1<<n)
					last_human_player_bit = 1<<n;
		}

		// Set player bits indicating which players should see the replay. This includes all human players that could see an AI unit at the
		// start of the interturn or that saw an AI unit move or bombard during the interturn, excluding the last human player.
		is->replay_for_players = (ai_unit_vis_before | (is->players_saw_ai_unit & *p_human_player_bits)) & ~last_human_player_bit;
	}
}

void __cdecl
patch_initialize_map_music (int civ_id, int era_id, bool param_3)
{
	if (! is->showing_hotseat_replay)
		initialize_map_music (civ_id, era_id, param_3);
}

void __stdcall
patch_deinitialize_map_music ()
{
	if (! is->showing_hotseat_replay)
		deinitialize_map_music ();
}

void __fastcall
patch_Fighter_do_bombard_tile (Fighter * this, int edx, Unit * unit, int neighbor_index, int mp_tile_x, int mp_tile_y)
{
	// Check if we're going to do PTW-like targeting, if not fall back on the base game's do_bombard_tile method. We'll also fall back on that
	// method in the case where we're in an online game and the bombard can't happen b/c the tile is occupied by another battle. In that case, no
	// bombard is possible but we'll call the base method anyway since it will show a little message saying as much.
	if (uses_ptw_arty_targeting (unit) &&
	    (is->bombard_stealth_target == NULL) &&
	    ! (is_online_game () && mp_check_current_combat (p_mp_object, __, mp_tile_x, mp_tile_y))) {

		City * city; {
			int dx, dy;
			neighbor_index_to_diff (neighbor_index, &dx, &dy);
			int tile_x = unit->Body.X + dx, tile_y = unit->Body.Y + dy;
			wrap_tile_coords (&p_bic_data->Map, &tile_x, &tile_y);
			city = city_at (tile_x, tile_y);
		}

		int rv;
		if ((city != NULL) && ((rv = rand_int (p_rand_object, __, 3)) < 2))
			Fighter_damage_city_by_bombardment (this, __, unit, city, rv, 0);
		else
			Fighter_do_bombard_tile (this, __, unit, neighbor_index, mp_tile_x, mp_tile_y);

	} else
		Fighter_do_bombard_tile (this, __, unit, neighbor_index, mp_tile_x, mp_tile_y);
}

bool __fastcall
patch_Unit_check_king_for_defense_priority (Unit * this, int edx, enum UnitTypeAbilities king_ability)
{
	return (! is->current_config.ignore_king_ability_for_defense_priority) || (*p_toggleable_rules & (TR_REGICIDE | TR_MASS_REGICIDE)) ?
		Unit_has_ability (this, __, king_ability) :
		false;
}

void WINAPI
patch_get_local_time_for_unit_ini (LPSYSTEMTIME lpSystemTime)
{
	GetLocalTime (lpSystemTime);
	if (is->current_config.no_elvis_easter_egg && (lpSystemTime->wMonth == 1) && (lpSystemTime->wDay == 8))
		lpSystemTime->wDay = 9;
}

bool __fastcall
patch_Leader_could_buy_tech_for_trade_screen (Leader * this, int edx, int tech_id, int from_civ_id)
{
	// Temporarily remove the untradable flag so this tech is listed on the screen instead of skipped over. After all the items have been
	// assembled, we'll go back and disable the untradable techs.
	if (is->current_config.show_untradable_techs_on_trade_screen) {
		int saved_flags = p_bic_data->Advances[tech_id].Flags;
		p_bic_data->Advances[tech_id].Flags &= ~ATF_Cannot_Be_Traded;
		bool tr = this->vtable->could_buy_tech (this, __, tech_id, from_civ_id);
		p_bic_data->Advances[tech_id].Flags = saved_flags;
		return tr;

	} else
		return this->vtable->could_buy_tech (this, __, tech_id, from_civ_id);
}

void __fastcall
patch_DiploForm_assemble_tradable_items (DiploForm * this)
{
	DiploForm_assemble_tradable_items (this);

	// Disable (gray out) all untradable techs
	if (is->current_config.show_untradable_techs_on_trade_screen)
		for (int n = 0; n < p_bic_data->AdvanceCount; n++)
			if (p_bic_data->Advances[n].Flags & ATF_Cannot_Be_Traded) {
				this->tradable_technologies[n].can_be_bought = 0;
				this->tradable_technologies[n].can_be_sold   = 0;
			}
}

bool __fastcall
patch_City_can_trade_via_water (City * this)
{
	if (is->current_config.optimize_improvement_loops) {
		for (int n = 0; n < is->water_trade_improvs.count; n++)
			if (has_active_building (this, is->water_trade_improvs.items[n]))
				return true;
		return false;
	} else
		return City_can_trade_via_water (this);
}

bool __fastcall
patch_City_can_trade_via_air (City * this)
{
	if (is->current_config.optimize_improvement_loops) {
		for (int n = 0; n < is->air_trade_improvs.count; n++)
			if (has_active_building (this, is->air_trade_improvs.items[n]))
				return true;
		return false;
	} else
		return City_can_trade_via_air (this);
}

int __fastcall
patch_City_get_building_defense_bonus (City * this)
{
	if (is->current_config.optimize_improvement_loops) {
		int tr = 0;
		int is_size_level_1 = (this->Body.Population.Size <= p_bic_data->General.MaximumSize_City) &&
			(this->Body.Population.Size <= p_bic_data->General.MaximumSize_Town);
		for (int n = 0; n < is->combat_defense_improvs.count; n++) {
			int improv_id = is->combat_defense_improvs.items[n];
			Improvement * improv = &p_bic_data->Improvements[improv_id];
			if ((is_size_level_1 || (improv->Combat_Bombard == 0)) && has_active_building (this, improv_id)) {
				int multiplier;
				if ((improv->Combat_Bombard > 0) &&
				    (Leader_count_wonders_with_flag (&leaders[(this->Body).CivID], __, ITW_Doubles_City_Defenses, NULL) > 0))
					multiplier = 2;
				else
					multiplier = 1;

				int building_defense = multiplier * improv->Combat_Defence;
				if (building_defense > tr)
					tr = building_defense;
			}
		}
		return tr;
	} else
		return City_get_building_defense_bonus (this);
}

bool __fastcall
patch_City_shows_harbor_icon (City * this)
{
	return is->current_config.city_icons_show_unit_effects_not_trade ?
		City_count_improvements_with_flag (this, __, ITF_Veteran_Sea_Units) > 0 :
		patch_City_can_trade_via_water (this);
}

bool __fastcall
patch_City_shows_airport_icon (City * this)
{
	return is->current_config.city_icons_show_unit_effects_not_trade ?
		City_count_improvements_with_flag (this, __, ITF_Veteran_Air_Units) > 0 :
		patch_City_can_trade_via_air (this);
}

int __fastcall
patch_Unit_eval_escort_requirement (Unit * this)
{
	UnitType * type = &p_bic_data->UnitTypes[this->Body.UnitTypeID];
	int ai_strat = type->AI_Strategy;

	// Apply special escort rules
	if (is->current_config.dont_escort_unflagged_units && ! UnitType_has_ability (type, __, UTA_Requires_Escort))
		return 0;
	if (is->current_config.use_offensive_artillery_ai && (ai_strat & UTAI_Artillery))
		return 1;

	int base = Unit_eval_escort_requirement (this);
	if ((~*p_human_player_bits & 1<<this->Body.CivID) && // Applying this to a human player's units messes with group movement (for some reason)
	    (ai_strat & (UTAI_Naval_Transport | UTAI_Naval_Carrier | UTAI_Naval_Missile_Transport)))
		return not_above (is->current_config.max_ai_naval_escorts, base);
	else
		return base;
}

bool __fastcall
patch_Unit_has_enough_escorters_present (Unit * this)
{
	UnitType * type = &p_bic_data->UnitTypes[this->Body.UnitTypeID];
	if (is->current_config.dont_escort_unflagged_units && ! UnitType_has_ability (type, __, UTA_Requires_Escort))
		return true;
	else
		return Unit_has_enough_escorters_present (this);
}

void __fastcall
patch_Unit_check_escorter_health (Unit * this, int edx, bool * has_any_escort_present, bool * any_escorter_cant_heal)
{
	UnitType * type = &p_bic_data->UnitTypes[this->Body.UnitTypeID];
	if (is->current_config.dont_escort_unflagged_units && ! UnitType_has_ability (type, __, UTA_Requires_Escort)) {
		*has_any_escort_present = true;
		*any_escorter_cant_heal = true; // Returning true here indicates the unit should not stop to wait for its escorter(s) to heal.
	} else
		Unit_check_escorter_health (this, __, has_any_escort_present, any_escorter_cant_heal);
}

void __fastcall
patch_Leader_unlock_technology (Leader * this, int edx, int tech_id, bool param_2, bool param_3, bool param_4)
{
	int * p_stack = (int *)&tech_id;
	int ret_addr = p_stack[-1];

	Leader_unlock_technology (this, __, tech_id, param_2, param_3, param_4);

	// If this method was not called during game initialization
	if ((ret_addr != ADDR_UNLOCK_TECH_AT_INIT_1) &&
	    (ret_addr != ADDR_UNLOCK_TECH_AT_INIT_2) &&
	    (ret_addr != ADDR_UNLOCK_TECH_AT_INIT_3)) {

		// If this tech obsoletes some building and we're configured to fix the maintenance bug then recompute city maintenance.
		if (is->current_config.patch_maintenance_persisting_for_obsolete_buildings) {
			bool obsoletes_anything = false;
			for (int n = 0; n < p_bic_data->ImprovementsCount; n++)
				if (p_bic_data->Improvements[n].ObsoleteID == tech_id) {
					obsoletes_anything = true;
					break;
				}
			if (obsoletes_anything)
				Leader_recompute_buildings_maintenance (this);
		}
	}
}

int __fastcall
patch_City_get_improv_maintenance_for_ui (City * this, int edx, int improv_id)
{
	Improvement * improv = &p_bic_data->Improvements[improv_id];
	if (is->current_config.patch_maintenance_persisting_for_obsolete_buildings &&
	    (improv->ObsoleteID >= 0) && Leader_has_tech (&leaders[this->Body.CivID], __, improv->ObsoleteID))
		return 0;
	else
		return City_get_improvement_maintenance (this, __, improv_id);
}

// Patch for barbarian diagonal bug. This bug is a small mistake in the original code, maybe a copy+paste error. The original code tries to loop over
// tiles around the barb unit by incrementing a neighbor index and coverting it to tile coords (like normal). The problem is that after it converts
// the neighbor index to dx and dy, it adds dx to both coords of the unit's position instead of using dy. The fix is simply to subtract off dx and add
// in dy when the Y coord is passed to Map::wrap_vert.
void __cdecl
patch_neighbor_index_to_diff_for_barb_ai (int neighbor_index, int * out_x, int * out_y)
{
	neighbor_index_to_diff (neighbor_index, out_x, out_y);
	is->barb_diag_patch_dy_fix = *out_y - *out_x;
}
int __fastcall
patch_Map_wrap_vert_for_barb_ai (Map * this, int edx, int y)
{
	return Map_wrap_vert (this, __, is->current_config.patch_barbarian_diagonal_bug ? (y + is->barb_diag_patch_dy_fix) : y);
}

void __fastcall
patch_Leader_do_production_phase (Leader * this)
{
	// Force-activate the barbs if there are any barb cities on the map and barb city capturing is enabled. This is necessary for barb city
	// production to work given how it's currently implemented.
	if (is->current_config.enable_city_capture_by_barbarians && (this->ID == 0)) {
		int any_barb_cities = 0;
		FOR_CITIES_OF (coi, this->ID) {
			any_barb_cities = 1;
			break;
		}
		if (any_barb_cities)
			is->force_barb_activity_for_cities = 1;
	}

	// If barbs are force-activated, make sure their activity level is at least 1 (sedentary).
	int * p_barb_activity = &p_bic_data->Map.World.Final_Barbarians_Activity;
	int saved_barb_activity = *p_barb_activity;
	if (is->force_barb_activity_for_cities && (*p_barb_activity <= 0))
		*p_barb_activity = 1;

	Leader_do_production_phase (this);

	if (is->force_barb_activity_for_cities) {
		*p_barb_activity = saved_barb_activity;
		is->force_barb_activity_for_cities = 0;
	}
}

// This function counts the number of players in the game. The return value will be compared to the number of cities on the map to determine if there
// are enough cities (per player) to unlock barb production. If barb activity is forced on, return zero so that the check always passes.
int __cdecl
patch_count_player_bits_for_barb_prod (unsigned int bit_field)
{
	return is->force_barb_activity_for_cities ? 0 : count_set_bits (bit_field);
}

Tile * __fastcall
patch_Map_get_tile_to_check_visibility (Map * this, int edx, int index)
{
	Tile * tr = Map_get_tile (this, __, index);
	int is_hotseat_game = *p_is_offline_mp_game && ! *p_is_pbem_game;
	if (is_hotseat_game && is->current_config.share_visibility_in_hoseat) {
		int human_bits = *p_human_player_bits;
		is->dummy_tile->Body.Fog_Of_War          = tr->Body.Fog_Of_War          | ((tr->Body.Fog_Of_War          & human_bits) != 0 ? human_bits : 0);
		is->dummy_tile->Body.FOWStatus           = tr->Body.FOWStatus           | ((tr->Body.FOWStatus           & human_bits) != 0 ? human_bits : 0);
		is->dummy_tile->Body.V3                  = tr->Body.V3                  | ((tr->Body.V3                  & human_bits) != 0 ? human_bits : 0);
		is->dummy_tile->Body.Visibility          = tr->Body.Visibility          | ((tr->Body.Visibility          & human_bits) != 0 ? human_bits : 0);
		is->dummy_tile->Body.field_D0_Visibility = tr->Body.field_D0_Visibility | ((tr->Body.field_D0_Visibility & human_bits) != 0 ? human_bits : 0);
		tr = is->dummy_tile;
	}
	is->tile_returned_for_visibility_check = tr;
	return tr;
}

Tile * __fastcall
patch_Map_get_tile_to_check_visibility_again (Map * this, int edx, int index)
{
	return is->tile_returned_for_visibility_check;
}

// Same as above except this method uses the FOWStatus field instead of Fog_Of_War
Tile * __fastcall
patch_Map_get_tile_for_fow_status_check (Map * this, int edx, int index)
{
	Tile * tile = Map_get_tile (this, __, index);
	int is_hotseat_game = *p_is_offline_mp_game && ! *p_is_pbem_game;
	if (is_hotseat_game && is->current_config.share_visibility_in_hoseat) {
		is->dummy_tile->Body.FOWStatus = ((tile->Body.FOWStatus & *p_human_player_bits) != 0) << p_main_screen_form->Player_CivID;
		return is->dummy_tile;
	} else
		return tile;
}

Tile * __cdecl
patch_tile_at_to_check_visibility (int x, int y)
{
	return patch_Map_get_tile_to_check_visibility (&p_bic_data->Map, __, (p_bic_data->Map.Width >> 1) * y + (x >> 1));
}

Tile * __cdecl
patch_tile_at_to_check_visibility_again (int x, int y)
{
	return is->tile_returned_for_visibility_check;
}

unsigned __fastcall
patch_Tile_m42_Get_Overlays (Tile * this, int edx, byte visible_to_civ)
{
	unsigned base_vis_overlays = Tile_m42_Get_Overlays (this, __, visible_to_civ);
	if ((visible_to_civ != 0) && // if we're seeing from a player's persp. instead of seeing the actual overlays AND
	    is->current_config.share_visibility_in_hoseat && // shared hotseat vis is enabled AND
	    ((1 << visible_to_civ) & *p_human_player_bits) && // the perspective is of a human player AND
	    (base_vis_overlays != this->Overlays) && // that player can't already see all the actual overlays AND
	    (*p_is_offline_mp_game && ! *p_is_pbem_game)) { // we're in a hotseat game

		// Check if there's another human player that can see the actual overlays. If so, give that info to this player and return it.
		unsigned player_bits = *(unsigned *)p_human_player_bits >> 1;
		int n_player = 1;
		while (player_bits != 0) {
			if ((player_bits & 1) && (n_player != visible_to_civ) &&
			    (Tile_m42_Get_Overlays (this, __, n_player) == this->Overlays)) {
				this->Body.Visibile_Overlays[visible_to_civ] = this->Overlays;
				return this->Overlays;
			}
			player_bits >>= 1;
			n_player++;
		}

		return base_vis_overlays;
	} else
		return base_vis_overlays;
}

int __fastcall
patch_Tile_check_water_for_sea_zoc (Tile * this)
{
	if ((is->current_config.special_zone_of_control_rules & (SZOCR_AMPHIBIOUS | SZOCR_AERIAL)) == 0)
		return this->vtable->m35_Check_Is_Water (this);
	else
		return 1; // The caller will skip ZoC logic if this is a land tile without a city because the targeted unit is a sea unit. Instead
			  // return 1, so all tiles are considered sea tiles, so we can run the ZoC logic for land units or air units on land.
}

int __fastcall
patch_Tile_check_water_for_land_zoc (Tile * this)
{
	// Same as above except this time we want to consider all tiles to be land
	return ((is->current_config.special_zone_of_control_rules & (SZOCR_AMPHIBIOUS | SZOCR_AERIAL)) == 0) ?
		this->vtable->m35_Check_Is_Water (this) :
		0;
}


int __fastcall
patch_Unit_get_attack_strength_for_sea_zoc (Unit * this)
{
	return (p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class == UTC_Sea) ? Unit_get_attack_strength (this) : 0;
}

int __fastcall
patch_Unit_get_attack_strength_for_land_zoc (Unit * this)
{
	return (p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class == UTC_Land) ? Unit_get_attack_strength (this) : 0;
}

Unit * __fastcall
patch_Main_Screen_Form_find_visible_unit (Main_Screen_Form * this, int edx, int tile_x, int tile_y, Unit * excluded)
{
	struct unit_display_override * override = &is->unit_display_override;
	if ((override->unit_id >= 0) && (override->tile_x == tile_x) && (override->tile_y == tile_y)) {
		Unit * unit = get_unit_ptr (override->unit_id);
		if (unit != NULL) {
			if ((unit->Body.X == tile_x) && (unit->Body.Y == tile_y))
				return unit;
		}
	}

	return Main_Screen_Form_find_visible_unit (this, __, tile_x, tile_y, excluded);
}

void __fastcall
patch_Animator_play_zoc_animation (Animator * this, int edx, Unit * unit, AnimationType anim_type, bool param_3)
{
	if (p_bic_data->UnitTypes[unit->Body.UnitTypeID].Unit_Class != UTC_Air)
		Animator_play_one_shot_unit_animation (this, __, unit, anim_type, param_3);
}

bool __fastcall
patch_Fighter_check_zoc_anim_visibility (Fighter * this, int edx, Unit * attacker, Unit * defender, bool param_3)
{
	// If we've reached this point in the code (in the calling method) then a unit has been selected to exert zone of control and it has passed
	// its dice roll to cause damage. Stash its pointer for possible use later.
	is->zoc_interceptor = attacker;

	// If an air unit was selected, pre-emptively undo the damage from ZoC since we'll want to run our own bit of logic to do that (the air unit
	// may still get shot down). Return false from this function to skip over all of the animation logic in the caller since it wouldn't work for
	// aircraft.
	if (p_bic_data->UnitTypes[attacker->Body.UnitTypeID].Unit_Class == UTC_Air) {
		defender->Body.Damage -= 1;
		return false;

	// Repeat a check done by the caller. We've deleted this check to ensure that this function always gets called so we can grab the interceptor.
	} else if (attacker->Body.Animation.field_111 == 0)
		return false;

	else {
		bool tr = Fighter_check_combat_anim_visibility (this, __, attacker, defender, param_3);

		// If necessary, set up to ensure the unit's attack animation is visible. This means forcing it to the top of its stack and
		// temporarily unfortifying it if it's fortified. (If it's fortified, the animation is occasionally not visible. Don't know why.)
		if (tr && is->current_config.show_zoc_attacks_from_mid_stack) {
			is->unit_display_override = (struct unit_display_override) { attacker->Body.ID, attacker->Body.X, attacker->Body.Y };
			if (attacker->Body.UnitState == UnitState_Fortifying) {
				Unit_set_state (attacker, __, 0);
				is->refortify_interceptor_after_zoc = true;
			}
		}

		return tr;
	}
}

void __fastcall
patch_Fighter_apply_zone_of_control (Fighter * this, int edx, Unit * unit, int from_x, int from_y, int to_x, int to_y)
{
	is->zoc_interceptor = NULL;
	is->zoc_defender = unit;
	is->refortify_interceptor_after_zoc = false;
	struct unit_display_override saved_udo = is->unit_display_override;
	Fighter_apply_zone_of_control (this, __, unit, from_x, from_y, to_x, to_y);

	// Actually exert ZoC if an air unit managed to do so.
	if ((is->zoc_interceptor != NULL) && (p_bic_data->UnitTypes[is->zoc_interceptor->Body.UnitTypeID].Unit_Class == UTC_Air)) {
		bool intercepted = Unit_try_flying_over_tile (is->zoc_interceptor, __, from_x, from_y);
		if (! intercepted) {
			Unit_play_bombing_animation (is->zoc_interceptor, __, from_x, from_y);
			unit->Body.Damage = not_below (0, unit->Body.Damage + 1);
		}
	}

	if (is->refortify_interceptor_after_zoc)
		Unit_set_state (is->zoc_interceptor, __, UnitState_Fortifying);
	is->unit_display_override = saved_udo;
}

// These two patches replace two function calls in Unit::move_to_adjacent_tile that come immediately after the unit has been subjected to zone of
// control. These calls recheck that the move is valid, not sure why. Here they're patched to indicate that the move in invalid when the unit was
// previously killed by ZoC. This causes move_to_adjacent_tile to return early without running the code that would place the unit on the destination
// tile and, for example, capturing an enemy city there.
int __fastcall
patch_Trade_Net_get_move_cost_after_zoc (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, unsigned param_7, int neighbor_index, Trade_Net_Distance_Info * dist_info)
{
	return ((is->current_config.special_zone_of_control_rules & SZOCR_LETHAL) == 0) || ((Unit_get_max_hp (unit) - unit->Body.Damage) > 0) ?
		patch_Trade_Net_get_movement_cost (this, __, from_x, from_y, to_x, to_y, unit, civ_id, param_7, neighbor_index, dist_info) :
		-1;
}
AdjacentMoveValidity __fastcall
patch_Unit_can_move_after_zoc (Unit * this, int edx, int neighbor_index, int param_2)
{
	return ((is->current_config.special_zone_of_control_rules & SZOCR_LETHAL) == 0) || ((Unit_get_max_hp (this) - this->Body.Damage) > 0) ?
		patch_Unit_can_move_to_adjacent_tile (this, __, neighbor_index, param_2) :
		AMV_1;
}

// Checks unit's HP after it was possibly hit by ZoC and deals with the consequences if it's dead. Does nothing if config option to make ZoC lethal
// isn't set or if interceptor is NULL. Returns true if the unit was killed, false otherwise.
bool
check_life_after_zoc (Unit * unit, Unit * interceptor)
{
	if ((is->current_config.special_zone_of_control_rules & SZOCR_LETHAL) && (interceptor != NULL) &&
	    ((Unit_get_max_hp (unit) - unit->Body.Damage) <= 0)) {
		Unit_score_kill (interceptor, __, unit, false);
		if ((! is_online_game ()) && Fighter_check_combat_anim_visibility (&p_bic_data->fighter, __, interceptor, unit, true))
			Animator_play_one_shot_unit_animation (&p_main_screen_form->animator, __, unit, AT_DEATH, false);
		patch_Unit_despawn (unit, __, interceptor->Body.CivID, 0, 0, 0, 0, 0, 0);
		return true;
	} else
		return false;
}

int __fastcall
patch_Unit_move_to_adjacent_tile (Unit * this, int edx, int neighbor_index, bool param_2, int param_3, byte param_4)
{
	is->moving_unit_to_adjacent_tile = true;

	is->zoc_interceptor = NULL;
	int tr = Unit_move_to_adjacent_tile (this, __, neighbor_index, param_2, param_3, param_4);
	if (check_life_after_zoc (this, is->zoc_interceptor))
		tr = ! is_online_game (); // This is what the original method returns when the unit was destroyed in combat

	is->temporarily_disallow_lethal_zoc = false;
	is->moving_unit_to_adjacent_tile = false;
	return tr;
}

int __fastcall
patch_Unit_teleport (Unit * this, int edx, int tile_x, int tile_y, Unit * unit_telepad)
{
	is->zoc_interceptor = NULL;
	int tr = Unit_teleport (this, __, tile_x, tile_y, unit_telepad);
	check_life_after_zoc (this, is->zoc_interceptor);
	return tr;
}

bool
can_do_defensive_bombard (Unit * unit, UnitType * type)
{
	if ((type->Bombard_Strength > 0) && (! Unit_has_ability (unit, __, UTA_Cruise_Missile))) {
		if ((unit->Body.Status & USF_USED_DEFENSIVE_BOMBARD) == 0) // has not already done DB this turn
			return true;

		// If the "blitz" special DB rule is activated and this unit has blitz, check if it still has an extra DB to use
		else if ((is->current_config.special_defensive_bombard_rules & SDBR_BLITZ) && UnitType_has_ability (type, __, UTA_Blitz)) {
			int extra_dbs;
			int got_value = itable_look_up (&is->extra_defensive_bombards, unit->Body.ID, &extra_dbs);
			if (! got_value)
				extra_dbs = 0;
			return type->Movement > extra_dbs + 1;

		} else
			return false;
	} else
		return false;
}

Unit * __fastcall
patch_Fighter_find_defensive_bombarder (Fighter * this, int edx, Unit * attacker, Unit * defender)
{
	int special_rules = is->current_config.special_defensive_bombard_rules;
	if (special_rules == 0)
		return Fighter_find_defensive_bombarder (this, __, attacker, defender);
	else {
		enum UnitTypeClasses attacker_class = p_bic_data->UnitTypes[attacker->Body.UnitTypeID].Unit_Class;
		int attacker_has_one_hp = Unit_get_max_hp (attacker) - attacker->Body.Damage <= 1;

		Tile * defender_tile = tile_at (defender->Body.X, defender->Body.Y);
		if ((Unit_get_defense_strength (attacker) < 1) || // if attacker cannot defend OR
		    (defender_tile == NULL) || (defender_tile == p_null_tile) || // defender tile is invalid OR
		    (((special_rules & SDBR_LETHAL) == 0) && attacker_has_one_hp) || // (DB is non-lethal AND attacker has one HP remaining) OR
		    ((special_rules & SDBR_NOT_INVISIBLE) && ! Unit_is_visible_to_civ (attacker, __, defender->Body.CivID, 1))) // (invisible units are immune to DB AND attacker is invisible)
			return NULL;

		Unit * tr = NULL;
		int highest_strength = -1;
		enum UnitTypeAbilities lethal_bombard_req = (attacker_class == UTC_Sea) ? UTA_Lethal_Sea_Bombardment : UTA_Lethal_Land_Bombardment;
		FOR_UNITS_ON (uti, defender_tile) {
			Unit * candidate = uti.unit;
			UnitType * candidate_type = &p_bic_data->UnitTypes[candidate->Body.UnitTypeID];
			if (can_do_defensive_bombard (candidate, candidate_type) &&
			    (candidate_type->Bombard_Strength > highest_strength) &&
			    (candidate != defender) &&
			    (Unit_get_containing_army (candidate) != defender) &&
			    ((attacker_class == candidate_type->Unit_Class) ||
			     ((special_rules & SDBR_AERIAL) &&
			      (candidate_type->Unit_Class == UTC_Air) &&
			      (candidate_type->Air_Missions & UCV_Bombing)) ||
			     ((special_rules & SDBR_DOCKED_VS_LAND) &&
			      (candidate_type->Unit_Class == UTC_Sea) &&
			      (get_city_ptr (defender_tile->CityID) != NULL))) &&
			    ((! attacker_has_one_hp) || UnitType_has_ability (candidate_type, __, lethal_bombard_req))) {
				tr = candidate;
				highest_strength = candidate_type->Bombard_Strength;
			}
		}
		return tr;
	}
}

void __fastcall
patch_Fighter_damage_by_db_in_main_loop (Fighter * this, int edx, Unit * bombarder, Unit * defender)
{
	if (p_bic_data->UnitTypes[bombarder->Body.UnitTypeID].Unit_Class == UTC_Air) {
		if (Unit_try_flying_over_tile (bombarder, __, defender->Body.X, defender->Body.Y))
			return; // intercepted
		else if (patch_Main_Screen_Form_is_unit_visible_to_player (p_main_screen_form, __, defender->Body.X, defender->Body.Y, bombarder))
			Unit_play_bombing_animation (bombarder, __, defender->Body.X, defender->Body.Y);
	}

	// If the unit has already performed DB this turn, then record that it's consumed one of its extra DBs
	if (bombarder->Body.Status & USF_USED_DEFENSIVE_BOMBARD) {
		int extra_dbs;
		int got_value = itable_look_up (&is->extra_defensive_bombards, bombarder->Body.ID, &extra_dbs);
		itable_insert (&is->extra_defensive_bombards, bombarder->Body.ID, got_value ? (extra_dbs + 1) : 1);
	}

	int damage_before = defender->Body.Damage;
	Fighter_damage_by_defensive_bombard (this, __, bombarder, defender);
	int damage_after = defender->Body.Damage;

	is->dbe.bombarder = bombarder;
	is->dbe.defender = defender;
	if (damage_after > damage_before) {
		is->dbe.damage_done = true;
		int max_hp = Unit_get_max_hp (defender);
		int dead_before = damage_before >= max_hp, dead_after = damage_after >= max_hp;

		// If the unit was killed by defensive bombard, play its death animation then toggle off animations for the rest of the combat so it
		// doesn't look like anything else happens. Technically, the combat continues and the dead unit is guarantted to lose because the
		// patch to get_combat_odds ensures the dead unit has no chance of winning a round.
		if (dead_before ^ dead_after) {
			is->dbe.defender_was_destroyed = true;
			if ((! is_online_game ()) && Fighter_check_combat_anim_visibility (this, __, bombarder, defender, true))
				Animator_play_one_shot_unit_animation (&p_main_screen_form->animator, __, defender, AT_DEATH, false);
			is->dbe.saved_animation_setting = this->play_animations;
			this->play_animations = 0;
		}
	}
}

int __fastcall
patch_Fighter_get_odds_for_main_combat_loop (Fighter * this, int edx, Unit * attacker, Unit * defender, bool bombarding, bool ignore_defensive_bonuses)
{
	// If the attacker was destroyed by defensive bombard, return a number that will ensure the defender wins the first round of combat, otherwise
	// the zero HP attacker might go on to win an absurd victory. (The attacker in the overall combat is the defender during DB).
	if (is->dbe.defender_was_destroyed)
		return 1025;

	else
		return Fighter_get_combat_odds (this, __, attacker, defender, bombarding, ignore_defensive_bonuses);
}

byte __fastcall
patch_Fighter_fight (Fighter * this, int edx, Unit * attacker, int attack_direction, Unit * defender_or_null)
{
	byte tr = Fighter_fight (this, __, attacker, attack_direction, defender_or_null);
	is->dbe = (struct defensive_bombard_event) {0};
	return tr;
}

void __fastcall
patch_Unit_score_kill_by_defender (Unit * this, int edx, Unit * victim, bool was_attacking)
{
	// This function is called when the defender wins in combat. If the attacker was actually killed by defensive bombardment, then award credit
	// for that kill to the defensive bombarder not the defender in combat.
	if (is->dbe.defender_was_destroyed) {
		Unit_score_kill (is->dbe.bombarder, __, victim, was_attacking);
		p_bic_data->fighter.play_animations = is->dbe.saved_animation_setting;

	} else
		Unit_score_kill (this, __, victim, was_attacking);
}

void __fastcall
patch_Unit_play_attack_anim_for_def_bombard (Unit * this, int edx, int direction)
{
	// Don't play any animation for air units, the animations are instead handled in the patch for damage_by_defensive_bombard
	if (p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class != UTC_Air)
		Unit_play_attack_animation (this, __, direction);
}

bool
can_precision_strike_tile_improv_at (Unit * unit, int x, int y)
{
	Tile * tile;
	return is->current_config.allow_precision_strikes_against_tile_improvements && // we're configured to allow prec. strikes vs tiles AND
		((tile = tile_at (x, y)) != p_null_tile) && // get tile, make sure it's valid AND
		is_explored (tile, unit->Body.CivID) && // tile has been explored by attacker AND
		has_any_destructible_overlays (tile, true); // it has something that can be destroyed by prec. strike
}

// Same as above function except this one applies to the V3 field instead of FOWStatus
Tile * __cdecl
patch_tile_at_for_v3_check (int x, int y)
{
	Tile * tile = tile_at (x, y);
	int is_hotseat_game = *p_is_offline_mp_game && ! *p_is_pbem_game;
	if (is_hotseat_game && is->current_config.share_visibility_in_hoseat) {
		is->dummy_tile->Body.V3 = ((tile->Body.V3 & *p_human_player_bits) != 0) << p_main_screen_form->Player_CivID;
		return is->dummy_tile;
	} else
		return tile;
}

bool __fastcall
patch_Unit_check_contact_bit_6_on_right_click (Unit * this, int edx, int civ_id)
{
	bool tr = Unit_check_contact_bit_6 (this, __, civ_id);
	if ((! tr) &&
	    is->current_config.share_visibility_in_hoseat &&
	    (*p_is_offline_mp_game && ! *p_is_pbem_game) && // is hotseat game
	    ((1 << civ_id) & *p_human_player_bits)) { // is civ_id a human player
		if ((1 << this->Body.CivID) & *p_human_player_bits)
			tr = true;

		else {
			// Check if any other human player has contact
			unsigned player_bits = *(unsigned *)p_human_player_bits >> 1;
			int n_player = 1;
			while (player_bits != 0) {
				if ((player_bits & 1) && (n_player != civ_id) &&
				    Unit_check_contact_bit_6 (this, __, n_player)) {
					tr = true;
					break;
				}
				player_bits >>= 1;
				n_player++;
			}
		}
	}
	return tr;
}

bool __fastcall
patch_Unit_check_precision_strike_target (Unit * this, int edx, int tile_x, int tile_y)
{
	return Unit_check_precision_strike_target (this, __, tile_x, tile_y) || can_precision_strike_tile_improv_at (this, tile_x, tile_y);
}

void __fastcall
patch_Unit_attack_tile (Unit * this, int edx, int x, int y, int bombarding)
{
	if (bombarding)
		is->unit_bombard_attacking_tile = this;
	Unit_attack_tile (this, __, x, y, bombarding);
	is->unit_bombard_attacking_tile = NULL;
}

void __fastcall
patch_Unit_do_precision_strike (Unit * this, int edx, int x, int y)
{
	if ((city_at (x, y) == NULL) && can_precision_strike_tile_improv_at (this, x, y))
		patch_Unit_attack_tile (this, __, x, y, 1);
	else
		Unit_do_precision_strike (this, __, x, y);
}

int __fastcall
patch_Unit_get_max_moves_after_barricade_attack (Unit * this)
{
	if (is->current_config.dont_end_units_turn_after_bombarding_barricade && (this == is->unit_bombard_attacking_tile))
		return this->Body.Moves + p_bic_data->General.RoadsMovementRate;
	else
		return Unit_get_max_move_points (this);
}

City * __cdecl
patch_city_at_in_find_bombard_defender (int x, int y)
{
	// The caller (Fighter::find_defender_against_bombardment) has a set of lists of bombard priority/eligibility in its stack memory. The list
	// for land units bombarding land tiles normally restricts the targets to land units by containing [UTC_Land, -1, -1]. If we're configured to
	// remove that restriction, modify the list so it contains instead [UTC_Land, UTC_Sea, UTC_Air]. Conveniently, the offset from this function's
	// first parameter to the list is 0x40 bytes in all executables.
	if (is->current_config.remove_land_artillery_target_restrictions) {
		enum UnitTypeClasses * list = (void *)((byte *)&x + 0x40);
		list[1] = UTC_Sea;
		list[2] = UTC_Air;
	}

	return city_at (x, y);
}

bool __fastcall
patch_Unit_check_bombard_target (Unit * this, int edx, int tile_x, int tile_y)
{
	bool base = Unit_check_bombard_target (this, __, tile_x, tile_y);
	Tile * tile;
	int overlays;
	if (base &&
	    is->current_config.disallow_useless_bombard_vs_airfields &&
	    ((tile = tile_at (tile_x, tile_y)) != p_null_tile) &&
	    ((overlays = tile->vtable->m42_Get_Overlays (tile, __, 0)) & 0x20000000) && // if tile has an airfield AND
	    (overlays == 0x20000000)) { // tile only has an airfield
		UnitType * this_type = &p_bic_data->UnitTypes[this->Body.UnitTypeID];

		// Check that a bombard attack vs this tile would not be wasted. It won't be if either (1) there are no units on the tile, (2) there
		// is a unit that can be damaged by bombarding, or (3) there are no units that can be damaged and no air units. The rules for
		// bombardment are that you can't damage aircraft in an airfield and you also can't destroy an airfield from underneath aircraft.
		int any_units = 0,
		    any_vulnerable_units = 0,
		    any_air_units = 0;
		FOR_UNITS_ON (uti, tile) {
			enum UnitTypeClasses class = p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID].Unit_Class;
			any_units = 1;
			any_air_units |= class == UTC_Air;
			any_vulnerable_units |= (class != UTC_Air) && (Unit_get_defense_strength (uti.unit) > 0) &&
				can_damage_bombarding (this_type, uti.unit, tile);
		}
		return (! any_units) || // case (1) above
			any_vulnerable_units || // case (2)
			((! any_air_units) && (! any_vulnerable_units)); // case (3)

	} else
		return base;
}

bool __fastcall
patch_Unit_can_disembark_anything (Unit * this, int edx, int tile_x, int tile_y)
{
	Tile * this_tile = tile_at (this->Body.X, this->Body.Y);
	bool base = Unit_can_disembark_anything (this, __, tile_x, tile_y);

	// Apply trespassing restriction. First check if this civ may move into (tile_x, tile_y) without trespassing. If it would be trespassing, then
	// we can only disembark anything if this transport has a passenger that can ignore the restriction. Without this check, the game can enter an
	// infinite loop under rare circumstances.
	if (base &&
	    is->current_config.disallow_trespassing &&
	    check_trespassing (this->Body.CivID, this_tile, tile_at (tile_x, tile_y))) {
		bool any_exempt_passengers = false;
		FOR_UNITS_ON (uti, this_tile)
			if ((uti.unit->Body.Container_Unit == this->Body.ID) && is_allowed_to_trespass (uti.unit)) {
				any_exempt_passengers = true;
				break;
			}
		return any_exempt_passengers;

	} else
		return base;
}

int __fastcall
patch_Unit_get_defense_for_bombardable_unit_check (Unit * this)
{
	// Returning a defense value of zero indicates this unit is not a target for bombardment. If configured, exclude all air units from
	// bombardment so the attacks target tile improvements instead. Do this only if the tile has another improvement in addition to an airfield,
	// or we could destroy the airfield itself.
	Tile * tile;
	int overlays;
	if (is->current_config.allow_bombard_of_other_improvs_on_occupied_airfield && // If configured AND
	    (p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class == UTC_Air) && // "this" is an air unit AND
	    ((tile = tile_at (this->Body.X, this->Body.Y)) != p_null_tile) && // "this" is on a valid tile AND
	    ((overlays = tile->vtable->m42_Get_Overlays (tile, __, 0)) & 0x20000000) && // tile has an airfield AND
	    (overlays != 0x20000000)) // tile does not only have an airfield
		return 0;

	else
		return Unit_get_defense_strength (this);
}

void __fastcall
patch_Demographics_Form_m22_draw (Demographics_Form * this)
{
	Demographics_Form_m22_draw (this);

	if (is->current_config.show_total_city_count) {
		// There's proably a better way to get the city count, but better safe than sorry. I don't know if it's possible for the city list to
		// contain holes so the surest thing is to check every possible ID.
		int city_count = 0; {
			if (p_cities->Cities != NULL)
				for (int n = 0; n <= p_cities->LastIndex; n++) {
					City_Body * body = p_cities->Cities[n].City;
					city_count += (body != NULL) && ((int)body != offsetof (City, Body));
				}
		}

		PCX_Image * canvas = &this->Base.Data.Canvas;

		// Draw backdrop
		{
			char temp_path[2*MAX_PATH];
			PCX_Image backdrop;
			PCX_Image_construct (&backdrop);
			get_mod_art_path ("CityCountBackdrop.pcx", temp_path, sizeof temp_path);
			PCX_Image_read_file (&backdrop, __, temp_path, NULL, 0, 0x100, 2);
			if (backdrop.JGL.Image != NULL) {
				int w = backdrop.JGL.Image->vtable->m54_Get_Width (backdrop.JGL.Image);
				PCX_Image_draw_onto (&backdrop, __, canvas, (1024 - w) / 2, 720);
			}
			backdrop.vtable->destruct (&backdrop, __, 0);
		}

		// Draw text on top of the backdrop
		char s[100];
		snprintf (s, sizeof s, "%s %d / 512", is->c3x_labels[CL_TOTAL_CITIES], city_count);
		s[(sizeof s) - 1] = '\0';
		PCX_Image_set_text_effects (canvas, __, 0x80000000, -1, 2, 2); // Set text color to black
		PCX_Image_draw_centered_text (canvas, __, get_font (14, FSF_NONE), s, 1024/2 - 100, 730, 200, strlen (s));
	}
}

int __fastcall
patch_Leader_get_optimal_city_number (Leader * this)
{
	if (! is->current_config.strengthen_forbidden_palace_ocn_effect)
		return Leader_get_optimal_city_number (this);
	else {
		int num_sans_fp = Leader_get_optimal_city_number (this), // OCN w/o contrib from num of FPs
		    fp_count = Leader_count_wonders_with_small_flag (this, __, ITSW_Reduces_Corruption, NULL),
		    s_diff = p_bic_data->DifficultyLevels[this->field_30].Optimal_Cities, // Difficulty scaling, called "percentage of optimal cities" in the editor
		    base_ocn = p_bic_data->WorldSizes[p_bic_data->Map.World.World_Size].OptimalCityCount;
		return num_sans_fp + (s_diff * fp_count * base_ocn + 50) / 100; // Add 50 to round off
	}
}

int __fastcall
patch_Leader_count_forbidden_palaces_for_ocn (Leader * this, int edx, enum ImprovementTypeSmallWonderFeatures flag, City * city_or_null)
{
	if (! is->current_config.strengthen_forbidden_palace_ocn_effect)
		return Leader_count_wonders_with_small_flag (this, __, flag, city_or_null);
	else
		return 0; // We'll add in the FP effect later with a different weight
}

void __fastcall
patch_Trade_Net_recompute_city_connections (Trade_Net * this, int edx, int civ_id, bool redo_road_network, byte param_3, int redo_roads_for_city_id)
{
	is->is_computing_city_connections = true;
	long long ts_before;
	QueryPerformanceCounter ((LARGE_INTEGER *)&ts_before);

	if (is->tnx_init_state == IS_OK) {
		if (is->tnx_cache == NULL) {
			is->tnx_cache = is->create_tnx_cache (&p_bic_data->Map);
			is->set_up_before_building_network (is->tnx_cache);
		} else if (! is->keep_tnx_cache)
			is->set_up_before_building_network (is->tnx_cache);
	}

	Trade_Net_recompute_city_connections (this, __, civ_id, redo_road_network, param_3, redo_roads_for_city_id);

	long long ts_after;
	QueryPerformanceCounter ((LARGE_INTEGER *)&ts_after);
	is->time_spent_computing_city_connections += ts_after - ts_before;
	is->count_calls_to_recompute_city_connections++;
	is->is_computing_city_connections = false;
}

void __fastcall
patch_Map_build_trade_network (Map * this)
{
	if ((is->tnx_init_state == IS_OK) && (is->tnx_cache != NULL))
		is->set_up_before_building_network (is->tnx_cache);
	is->keep_tnx_cache = true;
	Map_build_trade_network (this);
	is->keep_tnx_cache = false;
}

void __fastcall
patch_Trade_Net_recompute_city_cons_and_res (Trade_Net * this, int edx, bool param_1)
{
	if ((is->tnx_init_state == IS_OK) && (is->tnx_cache != NULL))
		is->set_up_before_building_network (is->tnx_cache);
	is->keep_tnx_cache = true;
	Trade_Net_recompute_city_cons_and_res (this, __, param_1);
	is->keep_tnx_cache = false;
}

int __fastcall
patch_Trade_Net_set_unit_path_to_fill_road_net (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, int flags, int * out_path_length_in_mp)
{
	int tr;
	long long ts_before;
	QueryPerformanceCounter ((LARGE_INTEGER *)&ts_before);

	if ((is->tnx_init_state == IS_OK) && (is->tnx_cache != NULL)) {
		is->flood_fill_road_network (is->tnx_cache, from_x, from_y, civ_id);
		tr = 0; // Return value is not used by caller anyway
	} else
		tr = Trade_Net_set_unit_path (this, __, from_x, from_y, to_x, to_y, unit, civ_id, flags, out_path_length_in_mp);

	long long ts_after;
	QueryPerformanceCounter ((LARGE_INTEGER *)&ts_after);
	is->time_spent_filling_roads += ts_after - ts_before;
	return tr;
}

bool
set_up_gdi_plus ()
{
	if (is->gdi_plus.init_state == IS_UNINITED) {
		is->gdi_plus.init_state = IS_INIT_FAILED;
		is->gdi_plus.gp_graphics = NULL;

		struct startup_input {
			UINT32 GdiplusVersion;
			void * DebugEventCallback;
			BOOL SuppressBackgroundThread;
			BOOL SuppressExternalCodecs;
		} startup_input = {1, NULL, FALSE, FALSE};

		is->gdi_plus.module = LoadLibraryA ("gdiplus.dll");
		if (is->gdi_plus.module == NULL) {
			MessageBoxA (NULL, "Failed to load gdiplus.dll!", "Error", MB_ICONERROR);
			goto end_init;
		}

		int (WINAPI * GdiplusStartup) (ULONG_PTR * out_token, struct startup_input *, void * startup_output) =
			(void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdiplusStartup");

		is->gdi_plus.CreateFromHDC    = (void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdipCreateFromHDC");
		is->gdi_plus.DeleteGraphics   = (void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdipDeleteGraphics");
		is->gdi_plus.SetSmoothingMode = (void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdipSetSmoothingMode");
		is->gdi_plus.SetPenDashStyle  = (void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdipSetPenDashStyle");
		is->gdi_plus.CreatePen1       = (void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdipCreatePen1");
		is->gdi_plus.DeletePen        = (void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdipDeletePen");
		is->gdi_plus.DrawLineI        = (void *)(*p_GetProcAddress) (is->gdi_plus.module, "GdipDrawLineI");
		if ((is->gdi_plus.CreateFromHDC == NULL) || (is->gdi_plus.DeleteGraphics == NULL) ||
		    (is->gdi_plus.SetSmoothingMode == NULL) || (is->gdi_plus.SetPenDashStyle == NULL) ||
		    (is->gdi_plus.CreatePen1 == NULL) || (is->gdi_plus.DeletePen == NULL) ||
		    (is->gdi_plus.DrawLineI == NULL)) {
			MessageBoxA (NULL, "Failed to get GDI+ proc addresses!", "Error", MB_ICONERROR);
			goto end_init;
		}

		int status = GdiplusStartup (&is->gdi_plus.token, &startup_input, NULL);
		if (status != 0) {
			char s[200];
			snprintf (s, sizeof s, "Failed to initialize GDI+! Startup status: %d", status);
			MessageBoxA (NULL, s, "Error", MB_ICONERROR);
			goto end_init;
		}

		is->gdi_plus.init_state = IS_OK;
	end_init:
		;
	}

	return is->gdi_plus.init_state == IS_OK;
}

int __fastcall
patch_OpenGLRenderer_initialize (OpenGLRenderer * this, int edx, PCX_Image * texture)
{
	if ((is->current_config.draw_lines_using_gdi_plus == LDO_NEVER) ||
	    ((is->current_config.draw_lines_using_gdi_plus == LDO_WINE) && ! is->running_on_wine))
		return OpenGLRenderer_initialize (this, __, texture);

	// Initialize GDI+ instead
	else {
		if (! set_up_gdi_plus ())
			return 2;
		if (is->gdi_plus.gp_graphics != NULL) {
			is->gdi_plus.DeleteGraphics (is->gdi_plus.gp_graphics);
			is->gdi_plus.gp_graphics = NULL;
		}
		int status = is->gdi_plus.CreateFromHDC (texture->JGL.Image->DC, &is->gdi_plus.gp_graphics);
		if (status == 0) {
			is->gdi_plus.SetSmoothingMode (is->gdi_plus.gp_graphics, 4); // 4 = SmoothingModeAntiAlias from GdiPlusEnums.h
			return 0;
		} else
			return 2;
	}
}

void __fastcall
patch_OpenGLRenderer_set_color (OpenGLRenderer * this, int edx, unsigned int rgb555)
{
	// Convert rgb555 to rgb888
	unsigned int rgb888 = 0; {
		unsigned int mask = 31;
		int shift = 3;
		for (int n = 0; n < 3; n++) {
			rgb888 |= (rgb555 & mask) << shift;
			mask <<= 5;
			shift += 3;
		}
	}

	is->ogl_color = (is->ogl_color & 0xFF000000) | rgb888;
	OpenGLRenderer_set_color (this, __, rgb555);
}

void __fastcall
patch_OpenGLRenderer_set_opacity (OpenGLRenderer * this, int edx, unsigned int alpha)
{
	is->ogl_color = (is->ogl_color & 0x00FFFFFF) | (alpha << 24);
	OpenGLRenderer_set_opacity (this, __, alpha);
}

void __fastcall
patch_OpenGLRenderer_set_line_width (OpenGLRenderer * this, int edx, int width)
{
	is->ogl_line_width = width;
	OpenGLRenderer_set_line_width (this, __, width);
}

void __fastcall
patch_OpenGLRenderer_enable_line_dashing (OpenGLRenderer * this)
{
	is->ogl_line_stipple_enabled = true;
	OpenGLRenderer_enable_line_dashing (this);
}

void __fastcall
patch_OpenGLRenderer_disable_line_dashing (OpenGLRenderer * this)
{
	is->ogl_line_stipple_enabled = false;
	OpenGLRenderer_disable_line_dashing (this);
}

void __fastcall
patch_OpenGLRenderer_draw_line (OpenGLRenderer * this, int edx, int x1, int y1, int x2, int y2)
{
	if ((is->current_config.draw_lines_using_gdi_plus == LDO_NEVER) ||
	    ((is->current_config.draw_lines_using_gdi_plus == LDO_WINE) && ! is->running_on_wine))
		OpenGLRenderer_draw_line (this, __, x1, y1, x2, y2);

	else if ((is->gdi_plus.init_state == IS_OK) && (is->gdi_plus.gp_graphics != NULL)) {
		void * gp_pen;
		int unit_world = 0; // = UnitWorld from gdiplusenums.h
		int status = is->gdi_plus.CreatePen1 (is->ogl_color, (float)is->ogl_line_width, unit_world, &gp_pen);
		if (status == 0) {
			if (is->ogl_line_stipple_enabled)
				is->gdi_plus.SetPenDashStyle (gp_pen, 1); // 1 = DashStyleDash from GdiPlusEnums.h
			is->gdi_plus.DrawLineI (is->gdi_plus.gp_graphics, gp_pen, x1, y1, x2, y2);
			is->gdi_plus.DeletePen (gp_pen);
		}
	}
}

int __fastcall
patch_Tile_check_water_for_retreat_on_defense (Tile * this)
{
	int is_water = this->vtable->m35_Check_Is_Water (this);
	if (is->current_config.allow_defensive_retreat_on_water &&
	    is_water &&
	    (p_bic_data->fighter.defender != NULL) &&
	    (p_bic_data->UnitTypes[p_bic_data->fighter.defender->Body.UnitTypeID].Unit_Class == UTC_Sea)) {
		return 0; // Say this is not water so the retreat is allowed to happen
	} else
		return is_water;
}

int __fastcall
patch_City_count_airports_for_airdrop (City * this, int edx, enum ImprovementTypeFlags airport_flag)
{
	if (is->current_config.allow_airdrop_without_airport)
		return 1;
	else
		return City_count_improvements_with_flag (this, __, airport_flag);
}

int __fastcall
patch_Leader_get_cont_city_count_for_worker_req (Leader * this, int edx, int cont_id)
{
	int tr = Leader_get_city_count_on_continent (this, __, cont_id);
	if (is->current_config.ai_worker_requirement_percent != 100)
		tr = (tr * is->current_config.ai_worker_requirement_percent + 50) / 100;
	return tr;
}

int __fastcall
patch_Leader_get_city_count_for_worker_prod_cap (Leader * this)
{
	int tr = this->Cities_Count;
	// Don't scale down the cap since it's pretty low to begin with
	if (is->current_config.ai_worker_requirement_percent > 100)
		tr = (tr * is->current_config.ai_worker_requirement_percent + 50) / 100;
	return tr;
}

// If "only_improv_id" is >= 0, will only return yields from mills attached to that improv, otherwise returns all yields from all improvs
void
gather_mill_yields (City * city, int only_improv_id, int * out_food, int * out_shields, int * out_commerce)
{
	int food = 0, shields = 0, commerce = 0;
	for (int n = 0; n < is->current_config.count_mills; n++) {
		struct mill * mill = &is->current_config.mills[n];
		if ((mill->flags & MF_YIELDS) &&
		    ((only_improv_id < 0) || (mill->improv_id == only_improv_id)) &&
		    can_generate_resource (city->Body.CivID, mill) &&
		    has_active_building (city, mill->improv_id) &&
		    has_resources_required_by_building (city, mill->improv_id)) {
			Resource_Type * res = &p_bic_data->ResourceTypes[mill->resource_id];
			food += res->Food;
			shields += res->Shield;
			commerce += res->Commerce;
		}
	}
	*out_food = food;
	*out_shields = shields;
	*out_commerce = commerce;
}


int __fastcall
patch_City_calc_tile_yield_while_gathering (City * this, int edx, YieldKind kind, int tile_x, int tile_y)
{
	int tr = City_calc_tile_yield_at (this, __, kind, tile_x, tile_y);

	// Include yields from generated resources
	if ((this->Body.X == tile_x) && (this->Body.Y == tile_y)) {
		int mill_food, mill_shields, mill_commerce;
		gather_mill_yields (this, -1, &mill_food, &mill_shields, &mill_commerce);
		if      (kind == YK_FOOD)     tr += mill_food;
		else if (kind == YK_SHIELDS)  tr += mill_shields;
		else if (kind == YK_COMMERCE) tr += mill_commerce;
	}

	return tr;
}

bool __fastcall
patch_Leader_record_export (Leader * this, int edx, int importer_civ_id, int resource_id)
{
	bool exported = Leader_record_export (this, __, importer_civ_id, resource_id);
	if (exported &&
	    (! is->is_computing_resource_access) && // if we're not already running recompute_resources
	    (is->mill_input_resource_bits[resource_id>>3] & (1 << (resource_id & 7)))) // if the traded resource is an input to any mill
		patch_Trade_Net_recompute_resources (p_trade_net, __, false);
	return exported;
}

bool __fastcall
patch_Leader_erase_export (Leader * this, int edx, int importer_civ_id, int resource_id)
{
	bool erased = Leader_erase_export (this, __, importer_civ_id, resource_id);
	if (erased &&
	    (! is->is_computing_resource_access) && // if we're not already running recompute_resources
	    (is->mill_input_resource_bits[resource_id>>3] & (1 << (resource_id & 7)))) // if the traded resource is an input to any mill
		patch_Trade_Net_recompute_resources (p_trade_net, __, false);
	return erased;
}

int __fastcall
patch_Tile_Image_Info_draw_improv_img_on_city_form (Tile_Image_Info * this, int edx, PCX_Image * canvas, int pixel_x, int pixel_y, int param_4)
{
	int tr = Tile_Image_Info_draw (this, __, canvas, pixel_x, pixel_y, param_4);

	int generated_resources[16];
	int generated_resource_count = 0;
	for (int n = 0; (n < is->current_config.count_mills) && (generated_resource_count < ARRAY_LEN (generated_resources)); n++) {
		struct mill * mill = &is->current_config.mills[n];
		if ((mill->improv_id == is->drawing_icons_for_improv_id) &&
		    ((mill->flags & MF_SHOW_BONUS) || (p_bic_data->ResourceTypes[mill->resource_id].Class != RC_Bonus)) &&
		    (! ((mill->flags & MF_HIDE_NON_BONUS) && (p_bic_data->ResourceTypes[mill->resource_id].Class != RC_Bonus))) &&
		    can_generate_resource (p_city_form->CurrentCity->Body.CivID, mill) &&
		    has_active_building (p_city_form->CurrentCity, mill->improv_id) &&
		    has_resources_required_by_building (p_city_form->CurrentCity, mill->improv_id))
			generated_resources[generated_resource_count++] = mill->resource_id;
	}

	if ((generated_resource_count > 0) && (is->resources_sheet != NULL) && (is->resources_sheet->JGL.Image != NULL) && (TransparentBlt != NULL)) {
		JGL_Image * jgl_canvas = canvas->JGL.Image,
			  * jgl_sheet = is->resources_sheet->JGL.Image;

		HDC canvas_dc = jgl_canvas->vtable->acquire_dc (jgl_canvas);
		if (canvas_dc != NULL) {
			HDC sheet_dc = jgl_sheet->vtable->acquire_dc (jgl_sheet);
			if (sheet_dc != NULL) {

				for (int n = 0; n < generated_resource_count; n++) {
					int icon_id = p_bic_data->ResourceTypes[generated_resources[n]].IconID,
					    sheet_row = icon_id / 6,
					    sheet_col = icon_id % 6;

					int dy = (n * 160 / not_below (1, generated_resource_count - 1) + 5) / 10;
					TransparentBlt (canvas_dc, // dest DC
							pixel_x + 15, pixel_y + dy, 24, 24, // dest x, y, width, height
							sheet_dc, // src DC
							9 + 50*sheet_col, 9 + 50*sheet_row, 33, 33, // src x, y, width, height
							0xFF00FF); // transparent color (RGB)
				}

				jgl_sheet->vtable->release_dc (jgl_sheet, __, 1);
			}
			jgl_canvas->vtable->release_dc (jgl_canvas, __, 1);
		}
	}
	return tr;
}

void __cdecl
patch_draw_improv_icons_on_city_screen (Base_List_Control * control, int improv_id, int item_index, int offset_x, int offset_y)
{
	is->drawing_icons_for_improv_id = improv_id;
	draw_improv_icons_on_city_screen (control, improv_id, item_index, offset_x, offset_y);
	is->drawing_icons_for_improv_id = -1;
}

int __fastcall
patch_City_get_tourism_amount_to_draw (City * this, int edx, int improv_id)
{
	is->tourism_icon_counter = 0;

	int mill_food, mill_shields, mill_commerce;
	gather_mill_yields (this, improv_id, &mill_food, &mill_shields, &mill_commerce);

	// TODO: Properly display negative mill yields. Right now negative values will mess with the arithmetic so I'm clamping the displayed yields
	// at zero to prevent that.
	mill_food     = not_below (0, mill_food);
	mill_shields  = not_below (0, mill_shields);
	mill_commerce = not_below (0, mill_commerce);

	is->convert_displayed_tourism_to_food = mill_food;
	is->convert_displayed_tourism_to_shields = mill_shields;
	return City_get_tourism_amount (this, __, improv_id) + mill_food + mill_shields + mill_commerce;
}

int __fastcall
patch_Tile_Image_Info_draw_tourism_gold (Tile_Image_Info * this, int edx, PCX_Image * canvas, int pixel_x, int pixel_y, int param_4)
{
	// Replace the yield sprite we're drawing with food or a shield if needed.
	Tile_Image_Info * sprite; {
		if (is->tourism_icon_counter < is->convert_displayed_tourism_to_food)
			sprite = &p_city_form->City_Icons_Images.Icon_15_Food;
		else if (is->tourism_icon_counter < is->convert_displayed_tourism_to_food + is->convert_displayed_tourism_to_shields)
			sprite = &p_city_form->City_Icons_Images.Icon_13_Shield;
		else
			sprite = this;
	}

	int tr = Tile_Image_Info_draw (sprite, __, canvas, pixel_x, pixel_y, param_4);
	is->tourism_icon_counter++;
	return tr;
}

Unit * __fastcall
patch_Leader_spawn_unit (Leader * this, int edx, int type_id, int tile_x, int tile_y, int barb_tribe_id, int id, bool param_6, LeaderKind leader_kind, int race_id)
{
	Unit * tr = Leader_spawn_unit (this, __, type_id, tile_x, tile_y, barb_tribe_id, id, param_6, leader_kind, race_id);
	if (tr != NULL)
		change_unit_type_count (this, type_id, 1);
	return tr;
}

Unit * __fastcall
patch_Leader_spawn_captured_unit (Leader * this, int edx, int type_id, int tile_x, int tile_y, int barb_tribe_id, int id, bool param_6, LeaderKind leader_kind, int race_id)
{
	Unit * tr = patch_Leader_spawn_unit (this, __, type_id, tile_x, tile_y, barb_tribe_id, id, param_6, leader_kind, race_id);
	if ((tr != NULL) && is->moving_unit_to_adjacent_tile)
		is->temporarily_disallow_lethal_zoc = true;
	return tr;
}

void __fastcall
patch_Leader_enter_new_era (Leader * this, int edx, bool param_1, bool no_online_sync)
{
	Leader_enter_new_era (this, __, param_1, no_online_sync);
	apply_era_specific_names (this);
}

char * __fastcall
patch_Leader_get_player_title_for_intro_popup (Leader * this)
{
	// Normally the era-specific names are applied later, after game loading is finished, but in this case we apply the player's names ahead of
	// time so they appear on the intro popup. "this" will always refer to the human player in this call.
	apply_era_specific_names (this);
	return Leader_get_title (this);
}

void __fastcall
patch_City_spawn_unit_if_done (City * this)
{
	bool skip_spawn = false;

	// Apply unit limit. If this city's owner has reached the limit for the unit type it's building then force it to build something else.
	int available;
	if ((this->Body.Order_Type == COT_Unit) &&
	    get_available_unit_count (&leaders[this->Body.CivID], this->Body.Order_ID, &available) &&
	    (available <= 0)) {
		int limited_unit_type_id = this->Body.Order_ID;

		if (*p_human_player_bits & 1<<this->Body.CivID) {
			// Find another type ID to build instead of the limited one
			int replacement_type_id = -1; {
				int limited_unit_strat = p_bic_data->UnitTypes[limited_unit_type_id].AI_Strategy,
				    shields_in_box = this->Body.StoredProduction;
				UnitType * replacement_type;
				for (int can_type_id = 0; can_type_id < p_bic_data->UnitTypeCount; can_type_id++)
					if (patch_City_can_build_unit (this, __, can_type_id, 1, 0, 0)) {
						UnitType * candidate_type = &p_bic_data->UnitTypes[can_type_id];

						// If we haven't found a replacement yet, use this one
						if (replacement_type_id < 0) {
							replacement_type_id = can_type_id;
							replacement_type = candidate_type;

						// Keep the prev replacement if it doesn't waste shields but this candidate would
						} else if ((replacement_type->Cost >= shields_in_box) && (candidate_type->Cost < shields_in_box))
							continue;

						// Keep the prev if it shares an AI strategy with the limited unit but this candidate doesn't
						else if (((replacement_type->AI_Strategy & limited_unit_strat) != 0) &&
							 ((candidate_type  ->AI_Strategy & limited_unit_strat) == 0))
							continue;

						// At this point we know switching to the candidate would not cause us to waste shields and would not
						// give us a worse match role-wise to the original limited unit. So pick it if it's better somehow,
						// either a better role match or more expensive.
						else if ((candidate_type->Cost > replacement_type->Cost) ||
							 (((replacement_type->AI_Strategy & limited_unit_strat) == 0) &&
							  ((candidate_type  ->AI_Strategy & limited_unit_strat) != 0))) {
							replacement_type_id = can_type_id;
							replacement_type = candidate_type;
						}
					}
			}

			if (replacement_type_id >= 0) {
				City_set_production (this, __, COT_Unit, replacement_type_id, false);
				if (this->Body.CivID == p_main_screen_form->Player_CivID) {
					PopupForm * popup = get_popup_form ();
					set_popup_str_param (0, this->Body.CityName, -1, -1);
					set_popup_str_param (1, p_bic_data->UnitTypes[limited_unit_type_id].Name, -1, -1);
					int limit = -1;
					get_unit_limit (&leaders[this->Body.CivID], limited_unit_type_id, &limit);
					set_popup_int_param (2, limit);
					set_popup_str_param (3, p_bic_data->UnitTypes[replacement_type_id].Name, -1, -1);
					popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_LIMITED_UNIT_CHANGE", -1, 0, 0, 0);
					int response = patch_show_popup (popup, __, 0, 0);
					if (response == 0)
						City_zoom_to (this, __, 0);
				}
			}

		} else {
			City_Order unused;
			patch_City_ai_choose_production (this, __, &unused);
		}

		// Just as a final check, if we weren't able to switch production off the limited unit, prevent it from being spawned so the limit
		// doesn't get violated.
		if ((this->Body.Order_Type == COT_Unit) && (this->Body.Order_ID == limited_unit_type_id))
			skip_spawn = true;
	}

	if (! skip_spawn)
		City_spawn_unit_if_done (this);
}

void __fastcall
patch_Leader_upgrade_all_units (Leader * this, int edx, int type_id)
{
	is->penciled_in_upgrade_count = 0;
	Leader_upgrade_all_units (this, __, type_id);
}

void __fastcall
patch_Main_Screen_Form_upgrade_all_units (Main_Screen_Form * this, int edx, int type_id)
{
	is->penciled_in_upgrade_count = 0;
	Main_Screen_Form_upgrade_all_units (this, __, type_id);
}

bool __fastcall
patch_Unit_can_perform_upgrade_all (Unit * this, int edx, int unit_command_value)
{
	bool base = patch_Unit_can_perform_command (this, __, unit_command_value);

	// Deal with unit limits. If the unit type we're upgrading to is limited, we need to pencil in the upgrade to make sure that we don't queue up
	// so many upgrades that we exceed the limit.
	City * city;
	int upgrade_id, available;
	if (base &&
	    (is->current_config.unit_limits.len > 0) &&
	    (NULL != (city = city_at (this->Body.X, this->Body.Y))) &&
	    (0 <= (upgrade_id = City_get_upgraded_type_id (city, __, this->Body.UnitTypeID))) &&
	    get_available_unit_count (&leaders[this->Body.CivID], upgrade_id, &available)) {

		// Find penciled in upgrade. Add a new one if we don't already have one.
		struct penciled_in_upgrade * piu = NULL; {
			for (int n = 0; n < is->penciled_in_upgrade_count; n++)
				if (is->penciled_in_upgrades[n].unit_type_id == upgrade_id) {
					piu = &is->penciled_in_upgrades[n];
					break;
				}
			if (piu == NULL) {
				reserve (sizeof is->penciled_in_upgrades[0], (void **)&is->penciled_in_upgrades, &is->penciled_in_upgrade_capacity, is->penciled_in_upgrade_count);
				piu = &is->penciled_in_upgrades[is->penciled_in_upgrade_count];
				is->penciled_in_upgrade_count += 1;
				piu->unit_type_id = upgrade_id;
				piu->count = 0;
			}
		}

		// If we can have more units of the type we're upgrading to, pencil in another upgrade and return true. Otherwise return false so this
		// unit isn't considered one of the upgradable ones.
		if (piu->count < available) {
			piu->count += 1;
			return true;
		} else
			return false;

	} else
		return base;
}

void __fastcall
patch_Fighter_animate_start_of_combat (Fighter * this, int edx, Unit * attacker, Unit * defender)
{
	// Temporarily clear the attacker retreat eligibility flag when needed so naval units are rotated and animated properly. Normally the game
	// does not use ranged animations when the attacker can retreat and, worse, not using ranged anims means it also ignores the
	// rotate-before-attack setting.
	bool restore_attacker_retreat_eligibility = false;
	if ((is->current_config.sea_retreat_rules != RR_STANDARD) &&
	    (p_bic_data->UnitTypes[attacker->Body.UnitTypeID].Unit_Class == UTC_Sea) &&
	    Unit_has_ability (attacker, __, UTA_Ranged_Attack_Animation) &&
	    Unit_has_ability (defender, __, UTA_Ranged_Attack_Animation) &&
	    this->attacker_eligible_to_retreat) {
		this->attacker_eligible_to_retreat = false;
		restore_attacker_retreat_eligibility = true;
	}

	Fighter_animate_start_of_combat (this, __, attacker, defender);

	if (restore_attacker_retreat_eligibility)
		this->attacker_eligible_to_retreat = true;
}

Unit * __fastcall
patch_Leader_spawn_unit_from_building (Leader * this, int edx, int type_id, int tile_x, int tile_y, int barb_tribe_id, int id, bool param_6, LeaderKind leader_kind, int race_id)
{
	int available;
	if (get_available_unit_count (this, type_id, &available) && (available <= 0))
		return NULL;
	else
		return patch_Leader_spawn_unit (this, __, type_id, tile_x, tile_y, barb_tribe_id, id, param_6, leader_kind, race_id);
}

int __fastcall
patch_City_count_improvs_enabling_upgrade (City * this, int edx, enum ImprovementTypeFlags flag)
{
	return is->current_config.allow_upgrades_in_any_city ? 1 : City_count_improvements_with_flag (this, __, flag);
}

City * __fastcall
patch_Leader_create_city_from_hut (Leader * this, int edx, int x, int y, int race_id, int param_4, char const * name, bool param_6)
{
	City * tr = Leader_create_city (this, __, x, y, race_id, param_4, name, param_6);
	if (tr != NULL)
		on_gain_city (this, tr, CGR_POPPED_FROM_HUT);
	return tr;
}

City * __fastcall
patch_Leader_create_city_for_ai_respawn (Leader * this, int edx, int x, int y, int race_id, int param_4, char const * name, bool param_6)
{
	City * tr = Leader_create_city (this, __, x, y, race_id, param_4, name, param_6);
	if (tr != NULL)
		on_gain_city (this, tr, CGR_PLACED_FOR_AI_RESPAWN);
	return tr;
}

City * __fastcall
patch_Leader_create_city_for_founding (Leader * this, int edx, int x, int y, int race_id, int param_4, char const * name, bool param_6)
{
	City * tr = Leader_create_city (this, __, x, y, race_id, param_4, name, param_6);
	if (tr != NULL)
		on_gain_city (this, tr, CGR_FOUNDED);
	return tr;
}

City * __fastcall
patch_Leader_create_city_for_scenario (Leader * this, int edx, int x, int y, int race_id, int param_4, char const * name, bool param_6)
{
	City * tr = Leader_create_city (this, __, x, y, race_id, param_4, name, param_6);
	if (tr != NULL)
		on_gain_city (this, tr, CGR_PLACED_FOR_SCENARIO);
	return tr;
}



bool __fastcall
patch_Leader_do_capture_city (Leader * this, int edx, City * city, bool involuntary, bool converted)
{
	is->currently_capturing_city = city;
	on_lose_city (&leaders[city->Body.CivID], city, converted ? CLR_CONVERTED : (involuntary ? CLR_CONQUERED : CLR_TRADED));
	bool tr = Leader_do_capture_city (this, __, city, involuntary, converted);
	on_gain_city (this, city, converted ? CGR_CONVERTED : (involuntary ? CGR_CONQUERED : CGR_TRADED));
	is->currently_capturing_city = NULL;
	return tr;
}

void __fastcall
patch_City_raze (City * this, int edx, int civ_id_responsible, bool checking_elimination)
{
	on_lose_city (&leaders[this->Body.CivID], this, CLR_DESTROYED);
	City_raze (this, __, civ_id_responsible, checking_elimination);
}

void __fastcall
patch_City_draw_hud_icon (City * this, int edx, PCX_Image * canvas, int pixel_x, int pixel_y)
{
	Leader * owner = &leaders[this->Body.CivID];
	int restore_capital = owner->CapitalID;

	// Temporarily set this city as the capital if it has an extra palace so it gets the capital star icon
	if ((is->current_config.ai_multi_city_start > 1) &&
	    ((*p_human_player_bits & (1 << owner->ID)) == 0) &&
	    has_extra_palace (this))
		owner->CapitalID = this->Body.ID;

	City_draw_hud_icon (this, __, canvas, pixel_x, pixel_y);
	owner->CapitalID = restore_capital;
}

bool __fastcall
patch_City_has_hud_icon (City * this)
{
	return City_has_hud_icon (this)
	       || (   (is->current_config.ai_multi_city_start > 1)
		   && ((*p_human_player_bits & (1 << this->Body.CivID)) == 0)
		   && has_extra_palace (this));
}

void __fastcall
patch_City_draw_on_map (City * this, int edx, Map_Renderer * map_renderer, int pixel_x, int pixel_y, int param_4, int param_5, int param_6, int param_7)
{
	Leader * owner = &leaders[this->Body.CivID];
	int restore_capital = owner->CapitalID;

	// Temporarily set this city as the capital if it has an extra palace so it gets drawn with the next larger size
	if ((is->current_config.ai_multi_city_start > 1) &&
	    ((*p_human_player_bits & (1 << owner->ID)) == 0) &&
	    has_extra_palace (this))
		owner->CapitalID = this->Body.ID;

	City_draw_on_map (this, __, map_renderer, pixel_x, pixel_y, param_4, param_5, param_6, param_7);
	owner->CapitalID = restore_capital;
}

// Writes a string to replace "Wake" or "Activate" on the right-click menu entry for the given unit. Some units might not have a replacement, in which
// case the function writes nothing and returns false. The string is written to out_str which must point to a buffer of at least str_capacity bytes.
bool
get_menu_verb_for_unit (Unit * unit, char * out_str, int str_capacity)
{
	enum c3x_label const labels[33] = {
		CL_IDLE        , // [No state] = 0x0
		CL_FORTIFIED   , // Fortifying = 0x1
		CL_WORKING     , // Build_Mines = 0x2
		CL_WORKING     , // Irrigate = 0x3
		CL_WORKING     , // Build_Fortress = 0x4
		CL_WORKING     , // Build_Road = 0x5
		CL_WORKING     , // Build_Railroad = 0x6
		CL_WORKING     , // Plant_Forest = 0x7
		CL_WORKING     , // Clear_Forest = 0x8
		CL_WORKING     , // Clear_Wetlands = 0x9
		CL_WORKING     , // Clear_Damage = 0xA
		CL_WORKING     , // Build_Airfield = 0xB
		CL_WORKING     , // Build_Radar_Tower = 0xC
		CL_WORKING     , // Build_Outpost = 0xD
		CL_WORKING     , // Build_Barricade = 0xE
		CL_INTERCEPTING, // Intercept = 0xF
		CL_MOVING      , // Go_To = 0x10
		CL_WORKING     , // Road_To_Tile = 0x11
		CL_WORKING     , // Railroad_To_Tile = 0x12
		CL_WORKING     , // Build_Colony = 0x13
		CL_AUTOMATED   , // Auto_Irrigate = 0x14
		CL_AUTOMATED   , // Build_Trade_Routes = 0x15
		CL_AUTOMATED   , // Auto_Clear_Forest = 0x16
		CL_AUTOMATED   , // Auto_Clear_Swamp = 0x17
		CL_AUTOMATED   , // Auto_Clear_Pollution = 0x18
		CL_AUTOMATED   , // Auto_Save_City_Tiles = 0x19
		CL_EXPLORING   , // Explore = 0x1A
		-1             , // ? = 0x1B
		-1             , // Fleeing = 0x1C
		-1             , // ? = 0x1D
		-1             , // ? = 0x1E
		CL_BOMBARDING  , // Auto_Bombard = 0x1F
		CL_BOMBARDING  , // Auto_Air_Bombard = 0x20
	};
	enum UnitStateType state = unit->Body.UnitState;
	enum c3x_label label;
	if ((state >= 0) && (state < ARRAY_LEN (labels)) && (0 <= (label = labels[state]))) {
		if (((label == CL_IDLE) || (label == CL_WORKING) || (label == CL_MOVING)) && unit->Body.automated)
			label = CL_AUTOMATED;
		else if ((label == CL_FORTIFIED) && (unit->Body.Status & (USF_SENTRY | USF_SENTRY_ENEMY_ONLY)))
			label = CL_SENTRY;

		strncpy (out_str, is->c3x_labels[label], str_capacity);
		out_str[str_capacity - 1] = '\0';
		return true;
	} else
		return false;
}

void __fastcall
patch_MenuUnitItem_write_text_to_temp_str (MenuUnitItem * this)
{
	MenuUnitItem_write_text_to_temp_str (this);

	Unit * unit = this->unit;
	char repl_verb[30];
	if ((unit->Body.CivID == p_main_screen_form->Player_CivID) &&
	    (Unit_get_containing_army (unit) == NULL) &&
	    get_menu_verb_for_unit (unit, repl_verb, sizeof repl_verb)) {
		char * verb = (unit->Body.UnitState == UnitState_Fortifying) ? (*p_labels)[0x100] : (*p_labels)[0xFF];
		char * verb_str_start = strstr (temp_str, verb);
		if (verb_str_start != NULL) {
			char s[500];
			char * verb_str_end = verb_str_start + strlen (verb);
			snprintf (s, sizeof s, "%.*s%s%s", verb_str_start - temp_str, temp_str, repl_verb, verb_str_end);
			s[(sizeof s) - 1] = '\0';
			strncpy (temp_str, s, sizeof s);
		}
	}
}

void __fastcall
patch_Tile_m74_Set_Square_Type_for_hill_gen (Tile * this, int edx, enum SquareTypes sq, int tile_x, int tile_y)
{
	if ((sq == SQ_Volcano) && is->current_config.do_not_generate_volcanos)
		sq = SQ_Mountains;
	this->vtable->m74_Set_Square_Type (this, __, sq, tile_x, tile_y);
}

void __fastcall
patch_Map_place_scenario_things (Map * this)
{
	is->is_placing_scenario_things = true;
	Map_place_scenario_things (this);
	is->is_placing_scenario_things = false;
}

void
on_open_advisor (AdvisorKind kind)
{
}

bool __fastcall patch_Advisor_Base_Form_domestic_m95 (Advisor_Base_Form * this) { on_open_advisor (AK_DOMESTIC); return Advisor_Base_Form_domestic_m95 (this); }
bool __fastcall patch_Advisor_Base_Form_trade_m95    (Advisor_Base_Form * this) { on_open_advisor (AK_TRADE)   ; return Advisor_Base_Form_trade_m95    (this); }
bool __fastcall patch_Advisor_Base_Form_military_m95 (Advisor_Base_Form * this) { on_open_advisor (AK_MILITARY); return Advisor_Base_Form_military_m95 (this); }
bool __fastcall patch_Advisor_Base_Form_foreign_m95  (Advisor_Base_Form * this) { on_open_advisor (AK_FOREIGN) ; return Advisor_Base_Form_foreign_m95  (this); }
bool __fastcall patch_Advisor_Base_Form_cultural_m95 (Advisor_Base_Form * this) { on_open_advisor (AK_CULTURAL); return Advisor_Base_Form_cultural_m95 (this); }
bool __fastcall patch_Advisor_Base_Form_science_m95  (Advisor_Base_Form * this) { on_open_advisor (AK_SCIENCE) ; return Advisor_Base_Form_science_m95  (this); }

// TCC requires a main function be defined even though it's never used.
int main () { return 0; }
