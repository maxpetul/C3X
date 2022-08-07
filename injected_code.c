
#include "stdlib.h"
#include "stdio.h"

#include "C3X.h"

void (__stdcall ** p_OutputDebugStringA) (char * lpOutputString) = ADDR_ADDR_OUTPUTDEBUGSTRINGA;

short (__stdcall ** p_GetAsyncKeyState) (int vKey) = ADDR_ADDR_GETASYNCKEYSTATE;

FARPROC (__stdcall ** p_GetProcAddress) (HMODULE hModule, char const * lpProcName) = ADDR_ADDR_GETPROCADDRESS;

HMODULE (__stdcall ** p_GetModuleHandleA) (char const * lpModuleName) = ADDR_ADDR_GETMODULEHANDLEA;

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
#define MessageBoxA is->MessageBoxA
#define MultiByteToWideChar is->MultiByteToWideChar
#define WideCharToMultiByte is->WideCharToMultiByte
#define GetLastError is->GetLastError
#define snprintf is->snprintf
#define malloc is->malloc
#define calloc is->calloc
#define realloc is->realloc
#define free is->free
#define strtol is->strtol
#define strncmp is->strncmp
#define strlen is->strlen
#define strncpy is->strncpy
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

void
pop_up_in_game_error (char const * msg)
{
	PopupForm * popup = get_popup_form ();
	popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_ERROR", -1, 0, 0, 0);
	PopupForm_add_text (popup, __, (char *)msg, 0);
	show_popup (popup, __, 0, 0);
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
	// Free list of perfume specs
	if (is->current_config.perfume_specs != NULL) {
		free (is->current_config.perfume_specs);
		is->current_config.perfume_specs = NULL;
		is->current_config.count_perfume_specs = 0;
	}

	// Free building-unit prereqs table
	size_t building_unit_prereqs_capacity = table_capacity (&is->current_config.building_unit_prereqs);
	for (size_t n = 0; n < building_unit_prereqs_capacity; n++) {
		int ptr;
		if (table_get_by_index (&is->current_config.building_unit_prereqs, n, &ptr) && ((ptr & 1) == 0))
			free ((void *)ptr);
	}
	table_deinit (&is->current_config.building_unit_prereqs);

	// Free list of mills
	if (is->current_config.mills != NULL) {
		free (is->current_config.mills);
		is->current_config.mills = NULL;
		is->current_config.count_mills = 0;
	}

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

int
find_improv_id_by_name (struct string_slice const * name, int * out)
{
	Improvement * improv;
	if (name->len <= sizeof improv->Name)
		for (int n = 0; n < p_bic_data->ImprovementsCount; n++) {
			improv = &p_bic_data->Improvements[n];
			if (strncmp (improv->Name.S, name->str, name->len) == 0) {
				*out = n;
				return 1;
			}
		}
	return 0;
}

int
find_unit_type_id_by_name (struct string_slice const * name, int * out)
{
	UnitType * unit_type;
	if (name->len <= sizeof unit_type->Name)
		for (int n = 0; n < p_bic_data->UnitTypeCount; n++) {
			unit_type = &p_bic_data->UnitTypes[n];
			if (strncmp (unit_type->Name, name->str, name->len) == 0) {
				*out = n;
				return 1;
			}
		}
	return 0;
}

int
find_resource_id_by_name (struct string_slice const * name, int * out)
{
	Resource_Type * res_type;
	if (name->len <= sizeof res_type->Name)
		for (int n = 0; n < p_bic_data->ResourceTypeCount; n++) {
			res_type = &p_bic_data->ResourceTypes[n];
			if (strncmp (res_type->Name, name->str, name->len) == 0) {
				*out = n;
				return 1;
			}
		}
	return 0;
}

// Converts a build name (like "Spearman" or "Granary") into a City_Order struct. Returns 0 if no by improvement or unit type was found, else 1.
int
find_city_order_by_name (struct string_slice const * name, City_Order * out)
{
	int id;
	if (find_improv_id_by_name (name, &id)) {
		out->OrderID = id;
		out->OrderType = COT_Improvement;
		return 1;
	} else if (find_unit_type_id_by_name (name, &id)) {
		out->OrderID = id;
		out->OrderType = COT_Unit;
		return 1;
	} else
		return 0;
}

// A "recognizable" is something that contains the name of a unit, building, etc. When parsing one, it's possible for it to be grammatically valid but
// contain a name that doesn't match anything in the scenario data. That's a special kind of error. In that case, we record the error to report in a
// popup and continue parsing.
enum recognizable_parse_result {
	RPR_OK = 0,
	RPR_UNRECOGNIZED,
	RPR_PARSE_ERROR
};

enum recognizable_parse_result
parse_perfume_spec (char ** p_cursor, struct error_line ** p_unrecognized_lines, void * out_perfume_spec)
{
	char * cur = *p_cursor;
	struct string_slice name, amount_str;
	City_Order order;
	int amount;
	if (parse_string (&cur, &name) &&
	    skip_punctuation (&cur, ':') &&
	    parse_string (&cur, &amount_str) &&
	    read_int (&amount_str, &amount)) {
		*p_cursor = cur;
		if (find_city_order_by_name (&name, &order)) {
			struct perfume_spec * out = out_perfume_spec;
			out->target_order = order;
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
	struct string_slice improv_name, second;
	if (parse_string (&cur, &improv_name) &&
	    skip_punctuation (&cur, ':') &&
	    parse_string (&cur, &second)) {

		int is_local;
		struct string_slice resource_name;
		if (strncmp ("local", second.str, second.len) == 0) {
			if (! parse_string (&cur, &resource_name))
				return RPR_PARSE_ERROR;
			is_local = 1;
		} else {
			is_local = 0;
			resource_name = second;
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
			out->is_local = is_local;
			return RPR_OK;
		}
	} else
		return RPR_PARSE_ERROR;
}

// Recognizable items are appended to out_list/count, which must have been previously initialized (NULL/0 is valid for an empty list).
int
read_recognizables (struct string_slice const * s,
		    struct error_line ** p_unrecognized_lines,
		    int item_size,
		    enum recognizable_parse_result (* parse_item) (char **, struct error_line **, void *),
		    void ** inout_list,
		    int * inout_count)
{
	if (s->len <= 0)
		return 1;
	char * extracted_slice = extract_slice (s);
	char * cursor = extracted_slice;

	int success = 0;
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

			if (skip_punctuation (&cursor, ','))
				continue;
			else if (skip_horiz_space (&cursor) && (*cursor == '\0')) {
				success = 1;
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
	return success;
}

int
read_building_unit_prereqs (struct string_slice const * s,
			    struct error_line ** p_unrecognized_lines,
			    struct table * building_unit_prereqs)
{
	if (s->len <= 0)
		return 1;
	char * extracted_slice = extract_slice (s);
	char * cursor = extracted_slice;
	int success = 0;

	struct prereq {
		int building_id;
		int unit_type_id;
	} * new_prereqs = NULL;
	int new_prereqs_capacity = 0;
	int count_new_prereqs = 0;

	while (1) {
		struct string_slice building_name;
		if (parse_string (&cursor, &building_name)) {
			int building_id;
			int have_building_id = find_improv_id_by_name (&building_name, &building_id);
			if (! have_building_id)
				add_unrecognized_line (p_unrecognized_lines, &building_name);
			if (! skip_punctuation (&cursor, ':'))
				break;
			struct string_slice unit_type_name;
			while (skip_white_space (&cursor) && parse_string (&cursor, &unit_type_name)) {
				int unit_type_id;
				if (find_unit_type_id_by_name (&unit_type_name, &unit_type_id)) {
					if (have_building_id) {
						reserve (sizeof new_prereqs[0], (void **)&new_prereqs, &new_prereqs_capacity, count_new_prereqs);
						new_prereqs[count_new_prereqs++] = (struct prereq) { .building_id = building_id, .unit_type_id = unit_type_id };
					}
				} else
					add_unrecognized_line (p_unrecognized_lines, &unit_type_name);
			}
			skip_punctuation (&cursor, ',');
			skip_white_space (&cursor);
		} else {
			success = (*cursor == '\0');
			break;
		}
	}

	// If parsing succeeded, add the new prereq rules to the table
	if (success)
		for (int n = 0; n < count_new_prereqs; n++) {
			struct prereq * prereq = &new_prereqs[n];

			// If this unit type ID is not already in the table, insert it paired with the encoded building ID
			int prev_val;
			if (! table_look_up (building_unit_prereqs, prereq->unit_type_id, &prev_val))
				table_insert (building_unit_prereqs, prereq->unit_type_id, (prereq->building_id << 1) | 1);

			// If the unit type ID is already associated with a building ID, create a list for both the old and new building IDs
			else if (prev_val & 1) {
				int * list = malloc (MAX_BUILDING_PREREQS_FOR_UNIT * sizeof *list);
				for (int n = 0; n < MAX_BUILDING_PREREQS_FOR_UNIT; n++)
					list[n] = -1;
				list[0] = prev_val >> 1; // Decode
				list[1] = prereq->building_id;
				table_insert (building_unit_prereqs, new_prereqs[n].unit_type_id, (int)list);

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


	free (new_prereqs);
	free (extracted_slice);
	return success;
}

int
read_retreat_rules (struct string_slice const * s, int * out_val)
{
	struct string_slice trimmed = trim_string_slice (s, 1);
	if      (0 == strncmp (trimmed.str, "standard" , trimmed.len)) { *out_val = RR_STANDARD;  return 1; }
	else if (0 == strncmp (trimmed.str, "none"     , trimmed.len)) { *out_val = RR_NONE;      return 1; }
	else if (0 == strncmp (trimmed.str, "all-units", trimmed.len)) { *out_val = RR_ALL_UNITS; return 1; }
	else if (0 == strncmp (trimmed.str, "if-faster", trimmed.len)) { *out_val = RR_IF_FASTER; return 1; }
	else
		return 0;

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

	char * cursor = text;
	int displayed_error_message = 0;
	struct error_line * unrecognized_lines = NULL;
	while (1) {
		skip_horiz_space (&cursor);
		if (*cursor == '\0')
			break;
		else if (*cursor == '\n')
			cursor++; // Continue to next line
		else if (*cursor == ';')
			skip_to_line_end (&cursor); // Skip comment line
		else if (*cursor == '[')
			skip_to_line_end (&cursor); // Skip section line
		else { // parse key-value pair
			struct string_slice key, value;
			if (parse_key_value_pair (&cursor, &key, &value)) {
				int ival;
				if ((0 == strncmp (key.str, "enable_stack_bombard", key.len)) && read_int (&value, &ival))
					cfg->enable_stack_bombard = ival != 0;
				else if ((0 == strncmp (key.str, "enable_disorder_warning", key.len)) && read_int (&value, &ival))
					cfg->enable_disorder_warning = ival != 0;
				else if ((0 == strncmp (key.str, "allow_stealth_attack_against_single_unit", key.len)) && read_int (&value, &ival))
					cfg->allow_stealth_attack_against_single_unit = ival != 0;
				else if ((0 == strncmp (key.str, "show_detailed_city_production_info", key.len)) && read_int (&value, &ival))
					cfg->show_detailed_city_production_info = ival != 0;
				else if ((0 == strncmp (key.str, "limit_railroad_movement", key.len)) && read_int (&value, &ival))
					cfg->limit_railroad_movement = ival;
				else if ((0 == strncmp (key.str, "enable_free_buildings_from_small_wonders", key.len)) && read_int (&value, &ival))
					cfg->enable_free_buildings_from_small_wonders = ival != 0;
				else if ((0 == strncmp (key.str, "enable_stack_unit_commands", key.len)) && read_int (&value, &ival))
					cfg->enable_stack_unit_commands = ival != 0;
				else if ((0 == strncmp (key.str, "skip_repeated_tile_improv_replacement_asks", key.len)) && read_int (&value, &ival))
					cfg->skip_repeated_tile_improv_replacement_asks = ival != 0;
				else if ((0 == strncmp (key.str, "autofill_best_gold_amount_when_trading", key.len)) && read_int (&value, &ival))
					cfg->autofill_best_gold_amount_when_trading = ival != 0;
				else if ((0 == strncmp (key.str, "adjust_minimum_city_separation", key.len)) && read_int (&value, &ival))
					cfg->adjust_minimum_city_separation = ival;
				else if ((0 == strncmp (key.str, "disallow_founding_next_to_foreign_city", key.len)) && read_int (&value, &ival))
					cfg->disallow_founding_next_to_foreign_city = ival != 0;
				else if ((0 == strncmp (key.str, "enable_trade_screen_scroll", key.len)) && read_int (&value, &ival))
					cfg->enable_trade_screen_scroll = ival != 0;
				else if ((0 == strncmp (key.str, "group_units_on_right_click_menu", key.len)) && read_int (&value, &ival))
					cfg->group_units_on_right_click_menu = ival != 0;
				else if ((0 == strncmp (key.str, "anarchy_length_reduction_percent", key.len)) && read_int (&value, &ival))
					cfg->anarchy_length_reduction_percent = ival;
				else if ((0 == strncmp (key.str, "show_golden_age_turns_remaining", key.len)) && read_int (&value, &ival))
					cfg->show_golden_age_turns_remaining = ival != 0;
				else if ((0 == strncmp (key.str, "reverse_specialist_order_with_shift", key.len)) && read_int (&value, &ival))
					cfg->reverse_specialist_order_with_shift = ival != 0;
				else if ((0 == strncmp (key.str, "dont_give_king_names_in_non_regicide_games", key.len)) && read_int (&value, &ival))
					cfg->dont_give_king_names_in_non_regicide_games = ival != 0;
				else if ((0 == strncmp (key.str, "disable_worker_automation", key.len)) && read_int (&value, &ival))
					cfg->disable_worker_automation = ival != 0;
				else if ((0 == strncmp (key.str, "enable_land_sea_intersections", key.len)) && read_int (&value, &ival))
					cfg->enable_land_sea_intersections = ival != 0;
				else if ((0 == strncmp (key.str, "disallow_trespassing", key.len)) && read_int (&value, &ival))
					cfg->disallow_trespassing = ival != 0;
				else if ((0 == strncmp (key.str, "show_detailed_tile_info", key.len)) && read_int (&value, &ival))
					cfg->show_detailed_tile_info = ival != 0;
				else if ((0 == strncmp (key.str, "perfume_specs", key.len)) &&
					 read_recognizables (&value,
							     &unrecognized_lines,
							     sizeof (struct perfume_spec),
							     parse_perfume_spec,
							     (void **)&cfg->perfume_specs,
							     &cfg->count_perfume_specs))
					;
				else if ((0 == strncmp (key.str, "building_prereqs_for_units", key.len)) && read_building_unit_prereqs (&value, &unrecognized_lines, &cfg->building_unit_prereqs))
					;
				else if ((0 == strncmp (key.str, "buildings_generating_resources", key.len)) &&
					 read_recognizables (&value,
							     &unrecognized_lines,
							     sizeof (struct mill),
							     parse_mill,
							     (void **)&cfg->mills,
							     &cfg->count_mills))
					;
				else if ((0 == strncmp (key.str, "warn_about_unrecognized_names", key.len)) && read_int (&value, &ival))
					cfg->warn_about_unrecognized_names = ival != 0;
				else if ((0 == strncmp (key.str, "enable_ai_production_ranking", key.len)) && read_int (&value, &ival))
					cfg->enable_ai_production_ranking = ival != 0;
				else if ((0 == strncmp (key.str, "enable_ai_city_location_desirability_display", key.len)) && read_int (&value, &ival))
					cfg->enable_ai_city_location_desirability_display = ival != 0;
				else if ((0 == strncmp (key.str, "zero_corruption_when_off", key.len)) && read_int (&value, &ival))
					cfg->zero_corruption_when_off = ival;
				else if ((0 == strncmp (key.str, "disallow_land_units_from_affecting_water_tiles", key.len)) && read_int (&value, &ival))
					cfg->disallow_land_units_from_affecting_water_tiles = ival;
				else if ((0 == strncmp (key.str, "dont_end_units_turn_after_airdrop", key.len)) && read_int (&value, &ival))
					cfg->dont_end_units_turn_after_airdrop = ival;
				else if ((0 == strncmp (key.str, "enable_negative_pop_pollution", key.len)) && read_int (&value, &ival))
					cfg->enable_negative_pop_pollution = ival;
				else if ((0 == strncmp (key.str, "retreat_rules", key.len)) && read_retreat_rules (&value, &ival))
					cfg->retreat_rules = ival;

				else if ((0 == strncmp (key.str, "use_offensive_artillery_ai", key.len)) && read_int (&value, &ival))
					cfg->use_offensive_artillery_ai = ival != 0;
				else if ((0 == strncmp (key.str, "ai_build_artillery_ratio", key.len)) && read_int (&value, &ival))
					cfg->ai_build_artillery_ratio = ival;
				else if ((0 == strncmp (key.str, "ai_artillery_value_damage_percent", key.len)) && read_int (&value, &ival))
					cfg->ai_artillery_value_damage_percent = ival;
				else if ((0 == strncmp (key.str, "ai_build_bomber_ratio", key.len)) && read_int (&value, &ival))
					cfg->ai_build_bomber_ratio = ival;
				else if ((0 == strncmp (key.str, "replace_leader_unit_ai", key.len)) && read_int (&value, &ival))
					cfg->replace_leader_unit_ai = ival != 0;
				else if ((0 == strncmp (key.str, "fix_ai_army_composition", key.len)) && read_int (&value, &ival))
					cfg->fix_ai_army_composition = ival != 0;
				else if ((0 == strncmp (key.str, "enable_pop_unit_ai", key.len)) && read_int (&value, &ival))
					cfg->enable_pop_unit_ai = ival != 0;

				else if ((0 == strncmp (key.str, "remove_unit_limit", key.len)) && read_int (&value, &ival))
					cfg->remove_unit_limit = ival != 0;
				else if ((0 == strncmp (key.str, "remove_era_limit", key.len)) && read_int (&value, &ival))
					cfg->remove_era_limit = ival != 0;

				else if ((0 == strncmp (key.str, "patch_submarine_bug", key.len)) && read_int (&value, &ival))
					cfg->patch_submarine_bug = ival != 0;
				else if ((0 == strncmp (key.str, "patch_science_age_bug", key.len)) && read_int (&value, &ival))
					cfg->patch_science_age_bug = ival != 0;
				else if ((0 == strncmp (key.str, "patch_pedia_texture_bug", key.len)) && read_int (&value, &ival))
					cfg->patch_pedia_texture_bug = ival != 0;
				else if ((0 == strncmp (key.str, "patch_disembark_immobile_bug", key.len)) && read_int (&value, &ival))
					cfg->patch_disembark_immobile_bug = ival != 0;
				else if ((0 == strncmp (key.str, "patch_houseboat_bug", key.len)) && read_int (&value, &ival))
					cfg->patch_houseboat_bug = ival != 0;
				else if ((0 == strncmp (key.str, "patch_intercept_lost_turn_bug", key.len)) && read_int (&value, &ival))
					cfg->patch_intercept_lost_turn_bug = ival != 0;
				else if ((0 == strncmp (key.str, "patch_phantom_resource_bug", key.len)) && read_int (&value, &ival))
					cfg->patch_phantom_resource_bug = ival != 0;

				else if ((0 == strncmp (key.str, "prevent_autorazing", key.len)) && read_int (&value, &ival))
					cfg->prevent_autorazing = ival != 0;
				else if ((0 == strncmp (key.str, "prevent_razing_by_ai_players", key.len)) && read_int (&value, &ival))
					cfg->prevent_razing_by_ai_players = ival != 0;

				else if (! displayed_error_message) {
					snprintf (err_msg, sizeof err_msg, "Error processing config option \"%.*s\" in \"%s\". Either the name of the option is not recognized or the value is invalid.", key.len, key.str, full_path);
					err_msg[(sizeof err_msg) - 1] = '\0';
					pop_up_in_game_error (err_msg);
					displayed_error_message = 1;
				}
			} else {
				if (! displayed_error_message) {
					int line_no = 1;
					for (char * c = text; c < cursor; c++)
						line_no += *c == '\n';
					snprintf (err_msg, sizeof err_msg, "Parse error on line %d of \"%s\".", line_no, full_path);
					err_msg[(sizeof err_msg) - 1] = '\0';
					pop_up_in_game_error (err_msg);
					displayed_error_message = 1;
				}
				skip_to_line_end (&cursor);
			}
		}
	}

	if (cfg->warn_about_unrecognized_names && (unrecognized_lines != NULL)) {
		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, "C3X_WARNING", -1, 0, 0, 0);
		char s[200];
		snprintf (s, sizeof s, "Unrecognized names in %s:", full_path);
		s[(sizeof s) - 1] = '\0';
		PopupForm_add_text (popup, __, s, 0);
		for (struct error_line * line = unrecognized_lines; line != NULL; line = line->next)
			PopupForm_add_text (popup, __, line->text, 0);
		show_popup (popup, __, 0, 0);
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

char __fastcall
patch_Leader_impl_would_raze_city (Leader * this, int edx, City * city)
{
	return is->current_config.prevent_razing_by_ai_players ? 0 : Leader_impl_would_raze_city (this, __, city);
}

// This function is used to fix a bug where the game would crash when using disembark all on a transport that contained an immobile unit. The bug
// comes from the fact that the function to disembark all units loops continuously over units in the transport until there are none left that
// can be disembarked. The problem is the logic to check disembarkability erroneously reports immobile units as disembarkable when they're not,
// so the program gets stuck in an infinite loop. The fix affects the function that checks disembarkability, replacing a call to
// Unit_get_max_move_points with a call to the function below. This function returns zero for immobile units, causing the caller to report
// (correctly) that the unit cannot be disembarked.
int __fastcall
patch_Unit_get_max_move_points_for_disembarking (Unit * this)
{
	if (is->current_config.patch_disembark_immobile_bug &&
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
get_neighbor_coords (Map * map, int x, int y, int neighbor_index, int * out_x, int * out_y)
{
	int dx, dy;
	neighbor_index_to_displacement (neighbor_index, &dx, &dy);
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

City *
city_at (int x, int y)
{
	Tile * tile = tile_at (x, y);
	return get_city_ptr (tile->vtable->m45_Get_City_ID (tile));
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

int
has_active_building (City * city, int improv_id)
{
	Leader * owner = &leaders[city->Body.CivID];
	Improvement * improv = &p_bic_data->Improvements[improv_id];
	return City_has_improvement (city, __, improv_id, 1) && // building is physically present in city AND
		((improv->ObsoleteID < 0) || (! Leader_has_tech (owner, __, improv->ObsoleteID))) && // building is not obsolete AND
		((improv->GovernmentID < 0) || (improv->GovernmentID == owner->GovernmentType)); // building is not restricted to a different govt
}

int
can_player_use_resource (int civ_id, int resource_id)
{
	Resource_Type * res = &p_bic_data->ResourceTypes[resource_id];
	return (res->RequireID < 0) || Leader_has_tech (&leaders[civ_id], __, res->RequireID);
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
	for (int n = 0; n < is->current_config.count_perfume_specs; n++) {
		struct perfume_spec const * spec = &is->current_config.perfume_specs[n];
		if ((spec->target_order.OrderType == order->OrderType) && (spec->target_order.OrderID == order->OrderID)) {
			valuation += spec->amount;
			break;
		}
	}

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
int has_resources_required_by_building_r (City * city, int improv_id, int max_req_resource_id);

byte __fastcall
patch_City_has_resource (City * this, int edx, int resource_id)
{
	byte tr;
	if (is->current_config.patch_phantom_resource_bug &&
	    (resource_id >= 32) && (resource_id < p_bic_data->ResourceTypeCount) &&
	    (! City_has_trade_connection_to_capital (this))) {
		unsigned bits = (this->Body.ID < is->extra_available_resources_capacity) ? *get_extra_resource_bits (this->Body.ID, resource_id) : 0;
		tr = (bits >> (resource_id&31)) & 1;
	} else
		tr = City_has_resource (this, __, resource_id);

	// Check if access to this resource is provided by a building in the city
	if ((! tr) && can_player_use_resource (this->Body.CivID, resource_id))
		for (int n = 0; n < is->current_config.count_mills; n++) {
			struct mill * mill = &is->current_config.mills[n];
			if ((mill->resource_id == resource_id) &&
			    mill->is_local &&
			    has_active_building (this, mill->improv_id) &&
			    has_resources_required_by_building_r (this, mill->improv_id, mill->resource_id - 1)) {
				tr = 1;
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
int
has_resources_required_by_building_r (City * city, int improv_id, int max_req_resource_id)
{
	Improvement * improv = &p_bic_data->Improvements[improv_id];
	if (! (improv->ImprovementFlags & ITF_Required_Goods_Must_Be_In_City_Radius)) {
		for (int n = 0; n < 2; n++) {
			int res_id = (&improv->Resource1ID)[n];
			if ((res_id >= 0) &&
			    ((res_id > max_req_resource_id) || (! patch_City_has_resource (city, __, res_id))))
				return 0;
		}
		return 1;
	} else {
		int * targets = &improv->Resource1ID;
		if ((targets[0] < 0) && (targets[1] < 0))
			return 1;
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

int
has_resources_required_by_building (City * city, int improv_id)
{
	return has_resources_required_by_building_r (city, improv_id, INT_MAX);
}

int
compare_mill_tiles (void const * vp_a, void const * vp_b)
{
	struct mill_tile const * a = vp_a, * b = vp_b;
	return a->mill->resource_id - b->mill->resource_id;
}

void __fastcall
patch_Trade_Net_recompute_resources (Trade_Net * this, int edx, byte skip_popups)
{
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
					if ((! mill->is_local) &&
					    has_active_building (city, mill->improv_id) &&
					    can_player_use_resource (city->Body.CivID, mill->resource_id)) {
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
	p_bic_data->Map.TileCount = is->saved_tile_count;
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

// Just calls VirtualProtect and displays an error message if it fails. Made for use by the WITH_MEM_PROTECTION macro.
int
check_virtual_protect (LPVOID addr, SIZE_T size, DWORD flags, PDWORD old_protect)
{
	if (VirtualProtect (addr, size, flags, old_protect))
		return 1;
	else {
		char err_msg[1000];
		snprintf (err_msg, sizeof err_msg, "VirtualProtect failed! Args:\n  Address: 0x%p\n  Size: %d\n  Flags: 0x%x", addr, size, flags);
		err_msg[(sizeof err_msg) - 1] = '\0';
		MessageBoxA (NULL, err_msg, NULL, MB_ICONWARNING);
		return 0;
	}
}

#define WITH_MEM_PROTECTION(addr, size, flags)				                \
	for (DWORD old_protect, unused, iter_count = 0;			                \
	     (iter_count == 0) && check_virtual_protect (addr, size, flags, &old_protect); \
	     VirtualProtect (addr, size, old_protect, &unused), iter_count++)

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

	// Fix submarine bug
	// Address refers to the last parameter (respect_unit_visiblity) for a call to Unit::is_visible_to_civ inside some kind of pathfinding
	// function.
	WITH_MEM_PROTECTION (ADDR_SUB_BUG_PATCH, 1, PAGE_EXECUTE_READWRITE)
		*(byte *)ADDR_SUB_BUG_PATCH = cfg->patch_submarine_bug ? 0 : 1;

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
			byte * cursor = ADDR_HOUSEBOAT_BUG_PATCH;
			*cursor++ = 0x50; // push eax
			int call_offset = (int)&tile_at_city_or_null - ((int)cursor + 5);
			*cursor++ = 0xE8; // call
			cursor = int_to_bytes (cursor, call_offset);
			for (; cursor < ADDR_HOUSEBOAT_BUG_PATCH_END; cursor++)
				*cursor = 0x90; // nop
		} else
			memmove (ADDR_HOUSEBOAT_BUG_PATCH,
				 is->houseboat_patch_area_original_contents,
				 ADDR_HOUSEBOAT_BUG_PATCH_END - ADDR_HOUSEBOAT_BUG_PATCH);
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
}

void
patch_init_floating_point ()
{
	init_floating_point ();

	// NOTE: At this point the program is done with the CRT initialization stuff and will start calling constructors for global
	// objects as soon as this function returns. This is a good place to inject code that will run at program start.

	is->kernel32 = (*p_GetModuleHandleA) ("kernel32.dll");
	is->user32   = (*p_GetModuleHandleA) ("user32.dll");
	is->msvcrt   = (*p_GetModuleHandleA) ("msvcrt.dll");

	// Remember the function names here are macros that expand to is->...
	VirtualProtect      = (void *)(*p_GetProcAddress) (is->kernel32, "VirtualProtect");
	CloseHandle         = (void *)(*p_GetProcAddress) (is->kernel32, "CloseHandle");
	CreateFileA         = (void *)(*p_GetProcAddress) (is->kernel32, "CreateFileA");
	GetFileSize         = (void *)(*p_GetProcAddress) (is->kernel32, "GetFileSize");
	ReadFile            = (void *)(*p_GetProcAddress) (is->kernel32, "ReadFile");
	MultiByteToWideChar = (void *)(*p_GetProcAddress) (is->kernel32, "MultiByteToWideChar");
	WideCharToMultiByte = (void *)(*p_GetProcAddress) (is->kernel32, "WideCharToMultiByte");
	GetLastError        = (void *)(*p_GetProcAddress) (is->kernel32, "GetLastError");
	MessageBoxA = (void *)(*p_GetProcAddress) (is->user32, "MessageBoxA");
	snprintf = (void *)(*p_GetProcAddress) (is->msvcrt, "_snprintf");
	malloc   = (void *)(*p_GetProcAddress) (is->msvcrt, "malloc");
	calloc   = (void *)(*p_GetProcAddress) (is->msvcrt, "calloc");
	realloc  = (void *)(*p_GetProcAddress) (is->msvcrt, "realloc");
	free     = (void *)(*p_GetProcAddress) (is->msvcrt, "free");
	strtol   = (void *)(*p_GetProcAddress) (is->msvcrt, "strtol");
	strncmp  = (void *)(*p_GetProcAddress) (is->msvcrt, "strncmp");
	strlen   = (void *)(*p_GetProcAddress) (is->msvcrt, "strlen");
	strncpy  = (void *)(*p_GetProcAddress) (is->msvcrt, "strncpy");
	strdup   = (void *)(*p_GetProcAddress) (is->msvcrt, "_strdup");
	strstr   = (void *)(*p_GetProcAddress) (is->msvcrt, "strstr");
	qsort    = (void *)(*p_GetProcAddress) (is->msvcrt, "qsort");
	memcmp   = (void *)(*p_GetProcAddress) (is->msvcrt, "memcmp");
	memcpy   = (void *)(*p_GetProcAddress) (is->msvcrt, "memcpy");

	// Set file path to mod's script.txt
	snprintf (is->mod_script_path, sizeof is->mod_script_path, "%s\\Text\\c3x-script.txt", is->mod_rel_dir);
	is->mod_script_path[(sizeof is->mod_script_path) - 1] = '\0';

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

	is->have_job_and_loc_to_skip = 0;

	// Initialize trade screen scroll vars
	is->open_diplo_form_straight_to_trade = 0;
	is->trade_screen_scroll_to_id = -1;
	is->trade_scroll_button_left = is->trade_scroll_button_right = NULL;
	is->trade_scroll_button_state = IS_UNINITED;
	is->eligible_for_trade_scroll = 0;

	// Read contents of region potentially overwritten by patch
	memmove (is->houseboat_patch_area_original_contents,
		 ADDR_HOUSEBOAT_BUG_PATCH,
		 ADDR_HOUSEBOAT_BUG_PATCH_END - ADDR_HOUSEBOAT_BUG_PATCH);

	is->unit_menu_duplicates = NULL;

	is->memo = NULL;
	is->memo_len = 0;
	is->memo_capacity = 0;

	is->city_loc_display_perspective = -1;

	is->extra_available_resources = NULL;
	is->extra_available_resources_capacity = 0;

	is->ai_prod_valuations = NULL;
	is->count_ai_prod_valuations = 0;
	is->ai_prod_valuations_capacity = 0;

	is->mill_tiles = NULL;
	is->count_mill_tiles = 0;
	is->mill_tiles_capacity = 0;

	is->loaded_config_names = NULL;
	reset_to_base_config ();
	apply_machine_code_edits (&is->current_config);
}

void
init_stackable_command_buttons ()
{
	if (is->sc_img_state == IS_UNINITED) {
		char temp_path[2*MAX_PATH];

		is->sb_activated_by_button = 0;
		is->sc_img_state = IS_INIT_FAILED;

		char const * filenames[4] = {"StackedNormButtons", "StackedRolloverButtons", "StackedHighlightedButtons", "StackedButtonsAlpha"};
		for (int n = 0; n < 4; n++) {
			PCX_Image * pcx = &is->sc_button_sheets[n];
			PCX_Image_construct (pcx);
			snprintf (temp_path, sizeof temp_path, "%s\\Art\\%s.pcx", is->mod_rel_dir, filenames[n]);
			temp_path[(sizeof temp_path) - 1] = '\0';
			PCX_Image_read_file (pcx, __, temp_path, NULL, 0, 0x100, 2);
			if (pcx->JGL.Image == NULL) {
				(*p_OutputDebugStringA) ("[C3X] Failed to load stacked command buttons sprite sheet.");
				return;
			}
		}
		
		for (int sc = 0; sc < COUNT_STACKABLE_COMMANDS; sc++) {
			int x = 32 * sc_button_infos[sc].tile_sheet_column,
			    y = 32 * sc_button_infos[sc].tile_sheet_row;
			for (int n = 0; n < 4; n++)
				Tile_Image_Info_slice_pcx (&is->sc_button_image_sets[sc].imgs[n], __, &is->sc_button_sheets[n], x, y, 32, 32, 1, 0);
		}

		is->sc_img_state = IS_OK;
	}
}

void
init_tile_highlights ()
{
	if (is->tile_highlight_state == IS_UNINITED) {
		char temp_path[2*MAX_PATH];

		is->tile_highlight_state = IS_INIT_FAILED;

		PCX_Image * pcx = &is->tile_highlight_sheet;
		PCX_Image_construct (pcx);
		snprintf (temp_path, sizeof temp_path, "%s\\Art\\TileHighlights.pcx", is->mod_rel_dir);
		temp_path[(sizeof temp_path) - 1] = '\0';
		PCX_Image_read_file (pcx, __, temp_path, NULL, 0, 0x100, 2);
		if (pcx->JGL.Image == NULL) {
			(*p_OutputDebugStringA) ("[C3X] Failed to load stacked command buttons sprite sheet.");
			return;
		}

		for (int n = 0; n < COUNT_TILE_HIGHLIGHTS; n++)
			Tile_Image_Info_slice_pcx (&is->tile_highlights[n], __, pcx, 128*n, 0, 128, 64, 1, 0);

		is->tile_highlight_state = IS_OK;
	}
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
	PCX_Image * pcx = new (sizeof *pcx);
	PCX_Image_construct (pcx);
	snprintf (temp_path, sizeof temp_path, "%s\\Art\\TradeScrollButtons.pcx", is->mod_rel_dir);
	temp_path[(sizeof temp_path) - 1] = '\0';
	PCX_Image_read_file (pcx, __, temp_path, NULL, 0, 0x100, 2);
	if (pcx->JGL.Image == NULL) {
		is->trade_scroll_button_state = IS_INIT_FAILED;
		(*p_OutputDebugStringA) ("[C3X] Failed to load TradeScrollButtons.pcx");
		return;
	}

	// imgs stores normal, rollover, and highlight images, in that order, first for the left button then for the right
	Tile_Image_Info * imgs[6];
	for (int n = 0; n < (sizeof imgs) / (sizeof imgs[0]); n++) {
		imgs[n] = new (sizeof **imgs);
		Tile_Image_Info_construct (imgs[n]);
	}

	for (int right = 0; right < 2; right++)
		for (int n = 0; n < 3; n++)
			Tile_Image_Info_slice_pcx (imgs[n + 3*right], __, pcx, right ? 44 : 0, 1 + 48*n, 43, 47, 1, 1);

	for (int right = 0; right < 2; right++) {
		Button * b = new (sizeof *b);

		Button_construct (b);
		int id = right ? TRADE_SCROLL_BUTTON_ID_RIGHT : TRADE_SCROLL_BUTTON_ID_LEFT;
		Button_initialize (b, __, NULL, id, right ? 622 : 358, 50, 43, 47, (Base_Form *)diplo_form, 0);
		for (int n = 0; n < 3; n++)
			b->Images[n] = imgs[n + 3*right];

		b->activation_handler = &activate_trade_scroll_button;
		b->field_630[0] = 0; // TODO: Is this necessary? It's done by the base game code when creating the city screen scroll buttons

		if (! right)
			is->trade_scroll_button_left = b;
		else
			is->trade_scroll_button_right = b;
	}

	is->trade_scroll_button_state = IS_OK;
}

void
init_mod_info_button_images ()
{
	if (is->mod_info_button_images_state != IS_UNINITED)
		return;

	is->mod_info_button_images_state = IS_INIT_FAILED;

	PCX_Image * descbox_pcx = new (sizeof *descbox_pcx);
	PCX_Image_construct (descbox_pcx);
	char * descbox_path = BIC_get_asset_path (p_bic_data, __, "art\\civilopedia\\descbox.pcx", 1);
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

int
can_attack_this_turn (Unit * unit)
{
	// return unit has moves left AND (unit hasn't attacked this turn OR unit has blitz ability)
	return (unit->Body.Moves < Unit_get_max_move_points (unit)) &&
		(((unit->Body.Status & 4) == 0) ||
		 UnitType_has_ability (&p_bic_data->UnitTypes[unit->Body.UnitTypeID], __, UTA_Blitz));
}

int
can_damage_bombarding (UnitType * attacker_type, Unit * defender, Tile * defender_tile)
{
	UnitType * defender_type = &p_bic_data->UnitTypes[defender->Body.UnitTypeID];
	if (defender_type->Unit_Class == UTC_Land) {
		int has_lethal_land_bombard = UnitType_has_ability (attacker_type, __, UTA_Lethal_Land_Bombardment);
		return defender->Body.Damage + (! has_lethal_land_bombard) < Unit_get_max_hp (defender);
	} else if (defender_type->Unit_Class == UTC_Sea) {
		// Land artillery can't damage ships in port
		if ((attacker_type->Unit_Class == UTC_Land) && Tile_has_city (defender_tile))
			return 0;
		int has_lethal_sea_bombard = UnitType_has_ability (attacker_type, __, UTA_Lethal_Sea_Bombardment);
		return defender->Body.Damage + (! has_lethal_sea_bombard) < Unit_get_max_hp (defender);
	} else if (defender_type->Unit_Class == UTC_Air) {
		// Can't damage aircraft in an airfield by bombarding, the attack doesn't even go off
		if ((defender_tile->vtable->m42_Get_Overlays (defender_tile, __, 0) & 0x20000000) != 0)
			return 0;
		// Land artillery can't damage aircraft but naval artillery and other aircraft can. Lethal bombard doesn't
		// apply; anything that can damage can kill.
		return attacker_type->Unit_Class != UTC_Land;
	} else // UTC_Space? UTC_Alternate_Dimension???
		return 0;
}

int
has_any_destructible_improvements (City * city)
{
	for (int n = 0; n < p_bic_data->ImprovementsCount; n++) {
		Improvement * improv = &p_bic_data->Improvements[n];
		if (((improv->Characteristics & (ITC_Wonder | ITC_Small_Wonder)) == 0) && // if improv is not a wonder AND
		    ((improv->ImprovementFlags & ITF_Center_of_Empire) == 0) && // it's not the palace AND
		    (improv->SpaceshipPart < 0) && // it's not a spaceship part AND
		    City_has_improvement (city, __, n, 0)) // it's present in the city ignoring free improvements
			return 1;
	}
	return 0;
}

void __fastcall
patch_Main_Screen_Form_perform_action_on_tile (Main_Screen_Form * this, int edx, enum Unit_Mode_Actions action, int x, int y)
{
	if ((! is->current_config.enable_stack_bombard) || // if stack bombard is disabled OR
	    (! ((action == UMA_Bombard) || (action == UMA_Air_Bombard))) || // action is not bombard OR
	    ((((*p_GetAsyncKeyState) (VK_CONTROL)) >> 8 == 0) &&  // (control key is not down AND
	     ((is->sc_img_state != IS_UNINITED) && (is->sb_activated_by_button == 0))) || // (button flag is valid AND not set)) OR
	    is_game_type_4_or_5 ()) { // is online game
		Main_Screen_Form_perform_action_on_tile (this, __, action, x, y);
		return;
	}

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
		if ((Unit_get_defense_strength (uti.unit) > 0) &&
		    (uti.unit->Body.Container_Unit < 0) &&
		    Unit_is_visible_to_civ (uti.unit, __, civ_id, 0) &&
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
		int moves_before_bombard = next_up->Body.Moves;

		Unit_bombard_tile (next_up, __, x, y);

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
				if (unit != NULL) {
					if (can_damage_bombarding (attacker_type, unit, target_tile)) {
						anything_left_to_attack = 1;
						break;
					}
				}
			}
		} else if (target_city != NULL) {
			anything_left_to_attack =
				(target_city->Body.Population.Size > 1) ||
				has_any_destructible_improvements (target_city);
		} else if (attacking_tile) {
			int destructible_overlays =
				0x00000003 | // road, railroad
				0x00000004 | // mine
				0x00000008 | // irrigation
				0x00000010 | // fortress
				0x10000000 | // barricade
				0x20000000 | // airfield
				0x40000000 | // radar
				0x80000000;  // outpost
			anything_left_to_attack = (target_tile->vtable->m42_Get_Overlays (target_tile, __, 0) & destructible_overlays) != 0;
		} else
			anything_left_to_attack = 0;
	} while ((next_up != NULL) && anything_left_to_attack && (! last_attack_didnt_happen));

	is->sb_activated_by_button = 0;
}

void
set_up_stack_bombard_buttons (Main_GUI * this)
{
	if (is_game_type_4_or_5 () || (! is->current_config.enable_stack_bombard))
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
	    is_game_type_4_or_5 ()) // is online game
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

void __fastcall
patch_Main_GUI_set_up_unit_command_buttons (Main_GUI * this)
{
	Main_GUI_set_up_unit_command_buttons (this);
	set_up_stack_bombard_buttons (this);
	set_up_stack_worker_buttons (this);
}

void
check_happiness_at_end_of_turn ()
{
	int num_unhappy_cities = 0;
	City * first_unhappy_city = NULL;
	if (p_cities->Cities != NULL)
		for (int n = 0; n <= p_cities->LastIndex; n++) {
			City * city = get_city_ptr (n);
			if ((city != NULL) && (city->Body.CivID == p_main_screen_form->Player_CivID)) {
				City_recompute_happiness (city);
				int num_happy = 0, num_unhappy = 0;
				FOR_CITIZENS_IN (ci, city) {
					num_happy   += ci.ctzn->Body.Mood == CMT_Happy;
					num_unhappy += ci.ctzn->Body.Mood == CMT_Unhappy;
				}
				if (num_unhappy > num_happy) {
					num_unhappy_cities++;
					if (first_unhappy_city == NULL)
						first_unhappy_city = city;
				}
			}
		}

	if (first_unhappy_city != NULL) {
		PopupForm * popup = get_popup_form ();
		set_popup_str_param (0, first_unhappy_city->Body.CityName, -1, -1);
		if (num_unhappy_cities > 1)
			set_popup_int_param (1, num_unhappy_cities - 1);
		char * key = (num_unhappy_cities > 1) ? "C3X_DISORDER_WARNING_MULTIPLE" : "C3X_DISORDER_WARNING_ONE";
		popup->vtable->set_text_key_and_flags (popup, __, is->mod_script_path, key, -1, 0, 0, 0);
		int response = show_popup (popup, __, 0, 0);

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
	is->eligible_for_trade_scroll = is->current_config.enable_trade_screen_scroll && (! is_game_type_4_or_5 ());

	if (is->eligible_for_trade_scroll && (is->trade_scroll_button_state == IS_UNINITED))
		init_trade_scroll_buttons (this);

	DiploForm_do_diplomacy (this, __, diplo_message, param_2, civ_id, do_not_request_audience, war_negotiation, disallow_proposal, our_offers, their_offers);

	while (is->trade_screen_scroll_to_id >= 0) {
		int scroll_to_id = is->trade_screen_scroll_to_id;
		is->trade_screen_scroll_to_id = -1;
		is->open_diplo_form_straight_to_trade = 1;
		DiploForm_do_diplomacy (this, __, DM_AI_COUNTER, 0, scroll_to_id, 1, 0, 0, NULL, NULL);
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

int
is_worker_or_settler_command (int unit_command_value)
{
	return (unit_command_value & 0x20000000) ||
		((unit_command_value >= UCV_Build_Remote_Colony) && (unit_command_value <= UCV_Auto_Save_Tiles));
}

byte __fastcall
patch_Unit_can_perform_command (Unit * this, int edx, int unit_command_value)
{
	if (is->current_config.disable_worker_automation &&
	    (this->Body.CivID == p_main_screen_form->Player_CivID) &&
	    (unit_command_value == UCV_Automate))
		return 0;
	else if (is->current_config.disallow_land_units_from_affecting_water_tiles &&
		 is_worker_or_settler_command (unit_command_value)) {
		Tile * tile = tile_at (this->Body.X, this->Body.Y);
		enum UnitTypeClasses class = p_bic_data->UnitTypes[this->Body.UnitTypeID].Unit_Class;
		return ((class != UTC_Land) || (! tile->vtable->m35_Check_Is_Water (tile))) &&
			Unit_can_perform_command (this, __, unit_command_value);
	} else
		return Unit_can_perform_command (this, __, unit_command_value);
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

		int cost = 0;
		FOR_UNITS_ON (uti, tile)
			if ((uti.unit->Body.UnitTypeID == unit_type_id) &&
			    (uti.unit->Body.Container_Unit < 0) &&
			    (uti.unit->Body.UnitState == 0) &&
			    Unit_can_perform_command (uti.unit, __, UCV_Upgrade_Unit)) {
				cost += Unit_get_upgrade_cost (uti.unit);
				memoize (uti.id);
			}

		if (cost <= our_treasury) {
			set_popup_str_param (0, p_bic_data->UnitTypes[unit_type_id].Name, -1, -1);
			set_popup_int_param (0, is->memo_len);
			set_popup_int_param (1, cost);
			popup->vtable->set_text_key_and_flags (popup, __, script_dot_txt_file_path, "UPGRADE_ALL", -1, 0, 0, 0);
			if (show_popup (popup, __, 0, 0) == 0)
				for (int n = 0; n < is->memo_len; n++) {
					Unit * to_upgrade = get_unit_ptr (is->memo[n]);
					if (to_upgrade != NULL)
						Unit_upgrade (to_upgrade, __, 0);
				}

		} else {
			set_popup_int_param (0, cost);
			int param_5 = is_game_type_4_or_5 () ? 0x4000 : 0; // As in base code
			popup->vtable->set_text_key_and_flags (popup, __, script_dot_txt_file_path, "NO_GOLD_TO_UPGRADE_ALL", -1, 0, param_5, 0);
			show_popup (popup, __, 0, 0);
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
			if (show_popup (popup, __, 0, 0) == 0) {
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
	    is_game_type_4_or_5 ()) { // is online game
		Main_GUI_handle_button_press (this, __, button_id);
		return;
	}

	enum stackable_command_kind kind = stack_button_info->kind;
	if ((kind == SCK_TERRAFORM) || (kind == SCK_UNIT_MGMT)) {
		// I don't know what these functions do but we have to call them to replicate the behavior of the function we're replacing
		clear_something_1 ();
		clear_something_2 (&this->Data_30_1);

		if (kind == SCK_TERRAFORM)
			issue_stack_worker_command (p_main_screen_form->Current_Unit, stack_button_info->command);
		else if (kind == SCK_UNIT_MGMT)
			issue_stack_unit_mgmt_command (p_main_screen_form->Current_Unit, stack_button_info->command);
	} else
		Main_GUI_handle_button_press (this, __, button_id);
}

int
is_command_button_active (Main_GUI * main_gui, enum Unit_Command_Values command)
{
	Command_Button * buttons = main_gui->Unit_Command_Buttons;
	for (int n = 0; n < 42; n++)
		if (((buttons[n].Button.Base_Data.Status2 & 1) != 0) && (buttons[n].Command == command))
			return 1;
	return 0;
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
	int in_disorder = city->Body.Status & 1,
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

int
is_trespassing (int civ_id, Tile * from, Tile * to)
{
	int from_territory_id = from->vtable->m38_Get_Territory_OwnerID (from),
	    to_territory_id   = to  ->vtable->m38_Get_Territory_OwnerID (to);
	return (civ_id > 0) &&
		(to_territory_id != civ_id) &&
		(to_territory_id > 0) &&
		(to_territory_id != from_territory_id) &&
		(! leaders[civ_id].At_War[to_territory_id]) &&
		((leaders[civ_id].Relation_Treaties[to_territory_id] & 2) == 0); // Check right of passage
}

int
is_allowed_to_trespass (Unit * unit)
{
	int type_id = unit->Body.UnitTypeID;
	if ((type_id >= 0) && (type_id < p_bic_data->UnitTypeCount)) {
		UnitType * type = &p_bic_data->UnitTypes[type_id];
		return UnitType_has_ability (type, __, UTA_Hidden_Nationality) || UnitType_has_ability (type, __, UTA_Invisible);
	} else
		return 0;
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
		if (is_trespassing (this->Body.CivID, from, tile_at (nx, ny)) && (! is_allowed_to_trespass (this)))
			return AMV_TRIGGERS_WAR;
	}

	return base_validity;
}

int __fastcall
patch_Trade_Net_get_movement_cost (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, unsigned param_7, int neighbor_index, int param_9)
{
	int const base_cost = Trade_Net_get_movement_cost (this, __, from_x, from_y, to_x, to_y, unit, civ_id, param_7, neighbor_index, param_9);

	// Apply trespassing restriction
	if (is->current_config.disallow_trespassing &&
	    is_trespassing (civ_id, tile_at (from_x, from_y), tile_at (to_x, to_y)) &&
	    ((unit == NULL) || (! is_allowed_to_trespass (unit))))
		return -1;

	// Adjust movement cost to enforce limited railroad movement
	int rail_limit = is->current_config.limit_railroad_movement;
	if ((rail_limit > 0) && (is->saved_road_movement_rate > 0)) {
		if ((unit != NULL) && (base_cost == 0))
			return Unit_get_max_move_points (unit) / rail_limit;
		else if (base_cost == 1)
			return rail_limit; // = p_bic_data->General.RoadsMovementRate / is->saved_road_movement_rate;
	}

	return base_cost;
}

int __cdecl
patch_do_save_game (char * file_path, char param_2, GUID * guid)
{
	if ((is->current_config.limit_railroad_movement <= 0) || (is->saved_road_movement_rate <= 0))
		return do_save_game (file_path, param_2, guid);
	else {
		// If the BIC's road-movement-rate was changed to limit railroad movement, avoid writing the changed value into the save
		int rmr = p_bic_data->General.RoadsMovementRate;
		p_bic_data->General.RoadsMovementRate = is->saved_road_movement_rate;
		int tr = do_save_game (file_path, param_2, guid);
		p_bic_data->General.RoadsMovementRate = rmr;
		return tr;
	}
}

unsigned __fastcall
patch_load_scenario (void * this, int edx, char * param_1, unsigned * param_2)
{
	int ret_addr = ((int *)&param_1)[-1];

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
	char * scenario_config_path = BIC_get_asset_path (p_bic_data, __, scenario_config_file_name, 0);
	// BIC_get_asset_path returns the file name when it can't find the file
	if (0 != strncmp (scenario_config_file_name, scenario_config_path, strlen (scenario_config_file_name)))
		load_config (scenario_config_path, 0);
	apply_machine_code_edits (&is->current_config);

	// Need to clear this since the resource count might have changed
	if (is->extra_available_resources != NULL) {
		free (is->extra_available_resources);
		is->extra_available_resources = NULL;
		is->extra_available_resources_capacity = 0;
	}

	// Set up for limiting railroad movement, if enabled
	if (is->current_config.limit_railroad_movement > 0) {
		is->saved_road_movement_rate = p_bic_data->General.RoadsMovementRate;
		p_bic_data->General.RoadsMovementRate *= is->current_config.limit_railroad_movement;
	} else
		is->saved_road_movement_rate = -1;

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

int __fastcall
patch_show_popup (void * this, int edx, int param_1, int param_2)
{
	is->show_popup_was_called = 1;
	return show_popup (this, __, param_1, param_2);
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

City *
get_city_near (int x, int y)
{
	FOR_TILES_AROUND (tai, 9, x, y) {
		City * city = get_city_ptr (tai.tile->vtable->m45_Get_City_ID (tai.tile));
		if (city != NULL)
			return city;
	}
	return NULL;
}

int
any_enemies_near (Leader const * me, int tile_x, int tile_y, enum UnitTypeClasses class, int num_tiles)
{
	int tr = 0;
	FOR_TILES_AROUND (tai, num_tiles, tile_x, tile_y) {
		int enemy_on_this_tile = 0;
		FOR_UNITS_ON (uti, tai.tile) {
			UnitType const * unit_type = &p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID];
			if (Unit_is_visible_to_civ (uti.unit, __, me->ID, 0) &&
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
			tr = 1;
			break;
		}
	}
	return tr;
}

int
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
	Trade_Net_set_unit_path (p_trade_net, __, unit->Body.X, unit->Body.Y, to_tile_x, to_tile_y, unit, unit->Body.CivID, 1, &dist_in_mp);
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
	if (p_cities->Cities != NULL)
		for (int n = 0; n <= p_cities->LastIndex; n++) {
			City * city = get_city_ptr (n);
			if ((city != NULL) && (city->Body.CivID == unit->Body.CivID)) {
				Tile * city_tile = tile_at (city->Body.X, city->Body.Y);
				if (continent_id == city_tile->vtable->m46_Get_ContinentID (city_tile)) {
					int dist_in_turns;
					if (! estimate_travel_time (unit, city->Body.X, city->Body.Y, &dist_in_turns))
						continue;
					int unattractiveness = 10 * dist_in_turns;
					unattractiveness = (10 + unattractiveness) / (10 + city->Body.Population.Size);
					if (city->Body.CultureIncome > 0)
						unattractiveness /= 5;
					if (unattractiveness < lowest_unattractiveness) {
						lowest_unattractiveness = unattractiveness;
						least_unattractive_city = city;
					}
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

	// Duplicate some logic from the base function. I'm pretty sure this disbands the unit if it's on a continent with no cities of the same civ.
	if (leaders[this->Body.CivID].ContinentStat1[continent_id] == 0) {
		Unit_disband (this);
		return;
	}

	// Flee if the unit is near an enemy without adequate escort
	int has_adequate_escort; {
		int escorter_count = 0;
		int any_healthy_escorters = 0;
		int index;
		for (int escorter_id = Unit_next_escorter_id (this, __, &index, 1); escorter_id >= 0; escorter_id = Unit_next_escorter_id (this, __, &index, 0)) {
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
		byte done = this->vtable->work (this);
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
	if (p_cities->Cities != NULL)
		for (int n = 0; n <= p_cities->LastIndex; n++) {
			City * city = get_city_ptr (n);
			if ((city != NULL) && (city->Body.CivID == this->Body.CivID)) {
				Tile * city_tile = tile_at (city->Body.X, city->Body.Y);
				if ((continent_id == city_tile->vtable->m46_Get_ContinentID (city_tile)) &&
				    Unit_can_hurry_production (this, __, city, 0)) {
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
		int first_move = Trade_Net_set_unit_path (p_trade_net, __, this->Body.X, this->Body.Y, moving_to_city->Body.X, moving_to_city->Body.Y, this, this->Body.CivID, 0x101, NULL);
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

byte __fastcall
patch_impl_ai_is_good_army_addition (Unit * this, int edx, Unit * candidate)
{
	if (! is->current_config.fix_ai_army_composition)
		return impl_ai_is_good_army_addition (this, __, candidate);

	UnitType * candidate_type = &p_bic_data->UnitTypes[candidate->Body.UnitTypeID];
	Tile * tile = tile_at (this->Body.X, this->Body.Y);

	if (((candidate_type->AI_Strategy & UTAI_Offence) == 0) ||
	    UnitType_has_ability (candidate_type, __, UTA_Hidden_Nationality))
		return 0;

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

byte __fastcall
patch_City_can_build_unit (City * this, int edx, int unit_type_id, byte exclude_upgradable, int param_3, byte allow_kings)
{
	byte base = City_can_build_unit (this, __, unit_type_id, exclude_upgradable, param_3, allow_kings);

	// Apply building prereqs
	if (base) {
		int building_prereq;
		if (table_look_up (&is->current_config.building_unit_prereqs, unit_type_id, &building_prereq)) {
			// If the prereq is an encoded building ID
			if (building_prereq & 1)
				return has_active_building (this, building_prereq >> 1);

			// Else it's a pointer to a list of building IDs
			else {
				int * list = (int *)building_prereq;
				for (int n = 0; n < MAX_BUILDING_PREREQS_FOR_UNIT; n++)
					if ((list[n] >= 0) && ! has_active_building (this, list[n]))
						return 0;
			}
		}
	}

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
	// It's also impossible to disemark units that are being escorted by an immobile unit. I think this is because the movement code will try to
	// move the escorter first. To fix this, break escort relationships with immobile units before disembarking.
	Tile * tile = tile_at (this->Body.X, this->Body.Y);
	if ((is->current_config.patch_disembark_immobile_bug) &&
	    (tile != NULL))
		FOR_UNITS_ON (uti, tile) {
			Unit * escortee = get_unit_ptr (uti.unit->Body.escortee);
			if ((escortee != NULL) &&
			    (uti.unit->Body.Container_Unit == this->Body.ID) &&
			    (escortee->Body.Container_Unit == this->Body.ID) &&
			    (UnitType_has_ability (&p_bic_data->UnitTypes[uti.unit->Body.UnitTypeID], __, UTA_Immobile)))
				Unit_set_escortee (uti.unit, __, -1);
		}
	return Unit_disembark_passengers (this, __, tile_x, tile_y);
}

// Returns whether or not the current deal being offered to the AI on the trade screen would be accepted. How advantageous the AI thinks the
// trade is for itself is stored in out_their_advantage if it's not NULL. This advantage is measured in gold, if it's positive it means the
// AI thinks it's gaining that much value from the trade and if it's negative it thinks it would be losing that much. I don't know what would
// happen if this function were called while the trade screen is not active.
int
is_current_offer_acceptable (int * out_their_advantage)
{
	int their_id = p_diplo_form->other_party_civ_id;

	DiploMessage consideration = Leader_consider_trade (
		&leaders[their_id],
		__,
		&p_diplo_form->our_offer_lists[their_id],
		&p_diplo_form->their_offer_lists[their_id],
		p_main_screen_form->Player_CivID,
		0, 1, 0, 0,
		out_their_advantage,
		NULL, NULL);

	return consideration == DM_AI_ACCEPT;
}

// Adds an offer of gold to the list and returns it. If one already exists in the list, returns a pointer to it.
TradeOffer *
offer_gold (TradeOfferList * list, int is_lump_sum)
{
	if (list->length > 0)
		for (TradeOffer * offer = list->first; offer != NULL; offer = offer->next)
			if ((offer->kind == 7) && (offer->param_1 == is_lump_sum))
				return offer;

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

	// This function gets called from all over the place, check that it's being called to setup the set gold amount popup in the trade screen
	if (is->current_config.autofill_best_gold_amount_when_trading &&
	    (ret_addr == ADDR_SETUP_ASKING_GOLD_RETURN) || (ret_addr == ADDR_SETUP_OFFERING_GOLD_RETURN)) {
		int asking = ret_addr == ADDR_SETUP_ASKING_GOLD_RETURN;
		int is_lump_sum = p_stack[TRADE_GOLD_SETTER_IS_LUMP_SUM_OFFSET]; // Read this variable from the caller's frame

		int their_id = p_diplo_form->other_party_civ_id,
		    our_id = p_main_screen_form->Player_CivID;

		int their_advantage;
		int is_original_acceptable = is_current_offer_acceptable (&their_advantage);

		int best_amount = 0;

		if ((p_diplo_form->their_offer_lists[their_id].length + p_diplo_form->our_offer_lists[their_id].length > 0) && // if anything is on the table AND
		    ((asking && is_original_acceptable) || // (we're asking for money on an acceptable trade OR
		     ((! asking) && (! is_original_acceptable)))) { // we're offering money on an unacceptable trade)

			TradeOfferList * offers = asking ? &p_diplo_form->their_offer_lists[their_id] : &p_diplo_form->our_offer_lists[their_id];
			TradeOffer * test_offer = offer_gold (offers, is_lump_sum);

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

			remove_offer (offers, test_offer);
			test_offer->vtable->destruct (test_offer, __, 1);
		}

		snprintf (is->ask_gold_default, sizeof is->ask_gold_default, "%d", best_amount);
		is->ask_gold_default[(sizeof is->ask_gold_default) - 1] = '\0';
		PopupForm_set_text_key_and_flags (this, __, script_path, text_key, param_3, (int)is->ask_gold_default, param_5, param_6);
	} else
		PopupForm_set_text_key_and_flags (this, __, script_path, text_key, param_3, param_4, param_5, param_6);
}

CityLocValidity __fastcall
patch_Map_check_city_location (Map *this, int edx, int tile_x, int tile_y, int civ_id, byte check_for_city_on_tile)
{
	int adjustment = is->current_config.adjust_minimum_city_separation;
	CityLocValidity base_result = Map_check_city_location (this, __, tile_x, tile_y, civ_id, check_for_city_on_tile);

	// If adjustment is zero, make no change
	if (adjustment == 0)
		return base_result;

	// If adjustment is negative, ignore the CITY_TOO_CLOSE objection to city placement unless the location is next to a city belonging to
	// another civ and the settings forbid founding there.
	else if ((adjustment < 0) && (base_result == CLV_CITY_TOO_CLOSE)) {
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
	} else if ((adjustment > 0) && (base_result == CLV_OK)) {
		// Check tiles around (x, y) for a city. Because the base result is CLV_OK, we don't have to check neighboring tiles, just those at
		// distance 2, 3, ... up to (an including) the adjustment + 1
		for (int dist = 2; dist <= adjustment + 1; dist++) {

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
				neighbor_index_to_displacement (edge_dirs[vert], &dx, &dy);
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

int
is_zero_strength (UnitType * ut)
{
	return (ut->Attack == 0) && (ut->Defence == 0) && (ut->Bombard_Strength == 0) && (ut->Air_Defence == 0);
}

int
is_captured (Unit_Body * u)
{
	return (u->RaceID >= 0) && (u->CivID >= 0) && (u->CivID < 32) && leaders[u->CivID].RaceID != u->RaceID;
}

// Decides whether or not units "a" and "b" are duplicates, i.e., whether they are completely interchangeable.
int
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

int __fastcall
patch_Context_Menu_add_item_and_set_field_18 (Context_Menu * this, int edx, int item_id, char * text, int field_18)
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
	if (is->current_config.group_units_on_right_click_menu &&
	    (0 <= (unit_id = item_id - (0x13 + p_bic_data->UnitTypeCount))) &&
	    (NULL != (unit_body = p_units->Units[unit_id].Unit)) &&
	    (unit_body->UnitTypeID >= 0) && (unit_body->UnitTypeID < p_bic_data->UnitTypeCount)) {

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

	return Context_Menu_add_item_and_set_field_18 (this, __, item_id, text, field_18);
}

int __fastcall
patch_Context_Menu_open (Context_Menu * this, int edx, int x, int y, int param_3)
{
	int * p_stack = (int *)&x;
	int ret_addr = p_stack[-1];

	if (is->current_config.group_units_on_right_click_menu &&
	    (ret_addr == ADDR_OPEN_UNIT_MENU_RETURN)) {

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

int
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
	if (leaders[this->Body.CivID].ContinentStat1[continent_id] == 0) {
		Unit_disband (this);
		return;
	}
	if (any_enemies_near_unit (this, 49)) {
		Unit_set_state (this, __, UnitState_Fleeing);
		byte done = this->vtable->work (this);
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
	if (p_cities->Cities != NULL)
		for (int n = 0; n <= p_cities->LastIndex; n++) {
			City * city = get_city_ptr (n);
			if ((city != NULL) && (city->Body.CivID == this->Body.CivID)) {
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
		}

	// Join city if we're already in the city we want to join, otherwise move to that city. If we couldn't find a city to join, go to the
	// nearest established city and wait.
	City * in_city = get_city_ptr (tile->vtable->m45_Get_City_ID (tile));
	City * moving_to_city = NULL;
	if (best_join_loc != NULL) {
		if ((best_join_loc == in_city) &&
		    Unit_can_perform_command (this, __, UCV_Join_City)) {
			Unit_join_city (this, __, in_city);
			return;
		} else
			moving_to_city = best_join_loc;
	} else if (in_city == NULL)
		moving_to_city = find_nearest_established_city (this, continent_id);

	if (moving_to_city) {
		int first_move = Trade_Net_set_unit_path (p_trade_net, __, this->Body.X, this->Body.Y, moving_to_city->Body.X, moving_to_city->Body.Y, this, this->Body.CivID, 0x101, NULL);
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
	int reduction = is->current_config.anarchy_length_reduction_percent;
	if (reduction > 0) {
		// Anarchy cannot be less than 2 turns or you'll get an instant government switch. Only let that happen when the reduction is set
		// above 100%, indicating a desire for no anarchy. Otherwise we can't reduce anarchy below 2 turns.
		if (reduction > 100)
			return 1;
		else if (base <= 2)
			return base;
		else
			return not_below (2, rand_div (base * (100 - reduction), 100));
	} else
		return base;
}

byte __fastcall
patch_Unit_check_king_ability_while_spawning (Unit * this, int edx, enum UnitTypeAbilities a)
{
	if (is->current_config.dont_give_king_names_in_non_regicide_games &&
	    ((*p_toggleable_rules & (TR_REGICIDE | TR_MASS_REGICIDE)) == 0))
		return 0;
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
byte __fastcall
patch_Improvement_has_wonder_com_bonus_for_ai_prod (Improvement * this, int edx, enum ImprovementTypeWonderFeatures flag)
{
	is->ai_considering_order.OrderID = this - p_bic_data->Improvements;
	is->ai_considering_order.OrderType = COT_Improvement;
	return Improvement_has_wonder_flag (this, __, flag);
}

// Similarly, this one sets the var when looping over unit types.
byte __fastcall
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
			snprintf (s, sizeof s, "^%d   %s", val->point_value, name);
			s[(sizeof s) - 1] = '\0';
			PopupForm_add_text (popup, __, s, 0);
		}
		show_popup (popup, __, 0, 0);
	}

	City_Form_m82_handle_key_event (this, __, virtual_key_code, is_down);
}

int
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

int
is_explored (Tile * tile, int civ_id)
{
	int in_debug_mode = (*p_debug_mode_bits & 8) != 0, // checking bit 3 here b/c that's how resource visibility is checked in open_tile_info
	    not_fogged = (tile->Body.Fog_Of_War & (1 << civ_id)) != 0;
	return in_debug_mode || not_fogged;
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
			int eval = ai_eval_city_location (is->viewing_tile_info_x, is->viewing_tile_info_y, is->city_loc_display_perspective, 0, NULL);
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
patch_City_compute_corrupted_yield (City * this, int edx, int gross_yield, byte is_production)
{
	if (is->current_config.zero_corruption_when_off) {
		Government * govt = &p_bic_data->Governments[leaders[this->Body.CivID].GovernmentType];
		if (govt->CorruptionAndWaste == CWT_Off)
			return 0;
	}

	return City_compute_corrupted_yield (this, __, gross_yield, is_production);
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
			int eval = ai_eval_city_location (tile_x, tile_y, is->city_loc_display_perspective, 0, NULL);
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
	    (! is_command_button_active (&this->GUI, UCV_Load)) &&
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
		int sel = show_popup (popup, __, 0, 0);
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

	snprintf (s, sizeof s, "%s: %d%c", is->c3x_labels[CL_VERSION], MOD_VERSION/100, MOD_VERSION%100 != 0 ? version_letter : ' ');
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

	show_popup (popup, __, 0, 0);
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

byte __fastcall
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
	byte tr = City_cycle_specialist_type (this, __, mouse_x, mouse_y, citizen, city_form);

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

void __fastcall
patch_City_add_or_remove_improvement (City * this, int edx, int improv_id, int add, byte param_3)
{
	// The enable_negative_pop_pollution feature changes the rules so that improvements flagged as removing pop pollution and having a negative
	// pollution amount contribute to the city's pop pollution instead of building pollution. Here we make sure that such improvements do not
	// contribute to building pollution by temporarily zeroing out their pollution stat when they're added to or removed from a city.
	Improvement * improv = &p_bic_data->Improvements[improv_id];
	if (is->current_config.enable_negative_pop_pollution &&
	    (improv->ImprovementFlags & ITF_Removes_Population_Pollution) &&
	    (improv->Pollution < 0)) {
		int saved_pollution_amount = improv->Pollution;
		improv->Pollution = 0;
		City_add_or_remove_improvement (this, __, improv_id, add, param_3);
		improv->Pollution = saved_pollution_amount;
	} else
		City_add_or_remove_improvement (this, __, improv_id, add, param_3);

	// Recompute available resources if this improvement has the ability to generate a traded resource. Don't bother if the improvement also
	// enables trade b/c then the recomputation would have already been done in the base game's add_or_remove_improvement method.
	if (((improv->ImprovementFlags & ITF_Allows_Water_Trade) == 0) &&
	    ((improv->ImprovementFlags & ITF_Allows_Air_Trade)   == 0) &&
	    ((improv->WonderFlags      & ITW_Safe_Sea_Travel)    == 0)) {
		for (int n = 0; n < is->current_config.count_mills; n++)
			if ((improv_id == is->current_config.mills[n].improv_id) &&
			    (! is->current_config.mills[n].is_local)) {
				patch_Trade_Net_recompute_resources (p_trade_net, __, 0);
				break;
			}
	}
}

void __fastcall
patch_Fighter_begin (Fighter * this, int edx, Unit * attacker, int attack_direction, Unit * defender)
{
	Fighter_begin (this, __, attacker, attack_direction, defender);

	// Apply override of retreat eligibility
	// Must use this->defender instead of the defender argument since the argument is often NULL, in which case Fighter_begin finds a defender on
	// the target tile and stores it in this->defender. Also must check that against NULL since Fighter_begin might fail to find a defender.
	enum retreat_rules retreat_rules = is->current_config.retreat_rules;
	if ((retreat_rules != RR_STANDARD) && (this->defender != NULL)) {
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

// TCC requires a main function be defined even though it's never used.
int main () { return 0; }
