
local civ3 = require("civ3")

function InterceptEndOfTurn()
  local cityCount = 0
  for city in civ3.GetUIController():Cities() do
    cityCount = cityCount + 1
  end

  local cityCount2 = 0
  for _, city in civ3.Cities() do
    cityCount2 = cityCount2 + 1
  end

  civ3.PopUpInGameError("City count A: " .. cityCount .. ", B: " .. cityCount2);
end
