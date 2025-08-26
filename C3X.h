
#include <stdbool.h>

#define NOVIRTUALKEYCODES // Keycodes defined in Civ3Conquests.h instead
#include "windows.h"

typedef unsigned char byte;

// Use fastcall as substitute for thiscall because TCC doesn't recognize thiscall
#define __fastcall __attribute__((fastcall))
#include "Civ3Conquests.h"

#define MOD_VERSION 2400
#define MOD_PREVIEW_VERSION 0

#define COUNT_TILE_HIGHLIGHTS 11
#define MAX_BUILDING_PREREQS_FOR_UNIT 10

// Initialize to zero. Implementation is in common.c
struct table {
	void * block;
	size_t capacity_exponent; // Actual capacity is 1 << capacity_exponent
	size_t len;
};

// Initialize to zero. Implementation in common.c
struct buffer {
	byte * contents;
	int length;
	int capacity;
};

// A mill is a city improvement that spawns a resource. These are read from the "buildings_generating_resources" key in the config but are called
// "mills" internally for brevity.
enum mill_flag {
	MF_LOCAL          = 1,
	MF_NO_TECH_REQ    = 2,
	MF_YIELDS         = 4,
	MF_SHOW_BONUS     = 8,
	MF_HIDE_NON_BONUS = 16
};
struct mill {
	short improv_id;
	short resource_id;
	short flags;
};

#define ERA_ALIAS_LIST_CAPACITY 4 // Must not exceed 32 so we can store all genders as bits in an int

// A list of per-era aliases for a civ's noun, adjective, or formal name.
struct civ_era_alias_list {
	char * key;
	char * aliases[ERA_ALIAS_LIST_CAPACITY];
};

// A list of per-era aliases for a civ's leader's name, including genders and (optionally) titles.
struct leader_era_alias_list {
	char * key;
	char * aliases[ERA_ALIAS_LIST_CAPACITY];
	int gender_bits; // Gender of each alias. Like LeaderGender in the Race object, 0 for male and 1 for female.
	char * titles[ERA_ALIAS_LIST_CAPACITY]; // Title for each alias. May be NULL.
};

struct unit_type_limit {
	int per_civ;
	int per_city;
	int cities_per;
};

enum retreat_rules {
	RR_STANDARD = 0,
	RR_NONE,
	RR_ALL_UNITS,
	RR_IF_FASTER
};

enum line_drawing_override {
	LDO_NEVER = 0,
	LDO_WINE,
	LDO_ALWAYS
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

enum work_area_limit {
	WAL_NONE = 0,
	WAL_CULTURAL,
	WAL_CULTURAL_MIN_2,
	WAL_CULTURAL_OR_ADJACENT
};

enum perfume_kind {
	PK_PRODUCTION = 0,
	PK_TECHNOLOGY,
	PK_GOVERNMENT,

