
#include <stdbool.h>

#define NOVIRTUALKEYCODES // Keycodes defined in Civ3Conquests.h instead
#include "windows.h"

typedef unsigned char byte;

// Use fastcall as substitute for thiscall because TCC doesn't recognize thiscall
#define __fastcall __attribute__((fastcall))
#include "Civ3Conquests.h"

#define MOD_VERSION 1700
#define MOD_PREVIEW_VERSION 1

#define COUNT_TILE_HIGHLIGHTS 11
#define MAX_BUILDING_PREREQS_FOR_UNIT 10

// Initialize to zero. Implementation is in common.c
struct table {
	void * block;
	size_t capacity_exponent; // Actual capacity is 1 << capacity_exponent
	size_t len;
};

struct perfume_spec {
	City_Order target_order;
	int amount;
};

// A mill is a city improvement that spawns a resource. These are read from the "buildings_generating_resources" key in the config but are called
// "mills" internally for brevity.
struct mill {
	int improv_id;
	int resource_id;
	bool is_local, no_tech_req;
};

enum retreat_rules {
	RR_STANDARD = 0,
	RR_NONE,
	RR_ALL_UNITS,
	RR_IF_FASTER
};

enum special_defensive_bombard_rules {
	SDBR_LETHAL         =  1,
	SDBR_NOT_INVISIBLE  =  2,
	SDBR_AERIAL         =  4,
	SDBR_BLITZ          =  8,
	SDBR_DOCKED_VS_LAND = 16,
};

enum special_zone_of_control_rules {
	SZOCR_LETHAL     = 1,
	SZOCR_AERIAL     = 2,
	SZOCR_AMPHIBIOUS = 4,
};

struct c3x_config {
	bool enable_stack_bombard;
	bool enable_disorder_warning;
	bool allow_stealth_attack_against_single_unit;
	bool show_detailed_city_production_info;
	int limit_railroad_movement;
	bool enable_free_buildings_from_small_wonders;
	bool enable_stack_unit_commands;
	bool skip_repeated_tile_improv_replacement_asks;
	bool autofill_best_gold_amount_when_trading;
	int minimum_city_separation;
	bool disallow_founding_next_to_foreign_city;
	bool enable_trade_screen_scroll;
	bool group_units_on_right_click_menu;
	int anarchy_length_percent;
	bool show_golden_age_turns_remaining;
	bool show_zoc_attacks_from_mid_stack;
	bool cut_research_spending_to_avoid_bankruptcy;
	bool dont_pause_for_love_the_king_messages;
	bool reverse_specialist_order_with_shift;
	bool dont_give_king_names_in_non_regicide_games;
	bool no_elvis_easter_egg;
	bool disable_worker_automation;
	bool enable_land_sea_intersections;
	bool disallow_trespassing;
	bool show_detailed_tile_info;
	struct perfume_spec * perfume_specs;
	int count_perfume_specs;
	struct table building_unit_prereqs; // A mapping from int keys to int values. The keys are unit type IDs. If an ID is present as a key in the
					    // table that means that unit type has one or more prereq buildings. The associated value is either a
					    // pointer to a list of MAX_BUILDING_PREREQS_FOR_UNITS improvement IDs or a single encoded improv ID. The
					    // contents of the list are NOT encoded and unused slots store -1. Encoding an ID is done by left-shifting
					    // it one place then inserting a 1 in the LSB. This way encoded IDs can be told apart from list pointers
					    // by checking the LSB (1 => encoded improv ID, 0 => list pointer).
	struct mill * mills;
	int count_mills;
	bool warn_about_unrecognized_names;
	bool enable_ai_production_ranking;
	bool enable_ai_city_location_desirability_display;
	bool zero_corruption_when_off;
	bool disallow_land_units_from_affecting_water_tiles;
	bool dont_end_units_turn_after_airdrop;
	bool enable_negative_pop_pollution;
	enum retreat_rules land_retreat_rules;
	enum retreat_rules sea_retreat_rules;
	bool enable_ai_two_city_start;
	int max_tries_to_place_fp_city;
	bool promote_forbidden_palace_decorruption;
	bool allow_military_leaders_to_hurry_wonders;
	bool halve_ai_research_rate;
	bool aggressively_penalize_bankruptcy;
	bool no_penalty_exception_for_agri_fresh_water_city_tiles;
	bool suppress_hypertext_links_exceeded_popup;
	bool indicate_non_upgradability_in_pedia;
	bool show_message_after_dodging_sam;
	bool include_stealth_attack_cancel_option;
	bool intercept_recon_missions;
	bool charge_one_move_for_recon_and_interception;
	bool polish_non_air_precision_striking;
	bool enable_stealth_attack_via_bombardment;
	bool immunize_aircraft_against_bombardment;
	bool replay_ai_moves_in_hotseat_games;
	int count_ptw_arty_types;
	int ptw_arty_types_capacity;
	int * ptw_arty_types; // List of unit type IDs
	bool restore_unit_directions_on_game_load;
	bool charm_flag_triggers_ptw_like_targeting;
	bool city_icons_show_unit_effects_not_trade;
	bool ignore_king_ability_for_defense_priority;
	bool show_untradable_techs_on_trade_screen;
	bool optimize_improvement_loops;
	bool disallow_useless_bombard_vs_airfields;
	bool enable_city_capture_by_barbarians;
	bool share_visibility_in_hoseat;
	bool allow_precision_strikes_against_tile_improvements;
	bool dont_end_units_turn_after_bombarding_barricade;
	bool remove_land_artillery_target_restrictions;
	bool allow_bombard_of_other_improvs_on_occupied_airfield;
	bool show_total_city_count;
	bool strengthen_forbidden_palace_ocn_effect;
	enum special_zone_of_control_rules special_zone_of_control_rules;
	enum special_defensive_bombard_rules special_defensive_bombard_rules;

