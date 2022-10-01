
local civ3 = require("civ3")

function InterceptEndOfTurn()
  local numUnhappyCities, firstUnhappyCity = 0, nil
  for city in civ3.mainScreenForm:GetController():Cities() do
    city:RecomputeHappiness()
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

  local prevTurnEndFlag = civ3.mainScreenForm.turnEndFlag

  local response
  response = civ3.ShowPopup(civ3.GetC3XScriptPath(), "C3X_DISORDER_WARNING_MULTIPLE", "Testing", 123)
  if response == 1 then
    civ3.mainScreenForm.turnEndFlag = true
  end
  if firstUnhappyCity ~= nil then
  end
end
