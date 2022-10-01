ffi = require("ffi")
ffi.cdef[[
typedef struct City_Body
{
  int field_0[3];
  char OwnerID;
  char field_D[3];
  int field_10[326];
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

function NextCity(city, id)
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

function Cities()
  return NextCity, nil, -1
end

function NextCityOf(cityOfListing, city)
  local cOL = cityOfListing
  while cOL.id <= cOL.lastIndex do
    cOL.id = cOL.id + 1
    local next = ffi.C.get_city_ptr(cOL.id)
    if (next ~= nil) and (next.Body.OwnerID == cOL.civID) then return next end
  end
  return nil
end

function CitiesOf(civID)
  local cityOfListing = { id = -1, civID = civID, lastIndex = ffi.C.get_p_cities().LastIndex }
  return NextCityOf, cityOfListing, nil
end

function InterceptEndOfTurn()
  local cityCount = 0
  for id = 0,(ffi.C.get_p_cities().LastIndex + 1) do
    local city = ffi.C.get_city_ptr(id)
    if city ~= nil then
      cityCount = cityCount + 1
    end
  end

  cityCount = 0
  for city in CitiesOf(ffi.C.get_ui_controller().ID) do
    cityCount = cityCount + 1
  end

  local cityCount2 = 0
  for _, city in Cities() do
    cityCount2 = cityCount2 + 1
  end


  -- for city in uiController.Cities() do
  -- end

  ffi.C.pop_up_in_game_error("City count A: " .. cityCount .. ", B: " .. cityCount2);
  -- return 7*6*5*4*3*2
end
