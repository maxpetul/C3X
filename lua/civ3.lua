
local civ3 = {}

local io = require("io")

local ffi = require("ffi")
local civ3_def_file = assert(io.open("C3X/lua/civ3_defs_for_lua.h", "rb"))
ffi.cdef(civ3_def_file:read("all"))
civ3_def_file:close()

-- **************************** --
-- BEGIN AUTO-GENERATED SECTION --
-- **************************** --

-- Do not edit. This section was generated automatically by generator.py.

--- @class Tile
local Tile
local Tile_metatable = {
  __index = {
  }
}
Tile = ffi.metatype("Tile", Tile_metatable)


-- ************************** --
-- END AUTO-GENERATED SECTION --
-- ************************** --

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
    if (next ~= nil) and (next.body.ownerID == cOL.civID) then return next end
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
  local citizenInListing = { index = -1, items = city.body.citizenList.items, lastIndex = city.body.citizenList.lastIndex }
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
    GetName = function(this) return ffi.string(this.body.cityName, 20) end
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
