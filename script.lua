
local civ3 = require("civ3")

function InterceptEndOfTurn()
  local unhappyCount = 0
  for city in civ3.GetUIController():Cities() do
    for citizen in city:Citizens() do
      if citizen.Mood == 2 then -- CMT_Unhappy == 2
        unhappyCount = unhappyCount + 1
      end
    end
  end

  civ3.PopUpInGameError("Unhappy count: " .. unhappyCount);
end
