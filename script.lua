
local civ3 = require("civ3")

function InterceptEndOfTurn()
  local numUnhappyCities, firstUnhappyCity = 0, nil
  for city in civ3.GetUIController():Cities() do
    -- recompute happiness
    local numHappy, numUnhappy = 0, 0
    for citizen in city:Citizens() do
      if citizen.Mood == civ3.CitizenMood.Happy then
        numHappy = numHappy + 1
      end
      if citizen.Mood == civ3.CitizenMood.Unhappy then
        numUnhappy = numUnhappy + 1
      end
    end
    if numUnhappy > numHappy then
      numUnhappyCities = numUnhappyCities + 1
      if firstUnhappyCity == nil then firstUnhappyCity = city end
    end
  end

  if firstUnhappyCity ~= nil then
    civ3.PopUpInGameError("At least one city is unhappy")
  end
end
