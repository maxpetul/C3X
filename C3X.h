
#define NOVIRTUALKEYCODES // Keycodes defined in Civ3Conquests.h instead
#include "windows.h"

typedef unsigned char byte;

// Use fastcall as substitute for thiscall because TCC doesn't recognize thiscall
#define __fastcall __attribute__((fastcall))
#include "Civ3Conquests.h"

#define MOD_VERSION 600

struct c3x_config {
	char enable_stack_bombard;
	char enable_disorder_warning;
	char allow_stealth_attack_against_single_unit;
	char show_detailed_city_production_info;
	int limit_railroad_movement;
	char enable_free_buildings_from_small_wonders;
	char enable_stack_worker_commands;
	char skip_repeated_tile_improv_replacement_asks;
	char autofill_best_gold_amount_when_trading;
	int adjust_minimum_city_separation;
	char disallow_founding_next_to_foreign_city;
	char enable_trade_screen_scroll;

	char use_offensive_artillery_ai;
	int ai_build_artillery_ratio;
	int ai_artillery_value_damage_percent;
	int ai_build_bomber_ratio;
	char replace_leader_unit_ai;
	char fix_ai_army_composition;

	char remove_unit_limit;
	char remove_era_limit;

	char patch_submarine_bug;
	char patch_science_age_bug;
	char patch_pedia_texture_bug;
	char patch_disembark_immobile_bug;
	char patch_houseboat_bug;

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
	COUNT_STACKABLE_COMMANDS
};

struct sc_button_info {
	enum Unit_Command_Values command;
	int tile_sheet_column,
	    tile_sheet_row;
} const sc_button_infos[COUNT_STACKABLE_COMMANDS] = {
	/* Bombard */    { .command = UCV_Bombard        , .tile_sheet_column = 3, .tile_sheet_row = 1 },
	/* Bomb */       { .command = UCV_Bombing        , .tile_sheet_column = 5, .tile_sheet_row = 4 },
	/* Fortress */   { .command = UCV_Build_Fortress , .tile_sheet_column = 0, .tile_sheet_row = 3 },
	/* Mine */       { .command = UCV_Build_Mine     , .tile_sheet_column = 1, .tile_sheet_row = 3 },
	/* Irrigate */   { .command = UCV_Irrigate       , .tile_sheet_column = 2, .tile_sheet_row = 3 },
	/* Chop For. */  { .command = UCV_Clear_Forest   , .tile_sheet_column = 3, .tile_sheet_row = 3 },
	/* Chop Jun. */  { .command = UCV_Clear_Jungle   , .tile_sheet_column = 4, .tile_sheet_row = 3 },
	/* Plant */      { .command = UCV_Plant_Forest   , .tile_sheet_column = 5, .tile_sheet_row = 3 },
	/* Clean Pol. */ { .command = UCV_Clear_Pollution, .tile_sheet_column = 6, .tile_sheet_row = 3 },
	/* Road */       { .command = UCV_Build_Road     , .tile_sheet_column = 6, .tile_sheet_row = 2 },
	/* Railroad */   { .command = UCV_Build_Railroad , .tile_sheet_column = 7, .tile_sheet_row = 2 },
};

enum init_state {
	IS_UNINITED = 0,
	IS_OK,
	IS_INIT_FAILED
};

enum label {
	LAB_NEVER_COMPLETES = 0,
	LAB_HALTED,
	LAB_SURPLUS,
	LAB_SURPLUS_NONE,
	LAB_SURPLUS_NA,
	LAB_SB_TOOLTIP,
	COUNT_LABELS
};

struct worker_job_and_location {
	enum Worker_Jobs job;
	int tile_x, tile_y;
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
	void (* qsort) (void *, size_t, size_t, int (*) (void const *, void const *));
	int (* memcmp) (void const *, void const *, size_t);

	struct c3x_config current_config;

	char * labels[COUNT_LABELS];

	int have_job_and_loc_to_skip; // 0 or 1 if the variable below has anything actionable in it. Gets cleared to 0 after
	// every turn.
	struct worker_job_and_location to_skip;

	byte houseboat_patch_area_original_contents[50];

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

	/*
	PCX_Image sb_command_button_sheets[4]; // TODO: I think we can deconstruct these after creating the tile images
	Tile_Image_Info sb_bard_button_images[4];
	Tile_Image_Info sb_bomb_button_images[4];
	*/

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

	// ==========
	// }
	// ==========
};

// Putting code in a header file like an absolute madman

// Writes an integer to a byte buffer. buf need not be aligned but it must have at least four bytes of free space. Written little-endian.
byte *
int_to_bytes (byte * buf, int x)
{
	buf[0] =  x        & 0xFF;
	buf[1] = (x >>  8) & 0xFF;
	buf[2] = (x >> 16) & 0xFF;
	buf[3] = (x >> 24) & 0xFF;
	return buf + 4;
}

// Read a little-endian int from a byte buffer
int
int_from_bytes (byte * buf)
{
	int lo = buf[0] | ((int)buf[1] << 8);
	int hi = buf[2] | ((int)buf[3] << 8);
	return (hi << 16) | lo;
}
