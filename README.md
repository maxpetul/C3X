
**C3X** is a mod that makes many improvements to the Civ 3 Conquests EXE. It adds quality of life features including stack unit commands (shown below) and an end-of-turn warning for cities about to riot. C3X also fixes many long standing bugs, including the infamous submarine bug, deepens the AI's ability to fight by enabling it to use artillery and army units, and reduces turn times by correcting a major inefficiency in the game's trade network calculation. C3X also forms a platform for other mods by enabling various gameplay changes not possible through the editor. Examples include resource generation from buildings, era-specific names for leaders and civs, limited trespassing and railroad movement like in Civ 4, and a broad expansion of the zone of control and defensive bombard abilities.

|![Demo of stack unit commands in C3X. Holding the control key causes all workers on a tile to build a railroad and causes all bombers to bombard a target.](Misc%20Images/new%20stack%20demo/output_gimp_with_msgs_optimized.gif)|
|:--:|
|Stack unit commands. Holding the control key converts most unit action buttons into stack buttons, which issue their commands to all units of the same type on the same tile. Similarly, holding control when selecting a tile to bombard performs a stack bombard. Also note the grouping of units and movement indicators on the right-click menus.|

C3X's primary home is on CivFanatics, see: [main mod page](https://forums.civfanatics.com/resources/c3x.28759/), [releases](https://forums.civfanatics.com/resources/c3x.28759/updates), [discussion thread](https://forums.civfanatics.com/threads/c3x-exe-mod-including-bug-fixes-stack-bombard-and-much-more.666881/latest)

