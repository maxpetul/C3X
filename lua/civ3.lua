
local civ3 = {}

local ffi = require("ffi")
ffi.cdef[[
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
  int field_D8[275];
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
]]

function civ3.GetUIController() return ffi.C.get_ui_controller() end
function civ3.PopUpInGameError(msg) ffi.C.pop_up_in_game_error(msg) end

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

local function CitiesOf(civID)
  local cityOfListing = { id = -1, civID = civID, lastIndex = ffi.C.get_p_cities().LastIndex }
  return NextCityOf, cityOfListing, nil
end

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

local Leader
local Leader_metatable = {
  __index = {
    Cities = function(this) return CitiesOf(this.ID) end
  }
}
Leader = ffi.metatype("Leader_t", Leader_metatable)

local City
local City_metatable = {
  __index = {
    Citizens = function(this) return CitizensIn(this) end
  }
}
City = ffi.metatype("City_t", City_metatable)

return civ3
