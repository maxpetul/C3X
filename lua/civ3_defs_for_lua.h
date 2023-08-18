typedef struct s_MainScreenForm
{
  int vtable;
  int field_4[4974];
  int playerCivID;
  int field_4DC0[42230];
  char turnEndFlag;
  char field_2E199[3];
  int field_2E19C[1731];
} MainScreenForm;

typedef struct s_Citizen_Body
{
  int vtable;
  int field_20[66];
  int mood;
  int gender;
  int field_130;
  int field_134;
  int field_138;
  int workerType;
  int raceID;
  int field_144;
  int field_148;
} Citizen_Body;

typedef struct s_Citizen_Info
{
  int field_0;
  Citizen_Body *body;
} Citizen_Info;

typedef struct s_CitizenList
{
  int vtable;
  Citizen_Info *items;
  int field_8;
  int field_C;
  int lastIndex;
  int capacity;
} CitizenList;

typedef struct s_City_Body
{
  int field_0[3];
  char ownerID;
  char field_D[3];
  int field_10[44];
  CitizenList citizenList;
  int field_D8[59];
  char cityName[20];
  int field_1D8[212];
} City_Body;

typedef struct s_City
{
  int field_0[7];
  City_Body body;
} City;

typedef struct s_CityItem
{
  int field_0;
  City_Body *city;
} CityItem;

typedef struct s_Cities
{
  int vtable;
  CityItem *cities;
  int v1;
  int v2;
  int lastIndex;
  int capacity;
} Cities;

typedef struct s_Leader
{
  int vtable;
  int field_4[6];
  int id;
  int field_20[2097];
} Leader;

void pop_up_in_game_error(char const * msg);
Cities * get_p_cities();
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