## Video Overview
See this video by Suede for a demonstration of how to install the mod and of some of its convenience features.
[![C3X: Incredible Quality of Life mod for Civ 3, Video by Suede CivIII](http://img.youtube.com/vi/VxQ5dVABJcQ/0.jpg)](http://www.youtube.com/watch?v=VxQ5dVABJcQ)

## Installation Details
Extract the mod, keeping it in its own folder, then copy that folder to your main Conquests directory (i.e. the folder containing Civ3Conquests.exe). Then activate the mod by double-clicking the INSTALL.bat script. You should get a message reporting that the installation was successful. You can also try RUN.bat, which launches a modded version of Civ 3 without installation, however I have received several reports that that script doesn't work for some people.

Notes about installation:
1. If your Civ 3 is installed inside Program Files then it may be necessary to run INSTALL.bat as administrator due to Windows restrictions on editing the contents of Program Files.
2. When installing, the mod will create a backup of the original unmodded executable named "Civ3Conquests-Unmodded.exe".
3. To uninstall the mod, delete the modded executable then rename the backed up version mentioned above to "Civ3Conquests.exe".
4. It is not necessary to uninstall the mod before installing a different version.
5. Even after installation, the mod still depends on some files in the mod folder, so you need to keep it around.
6. Rômulo Prado reports that RUN.bat started working for him after he installed the MS Visual C++ Redistributables versions 2005 and 2019 (while installing GOG Galaxy).

## Configuration
All aspects of C3X are configurable through INI text files. The basic INI file is called "default.c3x_config.ini" and is located in the mod folder. For example, if you want to turn off grouping of units on the right-click menu (that's what gives you "3x Spearman" instead of 3 identical Spearman entries), you could open that file in any text editor, find the line `group_units_on_right_click_menu = true`, and set it to `false`.

However, because the default config file gets updated with each new release of the mod, it's recommended to put changes like the one above in a new file named "custom.c3x\_config.ini". C3X supports up to three different config files, the default config, a scenario config named "scenario.c3x\_config.ini" located in a scenario's search folder, and a custom config. They are loaded in that order. Scenario configs are intended to contain rule settings relevant to a particular scenario and custom configs are intended to function like user preferences. For a quick example of a scenario config, see [this post](https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/page-28#post-16212316).

## Compatibility
C3X is compatible with the GOG and Steam versions of Civ 3 Complete and also with the DRM-free executable available through PCGames.de. If you have a CD version of the game, you can replace its EXE with the one from PCGames.de then install C3X on top of that. For more info about the PCGames.de executable, see this thread: [Civ 3, Windows Update KB3086255, & SafeDisc](https://forums.civfanatics.com/threads/civ-3-windows-update-kb3086255-safedisc.552308/).

For info about running C3X on Mac, see this thread: [Installing, Playing and Modding C3C on Apple Silicon](https://forums.civfanatics.com/threads/installing-playing-and-modding-c3c-on-apple-silicon.681540/)

C3X is compatible with existing saves and saves created with the mod active will still be loadable by the base game.

Online multiplayer is not officially supported but some features of the mod will work. Others will not, including stack unit commands. I have also received reports that C3X can cause crashes in online MP. In general, online play is something I'd like to support but haven't gotten around to yet.

## Feature Highlights
#### Disorder Warning:
![C3X disorder warning feature. The domestic advisor is popping up to warn about unhappy cities before the end of a turn.](Misc%20Images/disorder_warning.jpg)

If you try to end the turn with unhappy cities, the domestic advisor will pop up to warn you and give you the option to continue that turn.

#### AI Enhancements:
Numerous changes have been made to improve the AI's behavior, especially in combat. It can now use its artillery units in the field, i.e., it will take them out of its cities to bombard enemy cities or incoming enemy units. The AI's production of artillery has been significantly increased so that it can take advantage of this ability. The other major change is that the AI can now use armies properly, it builds them when it can and fills them with units, usually the strongest available. There are many smaller changes as well to fix bugs and improve heuristics. Some more details are available as comments in the config file.

#### Trade Screen Improvements:
![C3X trade screen arrows. Shows the top of the leader window with arrows on either side.](Misc%20Images/c3x_trade_screen_arrows.jpg)

When negotiating with the AI, you can quickly switch back and forth between civs using the added arrow buttons (arrow keys work as well). When asking for or offering gold, the set amount popup will appear with the best amount already filled in. Best amount means, when asking for gold on an acceptable trade, the most you could get, and when offering gold on an unacceptable trade, the least you need to pay.

#### Optimization:
A major inefficiency in the game's sea trade computation has been fixed. This eliminates one major cause of slow turns in the late game, especially on large maps with many coastal cities and many wars. For details about the problem and how C3X solves it, see [this post](https://forums.civfanatics.com/threads/c3x-exe-mod-including-bug-fixes-stack-bombard-and-much-more.666881/page-83#post-16536108).

#### Adjustable Movement Rules:
The functions governing unit movement have been modified to enable various adjustments to the game's movement rules not possible through the editor. As with all other engine extensions, the rules are not changed from vanilla Conquests unless the config file is edited.

Unlimited railroad movement can be turned off. The non-unlimited railroads can function like they do in Civ 4, meaning that all units will move the same distance along them regardless of how many moves they have, or they can function like an upgraded version of regular roads that have a lower movement cost. The relevant config variables are "limit\_railroad\_movement" and "limited\_railroads\_work\_like\_fast\_roads".

Enabling land/sea intersections allows sea units to travel over the thin isthmus that exists on the diagonal path between two land tiles. More specifically, imagine a diamond of four tiles with land terrain on the north & south tiles and water on the east & west ones. With land/sea intersections enabled, naval units can pass between the east and west tiles.

It is possible to disallow trespassing, which will prevent units from entering another civ's borders without a right of passage agreement, similar to the rules in Civ 4. Invisible and hidden nationality units are allowed to trespass in any case. It is also possible to impose a stack limit, which will prevent units from entering tiles that already have more than a set number of units occupying them. The stack limit can be set separately for land, sea, and air units.

#### Small Wonder Free Improvements:
The free improvements wonder effect (granaries from Pyramids etc.) now works on small wonders. Note to modders, to set this effect you must use a third party editor like Quintillus' ([thread link](https://forums.civfanatics.com/threads/cross-platform-editor-for-conquests-now-available.377188/)) because the option is grayed out in the standard editor. Even worse, if you set the effect then work on the BIQ in the standard editor, the effect won't be saved, so you'd need to set it every time or work exclusively in Quintillus' editor.

#### No-Raze & No Unit Limit:
The no-raze and no-unit-limit features of earlier modded EXEs have been re-implemented as part of C3X. To enable no-raze, edit the config. There are separate options to prevent autorazing (the forced destruction of size 1 cities) and razing by player's choice.

## Full Feature List
All C3X features are listed below. See the default config (default.c3x\_config.ini) for descriptions.
<details>
  <summary>Included in C3X</summary>

  #### Convenience features
   - Stack unit commands
      - Stack bombard
      - Worker buttons (irrigate, road, etc.) become stack buttons by holding CTRL
      - Stack fortify, upgrade, and disband also with CTRL
   - Disorder warning
   - Detailed city production info
   - Buttons on trade screen to quickly switch between civs
   - Ask/offer gold popup autofills best amount
   - Skip repeated popups asking to replace a tile improvement
   - Group units on right click menu
   - Show coordinates and chopped status in tile info box
   - Show golden age turns remaining
   - No special king unit names in non-regicide games
   - Option to disable worker automation
   - On the city screen, hold shift when clicking a specialist to switch to the previous type
   - Automatically cut research spending to avoid bankruptcy
   - Remove pause for "we love the king" messages
   - Suppress "maximum hypertext links exceeded" popup
   - Civilopedia indicates when units go obsolete but cannot be upgraded
   - Message appears after bomber dodges interception by air defense buildings
   - Option to replay AI moves for all human players in hotseat mode
   - Restore unit directions on game load
   - Option to remove Elvis Easter egg
   - Harbor/airport city icons indicate unit effects not trade abilities
   - Disallow useless bombard attacks vs airfields
   - Display total city count (disabled by default, appears on demographics screen)
   - Fix graphical issues when running on Wine
   - Option to pack the lists of luxuries and strategic resources more tightly into their boxes on the city screen
   - Right-click menu enhancements
      - Place icons next to units showing movement and combat status
      - Replace Wake/Activate with descriptions of what the units are doing
      - Gray out units if they have no remaining moves
   - Apply GridOn setting from conquests.ini after loading a save
   - Option to have a warning when the building you've selected to build would replace another already built in the city
   - Option not to unassign workers from tiles that become polluted
   - Pressing the Z key on the city screen toggles the zoom level of the map display
   - Option not to draw capital cities larger than they really are, named do_not_make_capital_cities_appear_larger
  #### Optimization
   - Optimize computation of trade networks
      - For details, see the info text file in the Trade Net X folder
   - Optimize improvement loops
   - Option to measure turn times
  #### AI Enhancements
   - Allow AI to use artillery in the field
   - Force AI to build more artillery and bombers
   - Replace leader unit AI to fix bugs and improve behavior
   - Fix bug preventing AI from filling its armies
   - Improve AI army composition to discourage mixing types & exclude HN units
   - AI routine for "pop units", which are modded units whose only purpose is to be joined into cities
   - AI routine for "caravan units", which are modded units whose only purpose is to be disbanded for shields
   - Can limit the number of escorts the AI assigns to its naval transports and carriers
   - Adjustable AI worker requirement
   - Option to stop AI from escorting units without the "requires escort" flag
  #### Bugs Fixed
   - AI pathfinding collides with invisible units (called the "submarine bug")
   - Science age beakers not actually awarded
   - Pink line in Civilopedia
   - Crash when doing disembark-all on transport containing immobile unit(s)
   - Crash possible when AI civ is left alive with only a settler on a transport (called the "houseboat bug")
   - Resources beyond the first 32 share access records in cities not on the main trade network (called the "phantom resource bug")
   - Air units lose a turn after being set to intercept
   - Cached building maintenance amounts not updated when buildings are obsoleted
   - Barbarian long-range search for targets is limited to tiles directly NW or SE
   - "Disables Diseases From Flood Plains" tech flag hardcoded to tech #8 (off by default)
   - Possible division by zero in AI logic to evaluate proposed alliances
   - Available movement computed incorrectly for empty armies
   - Off-map AI units may crash the game (fixed by deleting them at the start of their turns)
   - Pathfinder improperly truncates long paths, sending AI units on nonsensical or invalid paths
   - Cities with zero production crash the game due to division by zero
   - Icons for different kinds of specialist yields are drawn on top of, instead of next to, one another
  #### AMB Editor
   - A program for inspecting and modifying the special .amb sound files used by Civ 3.
   - For more info, see README.txt in the AMB Editor folder
  #### Engine Extensions
   - Adjustable minimum city distance
   - Option to limit railroad movement, as in Civ 4 or by converting them to fast roads
   - Removed unit limit
   - Removed city improvement limit
   - Option to limit how many units can share each tile
   - Enable free improvements from small wonders
   - Option to share visibility among all human players in a hotseat game
   - Option to prevent autoraze and razing by players
   - Trespassing prevention
   - Land/sea intersections
   - Adjustable anarchy length
   - Unit limits (stops players from producing units of a given type once they reach a maximum quantity)
   - "Perfume" city production options, technologies, and governments to control how likely the AI is to choose them
   - Reveal AI logic
      - Press P in city screen to see AI point value for each available build
      - Press L on map to see how desirable the AI finds each tile as a city location
   - Corruption can be completely removed with "OFF" government setting
   - Disallow land units from working or settling water tiles
   - Option to let units move after airdropping
   - Buildings can generate resources
   - Buildings can be set as prerequisites for unit production
   - Can cancel out pop pollution with negative pollution amount on building flagged as removing pop pollution
   - Option to modify rules for retreat eligibility
   - AI multi-city start
      - Starter cities can begin with improvements, including "extra palaces" which respawn like the real palace
   - Remove cap on turn limit
   - Option to strengthen forbidden palace decorruption effect to match the palace's
   - Option to allow military great leaders to hurry wonders
   - Option to multiply AI research rate by any amount
   - Option to aggressively penalize bankrupt players
   - Option to remove exception to tile penalty for city tiles with fresh water and Agri trait
   - Artillery can be set to use PTW-like targeting against cities
   - Recon missions can be made vulnerable to interception
   - Option to charge one move for recon missions and interception
   - Stealth attack changes
      - Option to perform stealth attack even when there's only one target
      - Enable stealth attacks via bombardment
      - Allow players to opt out of stealth attacks
      - Option to show unit hitpoints on the stealth attack target selection popup
      - Option to prevent stealth attacks from targeting units that are invisible and unrevealed
   - Polish precision striking by land or sea units
      - Use regular bombard animation instead of flying animation
      - Use bombard range instead of operational range
      - Despawn unit if cruise missile
      - Cannot be intercepted
   - Option to immunize aircraft against bombardment
   - Option to ignore king flag on defense, so kings aren't always last to defend in a stack
   - Option to show untradable techs on trade screen
   - Barbarian city capture & production (experimental)
   - Option to allow land units to bombard aircraft and naval units in cities
   - Zone of control changes
      - Allow land-to-sea and sea-to-land attacks, only using bombard stat
      - May be lethal
      - May be exerted by air units
      - Show attack animation even when attacker is not at the top of its stack
   - Defensive bombard changes
      - May be lethal
      - May be performed by air units
      - Invisible, undetected units may be made immune
      - May be performed multiple times per turn with blitz
      - Naval units in a city may perform defensive bombard vs land attackers
   - Allow precision strikes to target tile improvements
   - Option not to end a unit's turn after it bombards a barricade
   - Option to allow bombardment of other improvements on a tile with an occupied airfield
   - Option to boost OCN increase from forbidden palaces in non-communal governments
   - Option to allow airdrops without airports
   - Can increase unit maintenance costs based on their build costs
   - Civ and leader names can vary by era
   - Option to allow upgrades in any city
   - Option to stop the map generator from placing volcanos
   - Option to stop pollution from appearing on impassable tiles
   - Option to make planting forests produce LM forest terrain
   - Adjustable chance for max one HP units to be destroyed by nuclear strikes
   - Option to allow sale of buildings like aqueducts that uncap population growth
   - Option stopping non-sea detector units from revealing invisible sea units and vice-versa
   - Adjustable city work area size
      - Radius of area can be set from 1 to 7 tiles (2 is the standard)
      - Area can also be limited by a city's cultural level
   - Option to throttle AI's expansion by temporarily applying perfume to settlers each time it founds a city
   - Option to block the galley chaining exploit by preventing units from loading into two different transports on the same turn
   - Adjustable rebase range as multiple of operational range
   - Option to share wonders among human players in hotseat mode
   - Experimental option to move the game's trade net object to a different location in memory
</details>

## How It Works
Some parts of the mod (bug fixes, no-raze, no unit limit) are really just hex edits that are applied to the Civ program code. The real secret sauce is a system to compile and inject arbitrary C code into the process which makes it practical to implement new features in the game. The heart of the system is TCC (Tiny C Compiler) and much work puzzling out the functions and structs inside the executable. Much thanks to Antal1987 for figuring out most of the structs years before I came along, [his work is posted here](https://github.com/Antal1987/C3CPatchFramework)).

C3X is open source. The C code that gets injected into the game's EXE is located in injected_code.c and the code to perform the injection is located in ep.c. You're invited to explore the source code if you're interested.

## Special Thanks
1. Antal1987 for his work reverse engineering Civ3
2. Rômulo Prado for his help testing the mod
3. Civinator for the German translation. See: https://www.civforum.de/showthread.php?113285-Der-Flintlock-Deutsch-Patch
4. Vaughn Parker for generously commissioning the port to the PCGames.de EXE and many other features
