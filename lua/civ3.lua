
local civ3 = {}

local io = require("io")

local ffi = require("ffi")
local civ3_def_file = assert(io.open(_G["c3x_rel_dir"] .. "/lua/civ3_defs_for_lua.h", "rb")) -- c3x_rel_dir set by mod C code
ffi.cdef(civ3_def_file:read("all"))
civ3_def_file:close()

local function NextNeighborOf(neighborsOfListing, neighbor)
  local nOL = neighborsOfListing
  if nOL.index < 8 then
    nOL.index = nOL.index + 1
    local nX = ffi.new("int[1]", -1)
    local nY = ffi.new("int[1]", -1)
    ffi.C.get_neighbor_coords(nOL.map, nOL.centerX, nOL.centerY, nOL.index, nX, nY)
    return ffi.C.tile_at(nX[0], nY[0])
  end
  return nil
end

local function NeighborsOf(tileX, tileY)
  local bic = ffi.C.get_bic_data()
  local neighborsOfListing = { map = bic.map, index = 0, centerX = tileX, centerY = tileY }
  return NextNeighborOf, neighborsOfListing, nil
end

---@class Tile
---@field IsWater fun(this: Tile): boolean Whether this is a coast, sea, or ocean tile
---@field GetTileUnitID fun(this: Tile): integer
---@field GetTerrainBaseType fun(this: Tile): integer Returns the ID of the tile's base terrain type. The ID corresponds to an entry in the TerrainType table.
---@field SetTerrainType fun(this: Tile, terrainType: integer, x: integer, y: integer): nil Changes the tile's terrain. You must specify the tile's own X & Y coordinates as the last two parameters.
---@field GetCity fun(this: Tile): City | nil Returns the city on this tile or nil if there isn't any
local Tile
local Tile_metatable = {
  __index = {
    IsWater = function(this) return ffi.C.Tile_m35_Check_Is_Water(this) ~= 0 end,
    GetTileUnitID = function(this) return ffi.C.Tile_m40_get_TileUnit_ID(this) end,
    GetTerrainBaseType = function(this) return ffi.C.Tile_m50_Get_Square_BaseType(this) end,
    SetTerrainType = function(this, terrainType, x, y) ffi.C.Tile_m74_SetTerrainType(this, terrainType, x, y) end,
    GetCity = function(this) return ffi.C.get_city_ptr(this.cityID) end
  }
}
Tile = ffi.metatype("Tile", Tile_metatable)

---@class Unit
local Unit
local Unit_metatable = {
  __index = {
  }
}
Unit = ffi.metatype("Unit", Unit_metatable)

civ3.CitizenMood = {
  Happy = 0,
  Content = 1,
  Unhappy = 2,
  Rebel = 3
}

civ3.TerrainType = {
  Desert = 0,
  Plains = 1,
  Grassland = 2,
  Tundra = 3,
  FloodPlain = 4,
  Hills = 5,
  Mountains = 6,
  Forest = 7,
  Jungle = 8,
  Swamp = 9,
  Volcano = 10,
  Coast = 11,
  Sea = 12,
  Ocean = 13
}

function civ3.PopUpInGameError(msg) ffi.C.pop_up_in_game_error(msg) end
function civ3.GetC3XScriptPath() return ffi.C.get_c3x_script_path() end

--- @param x integer
--- @param y integer
--- @return Tile
function civ3.TileAt(x, y)
  return ffi.C.tile_at(x, y)
end

---@type MainScreenForm
civ3.mainScreenForm = ffi.C.get_main_screen_form()

local function NextUnit(unit, id)
  local lastIndex = ffi.C.get_p_units().lastIndex
  if (id ~= nil) and (id <= lastIndex) then
    while true do
      id = id + 1
      if id > lastIndex then break end
      local next = ffi.C.get_unit_ptr(id)
      if next ~= nil then return id, next end
    end
  end
end

function civ3.Units()
  return NextUnit, nil, -1
end

local function NextUnitOn(unit, index)
  local tileUnits = ffi.C.get_p_tile_units()
  if (index >= 0) and (index < tileUnits.lastIndex) then
    local item = tileUnits.items[index]
    return item.v, ffi.C.get_unit_ptr(item.object)
  end
end

local function UnitsOn(tile)
  local itemIndex = ffi.new("int[1]", -1)
  local unitID = ffi.C.TileUnits_TileUnitID_to_UnitID(ffi.C.get_p_tile_units(), tile:GetTileUnitID(), itemIndex)
  return NextUnitOn, ffi.C.get_unit_ptr(unitID), itemIndex[0]
end

local function NextCity(city, id)
  local lastIndex = ffi.C.get_p_cities().lastIndex
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
    if (next ~= nil) and (next.ownerID == cOL.civID) then return next end
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
  local cityOfListing = { id = -1, civID = civID, lastIndex = ffi.C.get_p_cities().lastIndex }
  return NextCityOf, cityOfListing, nil
end
--- @alias CitiesOf fun(civID: number): NextCityOf, any, City | nil

local function NextCitizenIn(citizenInListing, citizen)
  local cIL = citizenInListing
  while cIL.index <= cIL.lastIndex do
    cIL.index = cIL.index + 1
    local next = cIL.items[cIL.index].body
    if (next ~= nil) and (tonumber(ffi.cast("int", next)) ~= 28) then return next end
  end
  return nil
end

local function CitizensIn(city)
  local citizenInListing = { index = -1, items = city.citizenList.items, lastIndex = city.citizenList.lastIndex }
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
MainScreenForm = ffi.metatype("MainScreenForm", MainScreenForm_metatable)

---@class Leader
---@field Cities fun(): NextCityOf, any, City | nil Returns an iterator over this player's cities
local Leader

local Leader_metatable = {
  __index = {
    Cities = function(this) return CitiesOf(this.id) end
  }
}
Leader = ffi.metatype("Leader", Leader_metatable)

--- @class City
--- @field RecomputeHappiness fun(): nil
--- @field Citizens any
local City
local City_metatable = {
  __index = {
    RecomputeHappiness = function(this) ffi.C.City_recompute_happiness(this) end,
    ZoomTo = function(this) ffi.C.City_zoom_to(this) end,
    Citizens = function(this) return CitizensIn(this) end,
    GetName = function(this) return ffi.string(this.cityName, 20) end
  }
}
City = ffi.metatype("City", City_metatable)

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