	COUNT_PERFUME_KINDS
};

struct c3x_config {
	bool enable_stack_bombard;
	bool enable_disorder_warning;
	bool allow_stealth_attack_against_single_unit;
	bool show_detailed_city_production_info;
	int limit_railroad_movement;
	bool limited_railroads_work_like_fast_roads;
	int limit_units_per_tile[3]; // Limits for land, sea, and air units respectively
	bool exclude_cities_from_units_per_tile_limit;
	bool enable_free_buildings_from_small_wonders;
	bool enable_stack_unit_commands;
	bool skip_repeated_tile_improv_replacement_asks;
	bool autofill_best_gold_amount_when_trading;
	int minimum_city_separation;
	bool disallow_founding_next_to_foreign_city;
	bool enable_trade_screen_scroll;
	bool group_units_on_right_click_menu;
	bool gray_out_units_on_menu_with_no_remaining_moves;
	bool put_movement_icons_on_units_on_menu;
	bool describe_states_of_units_on_menu;
	int anarchy_length_percent;
	bool show_golden_age_turns_remaining;
	bool show_zoc_attacks_from_mid_stack;
	bool cut_research_spending_to_avoid_bankruptcy;
	bool dont_pause_for_love_the_king_messages;
	bool reverse_specialist_order_with_shift;
	bool toggle_zoom_with_z_on_city_screen;
	bool dont_give_king_names_in_non_regicide_games;
	bool no_elvis_easter_egg;
	bool disable_worker_automation;
	bool enable_land_sea_intersections;
	bool disallow_trespassing;
	bool show_detailed_tile_info;
	struct table perfume_specs[COUNT_PERFUME_KINDS]; // Each table maps strings to i31b's. Each i31b combines an amount and whether it's a percent
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
	bool allow_airdrop_without_airport;
	bool enable_negative_pop_pollution;
	enum retreat_rules land_retreat_rules;
	enum retreat_rules sea_retreat_rules;
	bool allow_defensive_retreat_on_water;
	int ai_multi_city_start;
	int max_tries_to_place_fp_city;
	int * ai_multi_start_extra_palaces;
	int count_ai_multi_start_extra_palaces;
	int ai_multi_start_extra_palaces_capacity;
	bool promote_forbidden_palace_decorruption;
	bool allow_military_leaders_to_hurry_wonders;
	int ai_research_multiplier;
	int ai_settler_perfume_on_founding;
	int ai_settler_perfume_on_founding_duration;
	bool aggressively_penalize_bankruptcy;
	bool no_penalty_exception_for_agri_fresh_water_city_tiles;
	bool suppress_hypertext_links_exceeded_popup;
	bool indicate_non_upgradability_in_pedia;
	bool show_message_after_dodging_sam;
	bool include_stealth_attack_cancel_option;
	bool intercept_recon_missions;
	bool charge_one_move_for_recon_and_interception;
	bool polish_precision_striking;
	bool enable_stealth_attack_via_bombardment;
	bool immunize_aircraft_against_bombardment;
	bool replay_ai_moves_in_hotseat_games;
	struct table ptw_arty_types; // Table mapping unit type IDs to 1's; used as a hash set
	bool restore_unit_directions_on_game_load;
	bool apply_grid_ini_setting_on_game_load;
	bool charm_flag_triggers_ptw_like_targeting;
	bool city_icons_show_unit_effects_not_trade;
	bool ignore_king_ability_for_defense_priority;
	bool show_untradable_techs_on_trade_screen;
	bool disallow_useless_bombard_vs_airfields;
	enum line_drawing_override draw_lines_using_gdi_plus;
	bool compact_luxury_display_on_city_screen;
	bool compact_strategic_resource_display_on_city_screen;
	bool warn_when_chosen_building_would_replace_another;
	bool do_not_unassign_workers_from_polluted_tiles;
	bool do_not_make_capital_cities_appear_larger;
	bool enable_city_capture_by_barbarians;
	bool share_visibility_in_hotseat;
	bool share_wonders_in_hotseat;
	bool allow_precision_strikes_against_tile_improvements;
	bool dont_end_units_turn_after_bombarding_barricade;
	bool remove_land_artillery_target_restrictions;
	bool allow_bombard_of_other_improvs_on_occupied_airfield;
	bool show_total_city_count;
	bool strengthen_forbidden_palace_ocn_effect;
	int extra_unit_maintenance_per_shields;
	enum special_zone_of_control_rules special_zone_of_control_rules;
	enum special_defensive_bombard_rules special_defensive_bombard_rules;
	struct civ_era_alias_list * civ_era_alias_lists;
	int count_civ_era_alias_lists;
	struct leader_era_alias_list * leader_era_alias_lists;
	int count_leader_era_alias_lists;
	struct table unit_limits; // Maps unit type names (strings) to pointers to limit objects (struct unit_type_limit *)
	bool allow_upgrades_in_any_city;
	bool do_not_generate_volcanos;
	bool do_not_pollute_impassable_tiles;
	bool show_hp_of_stealth_attack_options;
	bool exclude_invisible_units_from_stealth_attack;
	bool convert_to_landmark_after_planting_forest;
	int chance_for_nukes_to_destroy_max_one_hp_units;
	bool allow_sale_of_aqueducts_and_hospitals;
	bool no_cross_shore_detection;
	int city_work_radius;
	enum work_area_limit work_area_limit;
	int rebase_range_multiplier;
	bool limit_unit_loading_to_one_transport_per_turn;
	bool prevent_old_units_from_upgrading_past_ability_block;

