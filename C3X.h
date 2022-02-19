
#define NOVIRTUALKEYCODES // Keycodes defined in Civ3Conquests.h instead
#include "windows.h"

typedef unsigned char byte;

// Use fastcall as substitute for thiscall because TCC doesn't recognize thiscall
#define __fastcall __attribute__((fastcall))
#include "Civ3Conquests.h"

#define MOD_VERSION 901

#define COUNT_TILE_HIGHLIGHTS 11

struct perfume_config_spec {
	char * target_name;
	int amount;
};

struct perfume_internal_spec {
	City_Order target_order;
	int amount;
};

struct c3x_config {
	char enable_stack_bombard;
	char enable_disorder_warning;
	char allow_stealth_attack_against_single_unit;
	char show_detailed_city_production_info;
	int limit_railroad_movement;
	char enable_free_buildings_from_small_wonders;
	char enable_stack_unit_commands;
	char skip_repeated_tile_improv_replacement_asks;
	char autofill_best_gold_amount_when_trading;
	int adjust_minimum_city_separation;
	char disallow_founding_next_to_foreign_city;
	char enable_trade_screen_scroll;
	char group_units_on_right_click_menu;
	int anarchy_length_reduction_percent;
	char show_golden_age_turns_remaining;
	char dont_give_king_names_in_non_regicide_games;
	char disable_worker_automation;
	char enable_land_sea_intersections;
	char disallow_trespassing;
	char show_detailed_tile_info;
	struct perfume_config_spec * perfume_specs;
	int count_perfume_specs;
	char warn_about_unrecognized_perfume_target;
	char enable_ai_production_ranking;
	char enable_ai_city_location_desirability_display;
	char zero_corruption_when_off;
	char disallow_land_units_from_settling_water;
	char dont_end_units_turn_after_airdrop;

	char use_offensive_artillery_ai;
	int ai_build_artillery_ratio;
	int ai_artillery_value_damage_percent;
	int ai_build_bomber_ratio;
	char replace_leader_unit_ai;
	char fix_ai_army_composition;
	char enable_pop_unit_ai;

	char remove_unit_limit;
	char remove_era_limit;

	char patch_submarine_bug;
	char patch_science_age_bug;
	char patch_pedia_texture_bug;
	char patch_disembark_immobile_bug;
	char patch_houseboat_bug;
	char patch_intercept_lost_turn_bug;
	char patch_phantom_resource_bug;

	char prevent_autorazing;
	char prevent_razing_by_ai_players;
};

enum stackable_command {
	SC_BOMBARD = 0,
	SC_BOMB,
	SC_FORTRESS,
	SC_MINE,
	SC_IRRIGATE,
	SC_CHOP_FOREST,
	SC_CHOP_JUNGLE,
	SC_PLANT,
	SC_CLEAN_POLLUTION,
	SC_ROAD,
	SC_RAILROAD,
	SC_FORTIFY,
	SC_UPGRADE,
	SC_DISBAND,
	COUNT_STACKABLE_COMMANDS
};

enum stackable_command_kind {
	SCK_BOMBARD = 0,
	SCK_TERRAFORM,
	SCK_UNIT_MGMT,
	COUNT_STACKABLE_COMMAND_KINDS
};

struct sc_button_info {
	enum Unit_Command_Values command;
	enum stackable_command_kind kind;
	int tile_sheet_column,
	    tile_sheet_row;
} const sc_button_infos[COUNT_STACKABLE_COMMANDS] = {
	/* Bombard */    { .command = UCV_Bombard        , .kind = SCK_BOMBARD  , .tile_sheet_column = 3, .tile_sheet_row = 1 },
	/* Bomb */       { .command = UCV_Bombing        , .kind = SCK_BOMBARD  , .tile_sheet_column = 5, .tile_sheet_row = 4 },
	/* Fortress */   { .command = UCV_Build_Fortress , .kind = SCK_TERRAFORM, .tile_sheet_column = 0, .tile_sheet_row = 3 },
	/* Mine */       { .command = UCV_Build_Mine     , .kind = SCK_TERRAFORM, .tile_sheet_column = 1, .tile_sheet_row = 3 },
	/* Irrigate */   { .command = UCV_Irrigate       , .kind = SCK_TERRAFORM, .tile_sheet_column = 2, .tile_sheet_row = 3 },
	/* Chop For. */  { .command = UCV_Clear_Forest   , .kind = SCK_TERRAFORM, .tile_sheet_column = 3, .tile_sheet_row = 3 },
	/* Chop Jun. */  { .command = UCV_Clear_Jungle   , .kind = SCK_TERRAFORM, .tile_sheet_column = 4, .tile_sheet_row = 3 },
	/* Plant */      { .command = UCV_Plant_Forest   , .kind = SCK_TERRAFORM, .tile_sheet_column = 5, .tile_sheet_row = 3 },
	/* Clean Pol. */ { .command = UCV_Clear_Pollution, .kind = SCK_TERRAFORM, .tile_sheet_column = 6, .tile_sheet_row = 3 },
	/* Road */       { .command = UCV_Build_Road     , .kind = SCK_TERRAFORM, .tile_sheet_column = 6, .tile_sheet_row = 2 },
	/* Railroad */   { .command = UCV_Build_Railroad , .kind = SCK_TERRAFORM, .tile_sheet_column = 7, .tile_sheet_row = 2 },
	/* Fortify */    { .command = UCV_Fortify        , .kind = SCK_UNIT_MGMT, .tile_sheet_column = 2, .tile_sheet_row = 0 },
	/* Upgrade */    { .command = UCV_Upgrade_Unit   , .kind = SCK_UNIT_MGMT, .tile_sheet_column = 7, .tile_sheet_row = 1 },
	/* Disband */    { .command = UCV_Disband        , .kind = SCK_UNIT_MGMT, .tile_sheet_column = 3, .tile_sheet_row = 0 },
};