	bool use_offensive_artillery_ai;
	int ai_build_artillery_ratio;
	int ai_artillery_value_damage_percent;
	int ai_build_bomber_ratio;
	bool replace_leader_unit_ai;
	bool fix_ai_army_composition;
	bool enable_pop_unit_ai;
	int max_ai_naval_escorts;

	bool remove_unit_limit;
	bool remove_era_limit;
	bool remove_cap_on_turn_limit;

	bool patch_submarine_bug;
	bool patch_science_age_bug;
	bool patch_pedia_texture_bug;
	bool patch_disembark_immobile_bug;
	bool patch_houseboat_bug;
	bool patch_intercept_lost_turn_bug;
	bool patch_phantom_resource_bug;
	bool patch_maintenance_persisting_for_obsolete_buildings;
	bool patch_barbarian_diagonal_bug;

	bool prevent_autorazing;
	bool prevent_razing_by_players;
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
	CL_CREATING_CITIES,
	CL_2CS_FAILED_SANITY_CHECK,
	CL_2CS_ADJACENT_CITIES,
	CL_2CS_MISSING_CITIES,
	CL_OBSOLETED_BY,
	CL_NO_STEALTH_ATTACK,
	CL_DODGED_SAM,
	CL_PREVIEW,
	CL_CITY_TOO_CLOSE_BUTTON_TOOLTIP,
	CL_TOTAL_CITIES,

	// Offense, Defense, Artillery, etc.
	CL_FIRST_UNIT_STRAT,
	CL_LAST_UNIT_STRAT = CL_FIRST_UNIT_STRAT + 19,

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
	enum init_state disabled_command_img_state;

	// ==========
	// } These fields are valid at any time after patch_init_floating_point runs (which is at the program launch). {
	// ==========

	struct c3x_config base_config;

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
	HMODULE (WINAPI * LoadLibraryA) (LPCSTR);
	int (WINAPI * MultiByteToWideChar) (UINT, DWORD, LPCCH, int, LPWSTR, int);
	int (WINAPI * WideCharToMultiByte) (UINT, DWORD, LPCWCH, int, LPSTR, int, LPCCH, LPBOOL);
	int (WINAPI * GetLastError) ();
	void (WINAPI * GetLocalTime) (LPSYSTEMTIME);