	bool enable_trade_net_x;
	bool optimize_improvement_loops;
	bool measure_turn_times;

	bool use_offensive_artillery_ai;
	bool dont_escort_unflagged_units;
	int ai_build_artillery_ratio;
	int ai_artillery_value_damage_percent;
	int ai_build_bomber_ratio;
	bool replace_leader_unit_ai;
	bool fix_ai_army_composition;
	bool enable_pop_unit_ai;
	bool enable_caravan_unit_ai;
	int max_ai_naval_escorts;
	int ai_worker_requirement_percent;

	bool remove_unit_limit;
	bool remove_city_improvement_limit;
	bool remove_era_limit;
	bool remove_cap_on_turn_limit;
	bool move_trade_net_object;

	bool patch_submarine_bug;
	bool patch_science_age_bug;
	bool patch_pedia_texture_bug;
	bool patch_blocked_disembark_freeze;
	bool patch_houseboat_bug;
	bool patch_intercept_lost_turn_bug;
	bool patch_phantom_resource_bug;
	bool patch_maintenance_persisting_for_obsolete_buildings;
	bool patch_barbarian_diagonal_bug;
	bool patch_disease_stopping_tech_flag_bug;
	bool patch_division_by_zero_in_ai_alliance_eval;
	bool patch_empty_army_movement;
	bool delete_off_map_ai_units;
	bool fix_overlapping_specialist_yield_icons;
	bool patch_premature_truncation_of_found_paths;
	bool patch_zero_production_crash;

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

// ==========
// Districts
// ==========

const int COUNT_DISTRICT_TYPES = 1;

struct district_info {
	enum Unit_Command_Values command;
	char const * tooltip;
	char const * advance_prereq;
	char const * dependent_improvements[5];
	char const * img_paths[4];
	int allow_multiple,
		index,
		btn_tile_sheet_column,
	    btn_tile_sheet_row,
		total_img_columns;
} const district_infos[1] = {
	{ 
		.command = UCV_Build_Encampment, .tooltip = "Build Encampment", .img_paths = {"DistrictEncampment.pcx"},
		.advance_prereq = "Bronze Working", .dependent_improvements = {"Barracks", "SAM Missile Battery"},
		.allow_multiple = 0, .index = 0, .btn_tile_sheet_column = 0, .btn_tile_sheet_row = 0, .total_img_columns = 4
	},
	//{ 
	//	.command = UCV_Build_Campus, .tooltip = "Build Campus", .img_paths = {"DistrictCampus.pcx"}, 
	//	.prerequisite = "Literature", .allow_multiple = 0, .index = 1, .btn_tile_sheet_column = 1, .btn_tile_sheet_row = 0, .total_img_columns = 4
	//},
	//{ 
	//	.command = UCV_Build_HolySite, .tooltip = "Build Holy Site", .img_paths = {"DistrictHolySite.pcx"}, 
	//	.prerequisite = "Ceremonial Burial", .allow_multiple = 0, .index = 2, .btn_tile_sheet_column = 2, .btn_tile_sheet_row = 0, .total_img_columns = 4
	//}
};

// ==========
// End Districts
// ==========

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
	CL_MCS_FAILED_SANITY_CHECK,
	CL_MCS_ADJACENT_CITIES,
	CL_MCS_MISSING_CITIES,
	CL_OBSOLETED_BY,
	CL_NO_STEALTH_ATTACK,
	CL_DODGED_SAM,
	CL_PREVIEW,
	CL_CITY_TOO_CLOSE_BUTTON_TOOLTIP,
	CL_TOTAL_CITIES,