enum init_state {
	IS_UNINITED = 0,
	IS_OK,
	IS_INIT_FAILED
};

enum c3x_label {
	CL_NEVER_COMPLETES = 0,
	CL_HALTED,
	CL_SURPLUS,
	CL_SURPLUS_NONE,
	CL_SURPLUS_NA,
	CL_SB_TOOLTIP,
	CL_CHOPPED,
	CL_OFF,
	CL_MOD_INFO_BUTTON_TEXT,
	CL_VERSION,
	CL_CONFIG_FILES_LOADED,
	COUNT_C3X_LABELS
};

struct worker_job_and_location {
	enum Worker_Jobs job;
	int tile_x, tile_y;
};

struct ai_prod_valuation {
	int order_type;
	int order_id;
	int point_value;
};

struct injected_state {
	// ==========
	// These fields are valid at any time in the injected code because they're set by the patcher {
	// ==========

	int mod_version;
	// "mod relative directory" is the mod dir relative to the conquests dir. Usually it's "C3X_Rn" but it might be deeper.
	// It must be non-empty and must not have an ending backslash.
	char mod_rel_dir[MAX_PATH];

	enum init_state sc_img_state;
	enum init_state tile_highlight_state;
	enum init_state mod_info_button_images_state;

	struct c3x_config base_config;

	// ==========
	// } These fields are valid at any time after patch_init_floating_point runs (which is at the program launch). {
	// ==========

	// Windows modules
	HMODULE kernel32;
	HMODULE user32;
	HMODULE msvcrt;

