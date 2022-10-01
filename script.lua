
local civ3 = require("civ3")

function CheckHappinessAtEndOfTurn()
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


  if firstUnhappyCity ~= nil then
    local response
    if numUnhappyCities > 1 then
      response = civ3.ShowPopup(civ3.GetC3XScriptPath(), "C3X_DISORDER_WARNING_MULTIPLE", firstUnhappyCity:GetName(), numUnhappyCities)
    else
      response = civ3.ShowPopup(civ3.GetC3XScriptPath(), "C3X_DISORDER_WARNING_ONE", firstUnhappyCity:GetName())
    end

    if response == 2 then -- zoom to city
      civ3.mainScreenForm.turnEndFlag = true
      firstUnhappyCity:ZoomTo()
    elseif response == 1 then -- just cancel turn end
      civ3.mainScreenForm.turnEndFlag = true
    -- else do nothing, let turn end
    end
  end
end
