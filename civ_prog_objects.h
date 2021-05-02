
enum OBJECT_JOB {
	OJ_DEFINE = 0,
	OJ_INLEAD, // Patch this function with an inlead
	OJ_REPL_VPTR, // Patch this function by replacing a pointer to it. NOTE: The address column is the addr of the VPTR not the function itself.
};

struct civ_prog_object {
	enum OBJECT_JOB job;
	char const * name;
	char const * type;
	int gog_addr;
	int steam_addr;
} const civ_prog_objects[] = {
//       JOB            NAME                                            TYPE                                                                                                                                                                            GOG ADDR        STEAM ADDR
	{OJ_DEFINE,	"p_bic_data",					"BIC *",																					0x9C3508,	0x9E5D08},
	{OJ_DEFINE,	"p_units",					"Units *",																					0xA52E80,	0xA75680},
	{OJ_DEFINE,	"p_tile_units",					"TileUnits *",																					0xA52DD4,	0xA755D0},
	{OJ_DEFINE,	"p_cities",					"Cities *",																					0xA52E68,	0xA75668},
	{OJ_DEFINE,	"leaders",					"Leader *",																					0xA52E98,	0xA75698},
	{OJ_DEFINE,	"p_main_screen_form",				"Main_Screen_Form *",																				0x9F8700,	0xA1AF00},
	{OJ_DEFINE,	"is_game_type_4_or_5",				"char (__stdcall *) (void)",																			0x499FE0,	0x49F9F0},
	{OJ_DEFINE,	"tile_at",					"Tile * (__cdecl *) (int x, int y)",																		0x437A70,	0x439620},
	{OJ_DEFINE,	"TileUnits_TileUnitID_to_UnitID",		"int (__fastcall *) (TileUnits * this, int edx, int tile_unit_id, int * out_UnitItem_field_0)",											0x426C80,	0x4283C0},
	{OJ_DEFINE,	"Unit_bombard_tile",				"void (__fastcall *) (Unit * this, int edx, int x, int y)",															0x5C1410,	0x5CFFA0},
	{OJ_DEFINE,	"Unit_can_bombard_tile",			"void (__fastcall *) (Unit * this, int edx, int x, int y)",															0x5C0E20,	0x5CF9C0},
	{OJ_DEFINE,	"Unit_get_defense_strength",			"int (__fastcall *) (Unit * this)",																		0x5BE820,	0x5CD420},
	{OJ_DEFINE,	"Unit_is_visible_to_civ",			"char (__fastcall *) (Unit * this, int edx, int civ_id, int param_2)",														0x5BB650,	0x5CA190},
	{OJ_DEFINE,	"Tile_has_city",				"char (__fastcall *) (Tile * this)",																		0x5EA6C0,	0x5F9F10},
	{OJ_DEFINE,	"Unit_get_max_hp",				"int (__fastcall *) (Unit * this)",																		0x5BE5B0,	0x5CD180},
	{OJ_DEFINE,	"UnitType_has_ability",				"char (__fastcall *) (UnitType * this, int edx, enum UnitTypeAbilities a)",													0x5E4EF0,	0x5F4750},
	{OJ_DEFINE,	"Unit_get_max_move_points",			"int (__fastcall *) (Unit * this)",																		0x5BE470,	0x5CD030},
	{OJ_DEFINE,	"Button_set_tooltip",				"int (__fastcall *) (Button * this, int edx, char * str)",															0x60B370,	0x625850},
	{OJ_DEFINE,	"PCX_Image_construct",				"PCX_Image * (__fastcall *) (PCX_Image * this)",																0x5FC420,	0x60FC10},
	{OJ_DEFINE,	"PCX_Image_read_file",				"int (__fastcall *) (PCX_Image * this, int edx, char * file_path, void * param_2, int param_3, int param_4, int param_5)",							0x5FC820,	0x610360},
	{OJ_DEFINE,	"Tile_Image_Info_construct",			"Tile_Image_Info * (__fastcall *) (Tile_Image_Info * this)",															0x5F7E50,	0x608170},
	{OJ_DEFINE,	"Tile_Image_Info_slice_pcx",			"int (__fastcall *) (Tile_Image_Info * this, int edx, PCX_Image * img, int up_left_x, int up_left_y, int w, int h, int arg6, int arg7)",					0x5F7F90,	0x6083D0},
	{OJ_DEFINE,	"get_popup_form",				"PopupForm * (__stdcall *) ()",																			0x49FC50,	0x4A6790},
	{OJ_DEFINE,	"set_popup_str_param",				"int (__cdecl *) (int param_index, char * str, int param_3, int param_4)",													0x61C5A0,	0x63E390},
	{OJ_DEFINE,	"set_popup_int_param",				"int (__cdecl *) (int param_index, int value)",																	0x61C570,	0x63E350},
	{OJ_INLEAD,	"show_popup",					"int (__fastcall *) (void *this, int edx, int param_1, int param_2)",														0x611530,	0x62DAF0},
	{OJ_DEFINE,	"City_zoom_to",					"void (__fastcall *) (City *this, int edx, int param_1)",															0x4ACD20,	0x4B3CC0},
	{OJ_DEFINE,	"City_has_improvement",				"char (__fastcall *) (City * this, int edx, int i_improvement, char include_auto_improvements)",										0x4ACB50,	0x4B3AF0},
	{OJ_INLEAD,	"Main_Screen_Form_perform_action_on_tile",	"void (__fastcall *) (Main_Screen_Form * this, int edx, enum Unit_Mode_Actions action, int x, int y)",										0x4E6DB0,	0x4EF820},
	{OJ_INLEAD,	"Main_GUI_set_up_unit_command_buttons",		"void (__fastcall *) (Main_GUI * this)",																	0x550BB0,	0x55BCE0},
	{OJ_INLEAD,	"Main_GUI_handle_button_press",			"void (__fastcall *) (Main_GUI * this, int edx, int button_id)",														0x5577F0,	0x563600},
	{OJ_INLEAD,	"Main_Screen_Form_handle_key_down",		"int (__fastcall *) (Main_Screen_Form * this, int edx, int char_code, int virtual_key_code)",											0x4E05F0,	0x4E8F90},
	{OJ_INLEAD,	"handle_cursor_change_in_jgl",			"int (__stdcall *) ()",																				0x6268A0,	0x64B410},
	{OJ_INLEAD,	"Main_GUI_handle_click_in_status_panel",	"void (__fastcall *) (Main_GUI * this, int edx, int mouse_x, int mouse_y)",													0x555F80,	0x561B90},
	{OJ_DEFINE,	"get_font",					"Object_66C3FC * (__cdecl *) (int size, FontStyleFlags style)",															0x54CAD0,	0x557980},
	{OJ_DEFINE,	"PCX_Image_draw_centered_text",			"int (__fastcall *) (PCX_Image *this, int edx, Object_66C3FC * font, char * str, int x, int y, int width, int str_len)",							0x5FD750,	0x611850},
	{OJ_INLEAD,	"City_Form_draw",				"void (__fastcall *) (City_Form * this)",																	0x4196F0,	0x41A5D0},
	{OJ_INLEAD,	"City_Form_print_production_info",		"void (__fastcall *) (City_Form *this, int edx, String256 * out_strs, int str_capacity)",											0x41B4C0,	0x41C610},
	{OJ_DEFINE,	"City_get_order_cost",				"int (__fastcall *) (City * this)",																		0x4ACD70,	0x4B3D30},
	{OJ_DEFINE,	"City_get_order_progress",			"int (__fastcall *) (City * this)",																		0x4ACD40,	0x4B3CE0},
	{OJ_INLEAD,	"Trade_Net_get_movement_cost",			"int (__fastcall *) (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, unsigned param_7, int neighbor_index, int param_9)",	0x57F360,	0x58C080},
	{OJ_INLEAD,	"do_save_game",					"int (__cdecl *) (char * file_path, char param_2, GUID * guid)",														0x591BB0,	0x59F1D0},
	{OJ_INLEAD,	"General_clear",				"void (__fastcall *) (General * this)",																		0x5E7020,	0x5F6870},
	{OJ_INLEAD,	"General_load",					"void (__fastcall *) (General * this, int edx, byte ** buffer)",														0x5E78E0,	0x5F7130},
	{OJ_DEFINE,	"City_recompute_happiness",			"void (__fastcall *) (City * this)",																		0x4BCFF0,	0x4C4660},
	{OJ_INLEAD,	"Leader_recompute_auto_improvements",		"void (__fastcall *) (Leader * this)",																		0x55A560,	0x566470},
	{OJ_INLEAD,	"get_wonder_city_id",				"int (__fastcall *) (void * this, int edx, int wonder_improvement_id)",														0x539030,	0x543650},
	{OJ_DEFINE,	"ADDR_SMALL_WONDER_FREE_IMPROVS_RETURN",	"int",																						0x55A62E,	0x566538},
	{OJ_DEFINE,	"ADDR_RECOMPUTE_AUTO_IMPROVS_FILTER",		"void *",																					0x55A5C6,	0x5664D4},
	{OJ_REPL_VPTR,  "Main_Screen_Form_handle_key_up",		"int (__fastcall *) (Main_Screen_Form * this, int edx, int virtual_key_code)",													0x66B46C,	0x68856C},
	{OJ_DEFINE,	"Main_Screen_Form_issue_command",		"char (__fastcall *) (Main_Screen_Form * this, int edx, int command, Unit * unit)",												0x4DAA70,	0x4E3430},
	{OJ_DEFINE,	"clear_something_1",				"void (__stdcall *) ()",																			0x609D60,	0x6233C0},
	{OJ_DEFINE,	"clear_something_2",				"void (__fastcall *) (void * this)",																		0x6207B0,	0x644F10},
	{OJ_INLEAD,	"Leader_can_do_worker_job",			"char (__fastcall *) (Leader * this, int edx, enum Worker_Jobs job, int tile_x, int tile_y, int ask_if_replacing)",								0x55EFA0,	0x56B030},
	{OJ_REPL_VPTR,	"Unit_eval_escort_requirement",			"int (__fastcall *) (Unit *)",																			0x66DD04,	0x0},

	// {OJ_DEFINE,	"Leader_is_enemy_unit",				"char (__fastcall *) (Leader * this, int edx, Unit * unit)",															0x558F70,	0x0},
	// {OJ_DEFINE,	"p_trade_net",					"Trade_Net *",																					0xB72888,	0x0},
	// {OJ_DEFINE,	"Trade_Net_set_unit_path",			"int (__fastcall *) (Trade_Net * this, int edx, int from_x, int from_y, int to_x, int to_y, Unit * unit, int civ_id, int flags, int * param_8)",				0x580540,	0x0},
	{OJ_DEFINE,	"ADDR_CHECK_ARTILLERY_IN_CITY",			"void *",																					0x455288,	0x457458},
	{OJ_INLEAD,	"Unit_ai_move_artillery",			"void (__fastcall *) (Unit * this)",																		0x455140,	0x457310},
	// {OJ_DEFINE,	"Unit_ai_move_offensive_unit",			"void (__fastcall *) (Unit * this)",																		0x4507B0,	0x0},
	// {OJ_DEFINE,	"Unit_ai_eval_bombard_target",			"int (__fastcall *) (Unit * this, int edx, int tile_x, int tile_y, int param_3)",												0x44C340,	0x0},
	{OJ_DEFINE,	"neighbor_index_to_displacement",		"void (__cdecl *) (int neighbor_index, int * out_x, int * out_y)",														0x5E6E50,	0x5F66A0},
	{OJ_INLEAD,	"Unit_set_state",				"void (__fastcall *) (Unit * this, int edx, int new_state)",															0x5B3040,	0x5C1970},
	{OJ_DEFINE,	"Unit_set_escortee",				"void (__fastcall *) (Unit * this, int edx, int escortee)",															0x5B2F10,	0x5C1840},
	{OJ_DEFINE,	"p_rand_object",				"void *",																					0xA526B4,	0xA74EAC},
	{OJ_DEFINE,	"rand_int",					"int (__fastcall *) (void * this, int edx, int lim)",																0x60BAB0,	0x626440},
	// {OJ_DEFINE,	"Unit_pop_next_move_from_path",			"byte (__fastcall *) (Unit * this)",																		0x5B3290,	0x0},
	{OJ_INLEAD,	"City_ai_choose_production",			"void (__fastcall *) (City * this, int edx, City_Order * out)",															0x42C8A0,	0x42E430},
	{OJ_DEFINE,	"City_can_build_unit",				"byte (__fastcall *) (City * this, int edx, int unit_type_id, byte exclude_upgradable, int param_3, byte allow_kings)",								0x4C04E0,	0x4C7AB0},

	{OJ_DEFINE,	"ADDR_DISEMBARK_IMMOBILE_BUG_PATCH",		"void *",																					0x5C536D,	0x5D404E},
	{OJ_DEFINE,	"DISEMBARK_IMMOBILE_BUG_PATCH_ORIGINAL_OFFSET", "int",																						0xFFFF90FF,	0xFFFF8FDE},
	{OJ_INLEAD,	"Unit_disembark_passengers",			"int (__fastcall *) (Unit * this, int edx, int tile_x, int tile_y)",														0x5C5420,	0x5D4120},
	{OJ_DEFINE,	"p_null_tile",					"Tile *",																					0xCAA330,	0xCCCB98},
	{OJ_DEFINE,	"ADDR_HOUSEBOAT_BUG_PATCH",			"byte *",																					0x45A35F,	0x45C56F},
	{OJ_DEFINE,	"ADDR_HOUSEBOAT_BUG_PATCH_END",			"byte *",																					0x45A386,	0x45C596},

	{OJ_DEFINE, "Main_Screen_Form_show_map_message", "void (__fastcall *) (Main_Screen_Form * this, int edx, int tile_x, int tile_y, char * text_key, int param_4)", 0x4ED220, 0x0},

	// {OJ_DEFINE,	"City_get_turns_to_build",			"int (__fastcall *) (City * this, int edx, int order_type, int order_id, char param_3)",											0x4BFD40,	0x0},
	// {OJ_INLEAD,	"City_can_build_improvement",			"char (__fastcall *) (City * this, int edx, int i_improv, char param_2)",													0x4BFF80,	0x0},
};