	// Win32 API functions
	WINBOOL (WINAPI * VirtualProtect) (LPVOID, SIZE_T, DWORD, PDWORD);
	WINBOOL (WINAPI * CloseHandle) (HANDLE);
	HANDLE (WINAPI * CreateFileA) (LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
	DWORD (WINAPI * GetFileSize) (HANDLE, LPDWORD);
	WINBOOL (WINAPI * ReadFile) (HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);

	// Win32 funcs from user32.dll
	int (WINAPI * MessageBoxA) (HWND, LPCSTR, LPCSTR, UINT);

	// C standard library functions
	int (* snprintf) (char *, size_t, char const *, ...);
	void * (* malloc) (size_t);
	void (* free) (void *);
	long (* strtol) (char const *, char **, int);
	int (* strncmp) (char const *, char const *, size_t);
	size_t (* strlen) (char const *);
	char * (* strncpy) (char *, char const *, size_t);
	char * (* strdup) (char const *);
	char * (* strstr) (char const *, char const *);
	void (* qsort) (void *, size_t, size_t, int (*) (void const *, void const *));
	int (* memcmp) (void const *, void const *, size_t);
	void * (* memcpy) (void *, void const *, size_t);

	struct c3x_config current_config;

	// Keeps a record of all configs currently loaded. Useful to know. "name" is the file name for configs that come from files, which is all of
	// them except for the base config, whose name is "(base)".
	struct loaded_config_name {
		char * name;
		struct loaded_config_name * next;
	} * loaded_config_names;

	char mod_script_path[MAX_PATH];

	char * c3x_labels[COUNT_C3X_LABELS];

	int have_job_and_loc_to_skip; // 0 or 1 if the variable below has anything actionable in it. Gets cleared to 0 after
	// every turn.
	struct worker_job_and_location to_skip;

	byte houseboat_patch_area_original_contents[50];

	int * unit_menu_duplicates; // NULL initialized, allocated to an array of 0x100 ints when needed

	// List of temporary ints. Initializes to NULL/0/0, used with functions "memoize" and "clear_memo"
	int * memo;
	int memo_len;
	int memo_capacity;

	// Initialized to NULL/0, then filled out by load_scenario
	struct perfume_internal_spec * perfume_specs;
	int count_perfume_specs;

	// The civ ID of the player from whose perspective we're currently showing city loc desirability, or -1 if none. Initialized to -1.
	int city_loc_display_perspective;

	// Stores resource access bits for resources beyond the first 32, used to fix the phantom resource bug. Initialized to NULL/0 and allocated as
	// necessary by get_extra_resource_bits. Contains an array of groups of unsigned ints. There is one group per city (allocated lazily so you
	// must use the getter) and the number of ints in each group depends on the number of resources defined in the scenario, one for every 32
	// after the first 32.
	unsigned * extra_available_resources;
	int extra_available_resources_capacity; // In number of cities.

	// ==========
	// } These fields are valid only after init_stackable_command_buttons has been called. {
	// ==========

	PCX_Image sc_button_sheets[4];

	struct sc_button_image_set {
		Tile_Image_Info imgs[4];
	} sc_button_image_sets[COUNT_STACKABLE_COMMANDS];

	int sb_activated_by_button; // Gets set to 1 when the player clicks on the SB button so that perform_action_on_tile knows
	// to do stack instead of regular bombard. This is necessary since the SB button actually just activates regular bombard.
	// The proper way to implement the SB button would be to give it its own mode action but that's difficult to do because
	// the existing mode actions are baked into the code. Implementing it this way I estimate is less fragile and requires
	// less patching. The hard part is knowing when to clear this flag so that the player doesn't activate SB, cancel it, then
	// activate regular bombard and get SB instead. One trick to make this easier is to look for the change of cursor away
	// from the little bomb indicator instead of trying to intercept every relevant UI event. This flag is changed in these
	// circumstances:
	//   (1) At init, the flag is cleared, of course.
	//   (2) Pressing the SB button sets the flag and pressing another unit command button clears it.
	//   (3) If handle_cursor_change_in_jgl is called and the main_screen_form's Mode_Action isn't (air) bombard, the flag
	//       is cleared. That covers every case of switching off SB mode except one, when the player presses a key to switch
	//       off of SB to regular bombard, that's what case (4) is for.
	//   (4) The flag is cleared when the player presses the B key or (if the unit is capable of precision strikes) the P key.
	// One final complication, that I only discovered after trying to implement this, is that when clicking on the map with a
	// mode action, the cursor is cleared before the action is carried out. So we have to intercept that map click as well for
	// a total of 4 UI functions patched to make this damn button work. I doubt this is optimal but it works and I've wasted
	// enough time on this already. That click interceptor sets a flag value of 2 to indicate this annoying state.

	// ==========
	// } These fields are valid only after init_tile_highlights as been called. {
	// ==========

	PCX_Image tile_highlight_sheet;
	Tile_Image_Info tile_highlights[COUNT_TILE_HIGHLIGHTS];

	// ==========
	// } This one is valid only if init_mod_info_button_images has been called and mod_info_button_images_state equals IS_OK {
	// ==========

	Tile_Image_Info mod_info_button_images[3];

	// ==========
	// } These fields are temporary/situational {
	// ==========

	int saved_road_movement_rate; // Valid when railroad movement limit is applied (limit_railroad_movement > 0) and BIC
	// data has been loaded

	Leader * leader_param_for_patch_get_wonder_city_id; // Valid in patch_get_wonder_city_id when called from
	// Leader_recompute_auto_improvements

	int show_popup_was_called; // Set to 1 in show_popup. Used in patch_Leader_can_do_worker_job to check if the replacement
	// popup was shown.

	// Used to control trade screen scroll
	int open_diplo_form_straight_to_trade; // Initialized to 0, gets set to 1 by patch_DiploForm_do_diplomacy to signal
	// to patch_DiploForm_m68_Show_Dialog to open the diplo form straight into trade mode.
	int trade_screen_scroll_to_id; // Set by patch_DiploForm_m82_handle_key_event to signal to do_diplomacy that the form
	// was closed in order to scroll to the civ with the set ID. -1 indicates no scrolling.
	Button * trade_scroll_button_left; // initialized to NULL
	Button * trade_scroll_button_right; // initialized to NULL
	enum init_state trade_scroll_button_state;
	int eligible_for_trade_scroll;

	char ask_gold_default[32];

	// Set in patch_ai_choose_production, used by the various bits of injected code that run during the AI production choosing process.
	City * ai_considering_production_for_city;

	// This variable stores what improvement or unit the AI is evaluating inside ai_choose_production. It gets set inside two call replacements,
	// first inside the loop over improvements then again inside a loop over unit types. The var is used by the intercept consideration functions
	// which run at the end of each loop iteration.
	City_Order ai_considering_order;

	// Used in the code that adds additional info to the tile info box
	int viewing_tile_info_x, viewing_tile_info_y;

	// Stores a list of the production options in a given city and the point value the AI would assign to each. The list is populated by
	// rank_ai_production_options, items are added by record_improv_val which gets called by some code injected into one of the loops in
	// ai_choose_production. These vars are initialized to zero.
	struct ai_prod_valuation * ai_prod_valuations;
	int count_ai_prod_valuations;
	int ai_prod_valuations_capacity;

	// ==========
	// }
	// ==========
};

enum object_job {
	OJ_DEFINE = 0,
	OJ_INLEAD, // Patch this function with an inlead
	OJ_REPL_VPTR, // Patch this function by replacing a pointer to it. The address column is the addr of the VPTR not the function itself.
	OJ_REPL_CALL, // Patch a single function call. The address column is the addr of the call instruction, name refers to the new target function, type is not used.
	OJ_IGNORE
};

struct civ_prog_object {
	enum object_job job;
	int gog_addr;
	int steam_addr;
	char const * name;
	char const * type;
};