	// Offense, Defense, Artillery, etc.
	CL_FIRST_UNIT_STRAT,
	CL_LAST_UNIT_STRAT = CL_FIRST_UNIT_STRAT + 19,

	// Unit actions for right-click menu
	CL_IDLE,
	CL_FORTIFIED,
	CL_SENTRY,
	CL_MINING,
	CL_IRRIGATING,
	CL_BUILDING_FORTRESS,
	CL_BUILDING_ROAD,
	CL_BUILDING_RAILROAD,
	CL_PLANTING_FOREST,
	CL_CLEARING_FOREST,
	CL_CLEARING_WETLANDS,
	CL_CLEARING_DAMAGE,
	CL_BUILDING_AIRFIELD,
	CL_BUILDING_RADAR_TOWER,
	CL_BUILDING_OUTPOST,
	CL_BUILDING_BARRICADE,
	CL_BUILDING_COLONY,
	CL_INTERCEPTING,
	CL_MOVING,
	CL_AUTOMATED,
	CL_EXPLORING,
	CL_BOMBARDING,

	// "Action" for passenger units
	CL_TRANSPORTED,

	CL_IN_STATE_27,
	CL_IN_STATE_28,
	CL_IN_STATE_29,
	CL_IN_STATE_30,
	CL_IN_STATE_33,

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

enum unit_rcm_icon {
	URCMI_UNMOVED = 0,
	URCMI_MOVED_CAN_ATTACK,
	URCMI_MOVED_NO_ATTACK,
	URCMI_CANT_MOVE,

	COUNT_UNIT_RCM_ICONS
};

enum unit_rcm_icon_set {
	URCMIS_ATTACKER = 0,
	URCMIS_NONCOMBAT,
	URCMIS_BUSY_ATTACKER,
	URCMIS_BUSY_NONCOMBAT,

	COUNT_UNIT_RCM_ICON_SETS
};

enum city_gain_reason {
	CGR_FOUNDED = 0,
	CGR_CONQUERED,
	CGR_CONVERTED, // covers culture flips & bribes
	CGR_TRADED,
	CGR_POPPED_FROM_HUT,
	CGR_PLACED_FOR_AI_RESPAWN,
	CGR_PLACED_FOR_SCENARIO,
	CGR_PLACED_FOR_AI_MULTI_CITY_START,
};

enum city_loss_reason {
	CLR_DESTROYED = 0, // means city was razed for any reason (inc. by conqueror)
	CLR_CONQUERED,
	CLR_CONVERTED, // covers culture flips & bribes
	CLR_TRADED
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
	enum init_state dc_img_state;
	enum init_state dc_btn_img_state;
	enum init_state tile_highlight_state;
	enum init_state mod_info_button_images_state;
	enum init_state disabled_command_img_state;
	enum init_state unit_rcm_icon_state;
	enum init_state red_food_icon_state;
	enum init_state tile_already_worked_zoomed_out_sprite_init_state;

	// ==========
	// } These fields are valid at any time after patch_init_floating_point runs (which is at the program launch). {
	// ==========

	struct c3x_config base_config;

	// Windows modules
	HMODULE kernel32;
	HMODULE user32;
	HMODULE msvcrt;
	HMODULE msimg32;

