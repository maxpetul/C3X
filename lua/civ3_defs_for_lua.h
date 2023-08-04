typedef struct MainScreenForm
{
  int vtable;
  int field_4[4974];
  int playerCivID;
  int field_4DC0[42230];
  char turnEndFlag;
  char field_2E199[3];
  int field_2E19C[1731];
} MainScreenForm_t;

typedef struct Citizen_Body
{
  int vtable;
  int field_20[66];
  int Mood;
  int Gender;
  int field_130;
  int field_134;
  int field_138;
  int WorkerType;
  int RaceID;
  int field_144;
  int field_148;
} Citizen_Body_t;

typedef struct Citizen_Info
{
  int field_0;
  Citizen_Body_t *Body;
} Citizen_Info_t;

typedef struct CitizenList
{
  int vtable;
  Citizen_Info_t *Items;
  int field_8;
  int field_C;
  int LastIndex;
  int Capacity;
} CitizenList_t;

typedef struct City_Body
{
  int field_0[3];
  char OwnerID;
  char field_D[3];
  int field_10[44];
  CitizenList_t citizenList;
  int field_D8[59];
  char CityName[20];
  int field_1D8[212];
} City_Body_t;

typedef struct City
{
  int field_0[7];
  City_Body_t Body;
} City_t;

typedef struct CityItem
{
  int field_0;
  City_Body_t *City;
} CityItem_t;

typedef struct Cities
{
  int vtable;
  CityItem_t *Cities;
  int V1;
  int V2;
  int LastIndex;
  int Capacity;
} Cities_t;

typedef struct Leader
{
  int vtable;
  int field_4[6];
  int ID;
  int field_20[2097];
} Leader_t;

void pop_up_in_game_error(char const * msg);
Cities_t * get_p_cities();
City_t * get_city_ptr(int id);
Leader_t * get_ui_controller();
void __thiscall City_recompute_happiness(City_t * this);
void __thiscall City_zoom_to(City_t * this);
char * get_c3x_script_path();
MainScreenForm_t * get_main_screen_form();

void * __stdcall get_popup_form();
int __cdecl set_popup_str_param(int param_index, char const * str, int param_3, int param_4);
int __cdecl set_popup_int_param(int param_index, int value);
int __thiscall show_popup(void * this, int param_1, int param_2);
void __thiscall PopupForm_set_text_key_and_flags(void * this, char const * script_path, char const * text_key, int param_3, int param_4, int param_5, int param_6);