	// Win32 funcs from user32.dll
	int (WINAPI * MessageBoxA) (HWND, LPCSTR, LPCSTR, UINT);

	// C standard library functions
	int (* snprintf) (char *, size_t, char const *, ...);
	void * (* malloc) (size_t);
	void * (* calloc) (size_t, size_t);
	void * (* realloc) (void *, size_t);
	void (* free) (void *);
	long (* strtol) (char const *, char **, int);
	int (* strcmp) (char const *, char const *);
	int (* strncmp) (char const *, char const *, size_t);
	size_t (* strlen) (char const *);
	char * (* strncpy) (char *, char const *, size_t);
	char * (* strcpy) (char *, char const *);
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

	int have_job_and_loc_to_skip; // 0 or 1 if the variable below has anything actionable in it. Gets cleared to 0 after every turn.
	struct worker_job_and_location to_skip;

	struct table nopified_areas;

	int * unit_menu_duplicates; // NULL initialized, allocated to an array of 0x100 ints when needed

	// List of temporary ints. Initializes to NULL/0/0, used with functions "memoize" and "clear_memo"
	int * memo;
	int memo_len;
	int memo_capacity;

	// The civ ID of the player from whose perspective we're currently showing city loc desirability, or -1 if none. Initialized to -1.
	int city_loc_display_perspective;

	// Stores resource access bits for resources beyond the first 32, used to fix the phantom resource bug. Initialized to NULL/0 and allocated as
	// necessary by get_extra_resource_bits. Contains an array of groups of unsigned ints. There is one group per city (allocated lazily so you
	// must use the getter) and the number of ints in each group depends on the number of resources defined in the scenario, one for every 32
	// after the first 32.
	unsigned * extra_available_resources;
	int extra_available_resources_capacity; // In number of cities.

	// These lists store interception events per-player during the interturn so we can clear the interception state on fighters that have done so
	// at the beginning of their next turn. The point of this is to imitate the base game behavior where fighters that perform an interception are
	// knocked out of the interception state. We can't knock them out immediately b/c that will prevent them from doing multiple interceptions per
	// turn, so instead we record the event in these lists and reset their state later.
	struct interceptor_reset_list {
		struct interception {
			int unit_id;
			int x, y;
		} * items;
		int count;
		int capacity;
	} interceptor_reset_lists[32];

	// Stores the byte offsets into the c3x_config struct of all boolean/integer config options, accessible using the options' names as
	// strings. Used when reading in a config INI file.
	struct table boolean_config_offsets;
	struct table integer_config_offsets;

	// Maps unit types IDs to AI strategy indices (0 = offense, 1 = defense, 2 = artillery, etc.). If a unit type ID is in this table, that means
	// it's one of several duplicate types created to spread multiple AI strategies out so each type has only one.
	struct table unit_type_alt_strategies;

	// Tracks the number of "extra" defensive bombards units have performed, by their IDs. If the "blitz" special defensive bombard rule is
	// activated, units with blitz get an extra chance to perform DB for each movement point they have beyond the first.
	struct table extra_defensive_bombards;

	// ==========
	// } These fields are valid only after init_stackable_command_buttons has been called. {
	// ==========

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
	// } This field is only valid after init_disabled_command_buttons has been called and disabled_command_img_state equals IS_OK {
	// ==========

	Tile_Image_Info disabled_build_city_button_img;

	// ==========
	// } These fields are valid only after init_tile_highlights as been called. {
	// ==========

	Tile_Image_Info tile_highlights[COUNT_TILE_HIGHLIGHTS];

	// ==========
	// } This one is valid only if init_mod_info_button_images has been called and mod_info_button_images_state equals IS_OK {
	// ==========

	Tile_Image_Info mod_info_button_images[3];

