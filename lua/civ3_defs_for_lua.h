
typedef unsigned char byte;

// **************************** //
// BEGIN AUTO-GENERATED SECTION //
// **************************** //

// Do not edit. This section was generated automatically by generator.py.

typedef struct s_MainScreenForm
{
	byte _opaque_0[19900];
	int playerCivID;
	byte _opaque_1[168920];
	char turnEndFlag;
	byte _opaque_2[6927];
} MainScreenForm;

typedef struct s_CitizenBody
{
	byte _opaque_0[268];
	int mood;
	byte _opaque_1[32];
} CitizenBody;

typedef struct s_CitizenItem
{
	byte _opaque_0[4];
	CitizenBody * body;
} CitizenItem;

typedef struct s_CitizenList
{
	byte _opaque_0[4];
	CitizenItem * items;
	byte _opaque_1[8];
	int lastIndex;
	int capacity;
} CitizenList;

typedef struct s_City
{
	byte _opaque_0[40];
	char ownerID;
	byte _opaque_1[176];
	CitizenList citizenList;
	byte _opaque_2[236];
	char cityName;
	byte _opaque_3[848];
} City;

typedef struct s_CityBody
{
	byte _opaque_0[1320];
} CityBody;

typedef struct s_CityItem
{
	byte _opaque_0[4];
	CityBody * city;
} CityItem;

typedef struct s_CityList
{
	byte _opaque_0[4];
	CityItem * cityList;
	byte _opaque_1[8];
	int lastIndex;
	int capacity;
} CityList;

typedef struct s_Leader
{
	byte _opaque_0[28];
	int id;
	byte _opaque_1[8388];
} Leader;

typedef struct s_Tile
{
	byte _opaque_0[5];
	char territoryOwnerID;
	int resourceType;
	int tileUnitID;
	byte _opaque_1[10];
	short cityID;
	short tileBuildingID;
	short continentID;
	byte _opaque_2[8];
	int overlays;
	int squareType;
	byte _opaque_3[60];
	short cityAreaID;
	byte _opaque_4[110];
} Tile;



// ************************** //
// END AUTO-GENERATED SECTION //
// ************************** //

void pop_up_in_game_error(char const * msg);
CityList * get_p_cities();
City * get_city_ptr(int id);
Leader * get_ui_controller();
void __thiscall City_recompute_happiness(City * this);
void __thiscall City_zoom_to(City * this);
char * get_c3x_script_path();
MainScreenForm * get_main_screen_form();

void * __stdcall get_popup_form();
int __cdecl set_popup_str_param(int param_index, char const * str, int param_3, int param_4);
int __cdecl set_popup_int_param(int param_index, int value);
int __thiscall show_popup(void * this, int param_1, int param_2);
void __thiscall PopupForm_set_text_key_and_flags(void * this, char const * script_path, char const * text_key, int param_3, int param_4, int param_5, int param_6);