	// Win32 API functions
	WINBOOL (WINAPI * VirtualProtect) (LPVOID, SIZE_T, DWORD, PDWORD);
	WINBOOL (WINAPI * CloseHandle) (HANDLE);
	HANDLE (WINAPI * CreateFileA) (LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
	DWORD (WINAPI * GetFileSize) (HANDLE, LPDWORD);
	WINBOOL (WINAPI * ReadFile) (HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
	HMODULE (WINAPI * LoadLibraryA) (LPCSTR);
	BOOL (WINAPI * FreeLibrary) (HMODULE);
	int (WINAPI * MultiByteToWideChar) (UINT, DWORD, LPCCH, int, LPWSTR, int);
	int (WINAPI * WideCharToMultiByte) (UINT, DWORD, LPCWCH, int, LPSTR, int, LPCCH, LPBOOL);
	int (WINAPI * GetLastError) ();
	BOOL (WINAPI * QueryPerformanceCounter) (LARGE_INTEGER *);
	BOOL (WINAPI * QueryPerformanceFrequency) (LARGE_INTEGER *);
	void (WINAPI * GetLocalTime) (LPSYSTEMTIME);

	// Win32 funcs from user32.dll
	int (WINAPI * MessageBoxA) (HWND, LPCSTR, LPCSTR, UINT);

	// Win32 funcs from Msimg32.dll
	BOOL (WINAPI * TransparentBlt) (HDC, int, int, int, int, HDC, int, int, int, int, UINT);

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

	Unit * sb_next_up; // The unit currently doing a stack bombard or NULL otherwise. Gets set to NULL if the unit is despawned.

	Trade_Net * trade_net; // Pointer to the trade net object. If it hasn't been moved by the mod, this equals p_original_trade_net.

	enum init_state trade_net_refs_load_state;
	int * trade_net_refs;

	HMODULE trade_net_x;
	void (__stdcall * set_exe_version) (int);
	void * (__stdcall * create_tnx_cache) (Map *);
	void (__stdcall * destroy_tnx_cache) (void *);
	void (__stdcall * set_up_before_building_network) (void *);
	int (__stdcall * get_move_cost_for_sea_trade) (Trade_Net * trade_net, void * tnx_cache, int from_x, int from_y, int to_x, int to_y, int civ_id, unsigned int flags, int neighbor_index, Trade_Net_Distance_Info * dist_info);
	void (__stdcall * flood_fill_road_network) (void * tnx_cache, int from_x, int from_y, int civ_id);
	bool (__stdcall * try_drawing_sea_trade_route) (Trade_Net * trade_net, void * tnx_cache, int from_x, int from_y, int to_x, int to_y, int civ_id, unsigned int flags);

	void * tnx_cache; // Cache object used by Trade Net X. Initially NULL, must be recreated every time a new map is loaded.
	enum init_state tnx_init_state;
	bool is_computing_city_connections; // Set to true only while Trade_Net::recompute_city_connections is running
	bool keep_tnx_cache;

	// This flag gets set whenever a trade deal is signed or broken for a resource that's an input to a mill. It's necessary to call
	// recompute_resources in that case since a mill may have been de/activated by the change. We can't call it right when the change happens as
	// that can crash the game for reasons I don't completely understand. I believe it's because the recomputation can't be done while some other
	// econ logic is running. Instead, we set this variable then recompute before obsolete data would be an issue. Specifically, this is done:
	//   - When any player or AI begins their production phase
	//   - When the player zooms to any city
	//   - When the player opens the shift + right-click production chooser menu
	//   - When the player visits any advisor
	//   - When the player selects any unit
	bool must_recompute_resources_for_mill_inputs;

	bool is_placing_scenario_things; // Set to true only while Map::place_scenario_things is running

	bool paused_for_popup; // Set to true while a popup, map message, or the diplo screen is open
	long long time_spent_paused_during_popup; // Tracks time spent waiting for the three things above

	// This variable is increased with the time elapsed during every call to Trade_Net::recompute_city_connections. Time is measured using
	// Windows performance counter.
	long long time_spent_computing_city_connections;
	int count_calls_to_recompute_city_connections;

	long long time_spent_filling_roads;

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

	// Array mapping cultural neighbor indices to the standard indices that correspond to the same tiles. Generated at program start.
	byte * cultural_ni_to_standard;

	// Array mapping standard neighbor indices to the smallest work radius that includes the corresponding tile. Ex., if a given n.i. corresponds
	// to one of the adjacent tiles, maps to 1, if it's in the fat cross but not adjacent, maps to 2, and so forth, out into the extended area.
	char ni_to_work_radius[256];

	// The maximum number of tiles workable by cities including the city tile itself (21 under standard game rules). Updated whenever the
	// city_work_radius config value gets changed.
	int workable_tile_count;

	// The civ ID of the player from whose perspective we're currently showing city loc desirability, or -1 if none. Initialized to -1.
	int city_loc_display_perspective;

	// These are all bit fields that run parallel to player bits. Each bit indicates whether or not that name was replaced with an era-specific
	// version for that player. This allows us to tell the difference between custom names set by human players and replacements made by the mod.
	int aliased_civ_noun_bits;
	int aliased_civ_adjective_bits;
	int aliased_civ_formal_name_bits;
	int aliased_leader_name_bits;
	int aliased_leader_title_bits;

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

	// Records the turn number on which each player has most recently founded a city. This is intended to be used for the temp settler perfume
	// after founding feature so it may not be set if that feature is not activated or applicable. Defaults to -1.
	int turn_no_of_last_founding_for_settler_perfume[32];

	// Stores the byte offsets into the c3x_config struct of all boolean/integer config options, accessible using the options' names as
	// strings. Used when reading in a config INI file.
	struct table boolean_config_offsets;
	struct table integer_config_offsets;

	// Maps unit types IDs to AI strategy indices (0 = offense, 1 = defense, 2 = artillery, etc.). If a unit type ID is in this table, that means
	// it's one of several duplicate types created to spread multiple AI strategies out so each type has only one.
	struct table unit_type_alt_strategies;

	// Stores a linked list of unit type duplicates. Maps unit type IDs to the next duplicate ID. Use list_unit_type_duplicates to get the list of
	// the duplicates of a particular type as an array.
	struct table unit_type_duplicates;

	// Tracks the number of "extra" defensive bombards units have performed, by their IDs. If the "blitz" special defensive bombard rule is
	// activated, units with blitz get an extra chance to perform DB for each movement point they have beyond the first.
	struct table extra_defensive_bombards;

	// Table mapping unit IDs to how many times that unit has airdropped on the current turn. This is used to prevent units from airdropping
	// multiple times. That doesn't matter for the base game but stops the dont-end-turn-after-airdrop setting from letting units airdrop an
	// unlimited number of times.
	struct table airdrops_this_turn;

	// Stores city improvement bits for improvs beyond the first 256
	struct table extra_city_improvs;

	// These variables store the number of units of each type that each player has
	int unit_type_count_init_bits; // Player bits tracking which unit type count tables have been initialized.
	struct table unit_type_counts[32]; // One table per player. Each one maps unit type ids (ints) to counts (ints)

	// ==========
	// } These fields are valid only after init_stackable_command_buttons has been called. {
	// ==========

	struct sc_button_image_set {
		Sprite imgs[4];
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

	Sprite disabled_build_city_button_img;

	// ==========
	// } This field is only valid after init_unit_rcm_icons has been called and unit_rcm_icon_state equals IS_OK {
	// ==========

	// Sprites are stored together as sets, so the first COUNT_UNIT_RCM_ICONS elements are those from the first set, then the same number from the
	// second set, etc.
	Sprite unit_rcm_icons[COUNT_UNIT_RCM_ICONS * COUNT_UNIT_RCM_ICON_SETS];

	// ==========
	// } These fields are valid only after init_tile_highlights as been called. {
	// ==========

	Sprite tile_highlights[COUNT_TILE_HIGHLIGHTS];

	// ==========
	// } This one is valid only if init_mod_info_button_images has been called and mod_info_button_images_state equals IS_OK {
	// ==========

	Sprite mod_info_button_images[3];

	// ==========
	// } This one is valid only if init_red_food_icon has been called and red_food_icon_state equals IS_OK {
	// ==========

	Sprite red_food_icon;

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
	Sprite * trade_scroll_button_images; // inited to NULL, array of 6 images: normal, rollover, and highlight for left & right
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

	// Used in patch_Tile_m43_Get_field_30_for_city_loc_eval to change how the AI evaluates overlap between cities
	int ai_evaling_city_loc_x, ai_evaling_city_loc_y;
	int ai_evaling_city_field_30_get_counter;

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
	byte * mill_input_resource_bits; // Array of bits, one for each resource. Stores whether or not each one is an input to any mill.

	// Used for displaying yields from generated resources on the city screen
	int tourism_icon_counter; // Incremented each time a tourism yield icon (normally commerce) is drawn. Reset between improvs.
	int convert_displayed_tourism_to_food, convert_displayed_tourism_to_shields; // Number of commerce tourism icons to convert to food, shields
	int combined_tourism_and_mill_commerce; // Number of commerce tourism icons to display inc. from mills, maybe negative

	int drawing_icons_for_improv_id; // Stores the improv ID whose icons we're drawing. -1 while not drawing.

	PCX_Image * resources_sheet; // Sprite sheet of resource icons, i.e. resources.pcx

	Sprite tile_already_worked_zoomed_out_sprite; // Valid only if init state is OK

	// Only for use by patch_City_Form_draw_yields_on_worked_tiles and patch_Sprite_draw_already_worked_tile_img
	bool do_not_draw_already_worked_tile_img;

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

	// Set when Fighter::apply_zone_of_control is called to store the defending unit, used by the injected filter and Unit::move_to_adjacent_tile.
	// Cleared at each call to move_to_adjacent_tile and by Unit::despawn.
	Unit * zoc_defender;

	// Set to the bombarding unit while Unit::bombard_tile is running. NULL otherwise.
	Unit * bombarding_unit;

	// Normally set to NULL. When a unit bombards a tile (the tile itself, not something on it), set to point to that unit during the call to
	// Unit::attack_tile. Used to stop the unit from losing all of its movement if configured.
	Unit * unit_bombard_attacking_tile;

	// Set to the coords of the tile being attacked while Unit::attack_tile is running, -1 otherwise.
	int attacking_tile_x, attacking_tile_y;

	// Cleared to false when Fighter::apply_zone_of_control is called. The interceptor must be unfortified to ensure it plays its animation. If
	// that happens, this flag is set so that apply_zone_of_control knows to refortify the unit after the ZoC process is done.
	bool refortify_interceptor_after_zoc;

	// These flags are used to turn off lethal ZoC for units that capture an enemy on the tile they're moving into. Without this rule, the unit
	// might get killed by ZoC but the captured units will remain on a tile that was never actually entered (that tile might have an enemy city
	// for extra weirdness). Here's how it works:
	//  1. The moving_unit_to_adjacent_tile flag is set when Unit::move_to_adjacent_tile is called.
	//  2. If that flag is set and a unit is captured (detected by intercepting the calls to Leader::spawn_unit inside Unit::do_capture_units)
	//     then temporarily_disallow_lethal_zoc is set.
	//  3. If temporar... is set, the code to filter ZoC interceptors acts as if lethal ZoC were turned off.
	//  4. Both flags are cleared when Unit::move_to_adjacent_tile returns.
	bool temporarily_disallow_lethal_zoc;
	bool moving_unit_to_adjacent_tile;

	// Used to record info about a defensive bomardment event during Fighter::fight. Gets set by Fighter::damage_by_defensive_bombardment and
	// cleared when Fighter::fight returns.
	struct defensive_bombard_event {
		Unit * bombarder;
		Unit * defender;
		bool damage_done, defender_was_destroyed, saved_animation_setting;
	} dbe;

	// Set to true IFF we're showing a replay of AI moves in hotseat mode
	bool showing_hotseat_replay;

	// Set to true only during the first call to get_tile_occupier_id from Trade_Net::get_movement_cost. While this is set, we need to edit unit
	// visibility to patch the submarine bug.
	bool getting_tile_occupier_for_ai_pathfinding;

	bool running_on_wine; // Set to 1 IFF we're running in Wine, as opposed to actual Windows

	// gdi_plus.init_state is valid any time after patch_init_floating_point. All the other fields are only valid after set_up_gdi_plus has been
	// called and gdi_plus.init_state equals IS_OK.
	struct gdi_plus {
		enum init_state init_state;
		HMODULE module;
		ULONG_PTR token;
		void * gp_graphics;

		int (__stdcall * CreateFromHDC) (HDC hdc, void ** p_gp_graphics);
		int (__stdcall * DeleteGraphics) (void * gp_graphics);
		int (__stdcall * SetSmoothingMode) (void * graphics, int smoothing_mode);
		int (__stdcall * SetPenDashStyle) (void * gp_pen, int dash_style);
		int (__stdcall * CreatePen1) (unsigned int argb_color, float width, int gp_unit, void ** p_gp_pen);
		int (__stdcall * DeletePen) (void * gp_pen);
		int (__stdcall * DrawLineI) (void * gp_graphics, void * gp_pen, int x1, int y1, int x2, int y2);
	} gdi_plus;

	// These variables track the states of some OpenGL parameters. They're updated whenever methods like OpenGLRenderer::set_color are called.
	unsigned int ogl_color;
	int ogl_line_width;
	bool ogl_line_stipple_enabled;

	// Records how many units of each type we're going to upgrade to during an upgrade-all. This info will be used to impose the unit type limit
	// during the upgrade. Keep in mind units all get upgraded from the same type but might end up as different types after upgrade-all.
	struct penciled_in_upgrade {
		int unit_type_id;
		int count;
	} * penciled_in_upgrades;
	int penciled_in_upgrade_count;
	int penciled_in_upgrade_capacity;

	// While in Leader::do_capture_city, the city in question is stored in this var. Otherwise it's NULL.
	City * currently_capturing_city;

	// While a game is being saved or loaded, this variable points to the save file's MappedFile object. Otherwise it's NULL.
	MappedFile * accessing_save_file;

	// Used in patch_work_simple_job. If a method sets this variable while that method is running then at the end it sets the given tile as LM.
	Tile * lmify_tile_after_working_simple_job;

	// Reset to zero every time City_Form::draw is called. Incremented everything a strategic resource is drawn on the city screen.
	int drawn_strat_resource_count;

	int * charmed_types_converted_to_ptw_arty;
	int count_charmed_types_converted_to_ptw_arty;
	int charmed_types_converted_to_ptw_arty_capacity;

	// While Unit::is_visible_to_civ is running, this var is set to the unit in question. Otherwise it's NULL.
	Unit * checking_visibility_for_unit;

	// Normally false. When true, calls to bounce_trespassing_units won't kick out invisible units even if they're revealed.
	bool do_not_bounce_invisible_units;

	// If limit_unit_loading_to_one_transport_per_turn is on, maps unit IDs to the ID of the transport unit they're tied to for the current turn.
	struct table unit_transport_ties;

	// Initialized to NULL/0, used in patch_City_add_happiness_from_buildings
	short * saved_improv_counts;
	int saved_improv_counts_capacity;

	// Used to de-overlap specialist yield icons (option name fix_overlapping_specialist_yield_icons)
	int specialist_icon_drawing_running_x;

	// Initialized to 0, used to draw multipage descriptions in the Civilopedia
	struct civilopedia_multipage_description {
		bool active_now;
		int line_count;
		int shown_page; // zero-based
	} cmpd;

	// Districts data
	struct district_image_set {
		Sprite imgs[4][10]; // 1st dimension = era, 2nd dimension = district image variant
	} district_img_sets[1];

	struct district_button_image_set {
		Sprite imgs[4];
	} district_btn_img_sets[1];

	struct district_prereq {
		int tech_id;
	} * district_prereqs[1];

	struct district_improv {
		int improv_ids[5];
	} * district_improvs[1];

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
	OJ_EXT_WALUP, // "EXTend Work Area LuP" Patch a jump instruction at the end of a "lup" (loop) over a city's workable area to extend it
	OJ_IGNORE
};

struct civ_prog_object {
	enum object_job job;
	int addr;
	char const * name;
	char const * type;
};