	// ==========
	// } These fields are temporary/situational {
	// ==========

	int saved_road_movement_rate; // Valid when railroad movement limit is applied (limit_railroad_movement > 0) and BIC data has been loaded
	int road_mp_cost; // The cost of moving one tile along a road, in MP. Valid after BIC data was loaded.
	int railroad_mp_cost_per_move; // The cost of moving one tile along a railroad, in MP, per move available to the unit. This is measured per
				       // move since the cost of moving along a railroad is scaled by the total number of moves available to the
				       // unit. Valid after BIC data was loaded.

	int saved_barb_culture_group; // Valid when barb city capturing is enabled and BIC data has been loaded

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
	Tile_Image_Info * trade_scroll_button_images; // inited to NULL, array of 6 images: normal, rollover, and highlight for left & right
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

	// Used for generating resources from buildings
	struct mill_tile {
		Tile * tile;
		City * city;
		struct mill * mill;
	} * mill_tiles;
	int count_mill_tiles;
	int mill_tiles_capacity;
	struct mill_tile * got_mill_tile;
	int saved_tile_count; // Stores the actual tile count in case p_bic_data->Map.TileCount was temporarily overwritten. Set to -1 when empty.

	// Stores the trade offer object being modified when the user right-clicks on a gold offer/ask on the trade table. Gets set by a special
	// function call replacement (see apply_machine_code_edits for details).
	TradeOffer * modifying_gold_trade;

	// Initialized to NULL and reset to NULL after Unit::bombard_tile returns. Gets set just before the call to Unit::play_bombard_fire_animation
	// from inside bombard_tile. The value is the unit on the bombarded tile specifically targeted by the attack, if applicable.
	Unit * bombard_stealth_target;

	// Set to zero when Unit::select_stealth_attack_target is called then checked inside in the call to PopupSelection::add_item and set to
	// 1. This variable lets the add_item replacement function run special code for only the first item added, in order to insert an additional
	// first option to cancel the stealth attack.
	int added_any_stealth_target;

	// Initialized to zero. Temporarily set to 1 across a call to patch_Unit_select_stealth_attack_target to request that the selected target must
	// be suitable for attack via bombardment.
	int selecting_stealth_target_for_bombard;

	// Set in rand_int_to_dodge_city_aa and read immediately after in Unit_get_defense_to_dodge_city_aa. Allows us to determine whether the dodge
	// roll was successful in order to popup the message.
	int result_of_roll_to_dodge_city_aa;

	// Initialized to NULL. If set to non-NULL, the next call to show_map_message will consume that value and use it as the text on the
	// message. Used to implement show_map_specific_text.
	char const * map_message_text_override;

	// Initialized to NULL. If set to non-NULL, the next call to do_load_game will consume this value and use it as the file path of the game to
	// load instead of opening the file picker.
	char const * load_file_path_override;

	// A set of player bits indicating which players should see a replay of the AI's moves. Normally all zero, gets filled in by
	// patch_perform_interturn_in_main_loop only when/as appropriate. As replays are played (in patch_show_movement_phase_popup), the
	// corresponding bits are cleared.
	int replay_for_players;

	// Used by patch_perform_interturn_in_main_loop to determine which players need to see the replay. Clear before interturn processing, then
	// every time an AI unit moves onto or bombards a tile, any players that have vision on that tile will have their bit set in this var.
	int players_saw_ai_unit;

	// Initialized to 0. If set to non-zero, the next call to do_load_game will consume the value and skip the intro popup.
	int suppress_intro_after_load_popup;

	struct improv_id_list {
		int * items;
		int count;
		int capacity;
	} water_trade_improvs, air_trade_improvs, combat_defense_improvs;

	// Used by the fix for the barbarian diagonal bug
	int barb_diag_patch_dy_fix;

	// Initialized to 0. If 1, barbarian activity is force activated b/c there are barb cities on the map that need to do production.
	int force_barb_activity_for_cities;

