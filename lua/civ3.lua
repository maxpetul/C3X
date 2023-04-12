
local civ3 = {}

local ffi = require("ffi")
ffi.cdef[[
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
]]

civ3.CitizenMood = {
  Happy = 0,
  Content = 1,
  Unhappy = 2,
  Rebel = 3
}

function civ3.PopUpInGameError(msg) ffi.C.pop_up_in_game_error(msg) end
function civ3.GetC3XScriptPath() return ffi.C.get_c3x_script_path() end

---@type MainScreenForm
civ3.mainScreenForm = ffi.C.get_main_screen_form()

local function NextCity(city, id)
  local lastIndex = ffi.C.get_p_cities().LastIndex
  if (id ~= nil) and (id <= lastIndex) then
    while true do
      id = id + 1
      if id > lastIndex then break end
      local next = ffi.C.get_city_ptr(id)
      if next ~= nil then return id, next end
    end
  end
end

function civ3.Cities()
  return NextCity, nil, -1
end

local function NextCityOf(cityOfListing, city)
  local cOL = cityOfListing
  while cOL.id <= cOL.lastIndex do
    cOL.id = cOL.id + 1
    local next = ffi.C.get_city_ptr(cOL.id)
    if (next ~= nil) and (next.Body.OwnerID == cOL.civID) then return next end
  end
  return nil
end
--- @alias NextCityOf fun(cityOfListing: any, city: City): City | nil

--- Returns an iterator over cities belonging to this civ civID
--- @param civID number
--- @return NextCityOf
--- @return any
--- @return City | nil
local function CitiesOf(civID)
  local cityOfListing = { id = -1, civID = civID, lastIndex = ffi.C.get_p_cities().LastIndex }
  return NextCityOf, cityOfListing, nil
end
--- @alias CitiesOf fun(civID: number): NextCityOf, any, City | nil

local function NextCitizenIn(citizenInListing, citizen)
  local cIL = citizenInListing
  while cIL.index <= cIL.lastIndex do
    cIL.index = cIL.index + 1
    local next = cIL.items[cIL.index].Body
    if (next ~= nil) and (tonumber(ffi.cast("int", next)) ~= 28) then return next end
  end
  return nil
end

local function CitizensIn(city)
  local citizenInListing = { index = -1, items = city.Body.citizenList.Items, lastIndex = city.Body.citizenList.LastIndex }
  return NextCitizenIn, citizenInListing, nil
end

---@class MainScreenForm
---@field GetController fun(): Leader Returns the Leader object corresponding to the seated player
local MainScreenForm

local MainScreenForm_metatable = {
  __index = {
    GetController = function() return ffi.C.get_ui_controller() end
  }
}
MainScreenForm = ffi.metatype("MainScreenForm_t", MainScreenForm_metatable)

---@class Leader
---@field Cities fun(): NextCityOf, any, City | nil Returns an iterator over this player's cities
local Leader

local Leader_metatable = {
  __index = {
    Cities = function(this) return CitiesOf(this.ID) end
  }
}
Leader = ffi.metatype("Leader_t", Leader_metatable)

--- @class City
--- @field RecomputeHappiness fun(): nil
--- @field Citizens any
local City
local City_metatable = {
  __index = {
    RecomputeHappiness = function(this) ffi.C.City_recompute_happiness(this) end,
    ZoomTo = function(this) ffi.C.City_zoom_to(this) end,
    Citizens = function(this) return CitizensIn(this) end,
    GetName = function(this) return ffi.string(this.Body.CityName, 20) end
  }
}
City = ffi.metatype("City_t", City_metatable)

--- Opens a popup window
--- @param scriptPath string Path to the script.txt file containing the text key
--- @param textKey string Which text key to use
--- @return integer choice If the popup contains multiple choices, this is the zero-based index of the player's choice
function civ3.ShowPopup(scriptPath, textKey, ...)
  local popup = ffi.C.get_popup_form()
  for n = 1,select("#", ...) do
    local param = select(n, ...)
    if type(param) == "string" then
      ffi.C.set_popup_str_param(n - 1, param, -1, -1)
    elseif type(param) == "number" then
      ffi.C.set_popup_int_param(n - 1, param)
    end
  end
  ffi.C.PopupForm_set_text_key_and_flags(popup, scriptPath, textKey, -1, 0, 0, 0)
  return ffi.C.show_popup(popup, 0, 0)
end

return civ3