	// Used as a stand-in for an actual tile where needed. In particular, this object is returned from various get_tile and tile_at replacements
	// when we need to override the visibility data to implement hotseat shared vis.
	Tile * dummy_tile;

	// When the game checks visibility for a tile, it accesses all four visibility fields with separate calls to Map::get_tile and/or tile_at. We
	// replace the first call, maybe altering its return to implement shared visibility, cache the return in this variable, then re-use the cached
	// value for the next three calls.
	Tile * tile_returned_for_visibility_check;

	// Initialized to all -1. If set, the unit with the specified ID will always be the top unit displayed on the specified tile. If the unit is
	// not on that tile, there is no effect. This is only intended to be used on a temporary basis.
	struct unit_display_override {
		int unit_id, tile_x, tile_y;
	} unit_display_override;

	// Used to extract which unit (if any) exerted zone of control from within Fighter::apply_zone_of_control.
	Unit * zoc_interceptor;

	// Set when Fighter::apply_zone_of_control is called to store the defending unit, used by the injected filter.
	Unit * zoc_defender;

	// Normally set to NULL. When a unit bombards a tile (the tile itself, not something on it), set to point to that unit during the call to
	// Unit::attack_tile. Used to stop the unit from losing all of its movement if configured.
	Unit * unit_bombard_attacking_tile;

	// Cleared to zero when Fighter::apply_zone_of_control is called. The interceptor must be unfortified to ensure it plays its animation. If
	// that happens, this flag is set so that apply_zone_of_control knows to refortify the unit after the ZoC process is done.
	int refortify_interceptor_after_zoc;

	// Used to record info about a defensive bomardment event during Fighter::fight. Gets set by Fighter::damage_by_defensive_bombardment and
	// cleared when Fighter::fight returns.
	struct defensive_bombard_event {
		Unit * bombarder;
		Unit * defender;
		bool damage_done, defender_was_destroyed, saved_animation_setting;
	} dbe;

	// Set to 1 IFF we're showing a replay of AI moves in hotseat mode
	bool showing_hotseat_replay;

	// Set to 1 only during the first call to get_tile_occupier_id from Trade_Net::get_movement_cost. While this is set, we need to edit unit
	// visibility to patch the submarine bug.
	bool getting_tile_occupier_for_ai_pathfinding;

	// gdi_plus.init_state is valid any time after patch_init_floating_point. All the other fields are only valid after set_up_gdi_plus has been
	// called and gdi_plus.init_state equals IS_OK.
	struct gdi_plus {
		enum init_state init_state;
		HMODULE module;
		ULONG_PTR token;
		void * gp_graphics;

		int (__stdcall * CreateFromHDC) (HDC hdc, void ** p_gp_graphics);
		int (__stdcall * DeleteGraphics) (void * gp_graphics);
		int (__stdcall * CreatePen1) (unsigned int argb_color, float width, int gp_unit, void ** p_gp_pen);
		int (__stdcall * DeletePen) (void * gp_pen);
		int (__stdcall * DrawLineI) (void * gp_graphics, void * gp_pen, int x1, int y1, int x2, int y2);
	} gdi_plus;

	// These variables track the states of some OpenGL parameters. They're updated whenever methods like OpenGLRenderer::set_color are called.
	unsigned int ogl_color;
	int ogl_line_width;

	// ==========
	// }
	// ==========
};

enum object_job {
	OJ_DEFINE = 0,
	OJ_INLEAD, // Patch this function with an inlead
	OJ_REPL_VPTR, // Patch this function by replacing a pointer to it. The address column is the addr of the VPTR not the function itself.
	OJ_REPL_CALL, // Patch a single function call. The address column is the addr of the call instruction, name refers to the new target function, type is not used.
	OJ_REPL_VIS, // Patch a cluster of four function calls that make up a check of tile visibility. See implementation for details.
	OJ_IGNORE
};

struct civ_prog_object {
	enum object_job job;
	int addr;
	char const * name;
	char const * type;
};
