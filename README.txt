C3X: Executable Mod for Civ 3 Complete
Release 20 Preview 2

INCLUDES (** = new in version 19):
Convenience features:
 - Stack unit commands
  - Stack bombard
   - ** Fix bombers not doing stack bombard and not showing as attack-capable on right-click menu
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
 - Option to pack the list of luxuries more tightly into the box on the city screen
 - Right-click menu enhancements
  - Place icons next to units showing movement and combat status
   - ** Rework UnitRCMIcons.pcx so busy & combat units can be given special icons
   - ** Put right-click menu sword icon on non-empty armies
  - Replace Wake/Activate with descriptions of what the units are doing
   - ** Describe the action of passenger units as "Transported" instead of "Fortified"
   - ** Describe the action of units waiting to fortify as "Fortified" instead of leaving it unreplaced
  - Gray out units if they have no remaining moves
  - ** Fix crash when opening right-click menu with fleeing automated workers
 - Apply GridOn setting from conquests.ini after loading a save
Optimization:
 - Optimize computation of trade networks
  - For details, see the info text file in the Trade Net X folder
 - Optimize improvement loops
 - Option to measure turn times
AI enhancements:
 - Allow AI to use artillery in the field
 - Force AI to build more artillery and bombers
 - Replace leader unit AI to fix bugs and improve behavior
 - Fix bug preventing AI from filling its armies
 - Improve AI army composition to discourage mixing types & exclude HN units
 - AI routine for "pop units" that may appear in mods
 - Can limit the number of escorts the AI assigns to its naval transports and carriers
 - Adjustable AI worker requirement
 - Option to stop AI from escorting units without the "requires escort" flag
  - ** Fix dont_escort_unflagged_units option interfering with group movement of units
Bugs fixed:
 - AI pathfinding collides with invisible units (called the "submarine bug")
 - Science age beakers not actually awarded
 - Pink line in Civilopedia
 - Crash when doing disembark-all on transport containing immobile unit(s)
 - Crash possible when AI civ is left alive with only a settler on a transport (called the "houseboat bug")
 - Resources beyond the first 32 share access records in cities not on the main trade network (called the "phantom resource bug")
 - Air units lose a turn after being set to intercept
 - Cached building maintenance amounts not updated when buildings are obsoleted
 - Barbarian long-range search for targets is limited to tiles directly NW or SE
Engine extensions:
 - Adjustable minimum city distance
 - Option to limit railroad movement
 - Removed unit limit
 - Enable free improvements from small wonders
 - Option to share visibility among all human players in a hotseat game
 - Option to prevent autoraze and razing by players
 - Stealth attack activates even when there's only one target
 - Trespassing prevention
 - Land/sea intersections
 - Adjustable anarchy length
 - Unit limits (stops players from producing units of a given type once they reach a maximum quantity)
 - "Perfume" units or improvements to control how likely the AI is to build them
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
 - Enable stealth attacks via bombardment
 - Artillery can be set to use PTW-like targeting against cities
 - Recon missions can be made vulnerable to interception
 - Option to charge one move for recon missions and interception
 - Allow players to opt out of stealth attacks
 - Polish precision striking by land or sea units
  - Use regular bombard animation instead of flying animation
  - Use bombard range instead of operational range
  - Cannot be intercepted
 - Option to immunize aircraft against bombardment
 - Option to ignore king flag on defense, so kings aren't always last to defend in a stack
 - Option to show untradable techs on trade screen
 - Barbarian city capture & production (experimental)
 - Option to allow land units to bombard aircraft and naval units in cities
 - Zone of control changes
  - Allow land-to-sea and sea-to-land attacks, only using bombard stat
  - May be lethal
   - ** Fix lethal ZoC setting allowing coastal fortresses to knock the last hit point off passing units
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
 - ** Option to stop pollution from appearing on impassable tiles

INSTALLATION AND USAGE:
Extract the mod to its own folder then copy that folder into your Civ install directory (i.e. the folder containing Civ3Conquests.exe). Then activate the mod by double-clicking the INSTALL.bat or RUN.bat scripts. INSTALL.bat will install the mod into Civ3Conquests.exe, RUN.bat will launch Civ 3 then apply the mod to the program in memory. The mod's behavior is highly adjustable by editing the config file named "default.c3x_config.ini". Also that config file contains info about some mod features that aren't fully explained in this README.

Notes about installation:
1. If your Civ 3 is installed inside Program Files then it's necessary to run INSTALL.bat as administrator. This is because admin permissions are required to edit anything within the Program Files directory.
2. When installing, the mod will create a backup of the original unmodded executable named "Civ3Conquests-Unmodded.exe".
3. To uninstall the mod, delete the modded executable then rename the backed up version mentioned above to "Civ3Conquests.exe".
4. It is not necessary to uninstall the mod before installing a different version.
5. Even after installation, the mod still depends on some files in the mod folder, so you need to keep it around.
6. I've received multiple reports that RUN.bat doesn't work while installing does, so know that installation is the more reliable option.
7. R�mulo Prado reports that RUN.bat started working for him after he installed the MS Visual C++ Redistributables versions 2005 and 2019 (while installing GOG Galaxy).

COMPATIBILITY:
- The mod is compatible with the GOG and Steam versions of Civ 3 Complete, and with the DRM-free executable available through PCGames.de.
- It works with existing save files and saves made with the mod active will still work in the base game.
- Multiplayer is not officially supported but some features will work in MP, see this post for more details: https://forums.civfanatics.com/thre...s-in-exe-modding.666881/page-16#post-16126470.
- The mod can work on Mac OS, see instructions here: https://forums.civfanatics.com/thre...rd-and-much-more.666881/page-56#post-16369665.
- For more info about the PCGames.de executable, see here: https://forums.civfanatics.com/threads/civ-3-windows-update-kb3086255-safedisc.552308/.

STACK BOMBARD:
Activate stack bombard on any unit capable of bombarding by clicking the stack bombard button or by activating normal bombard then CTRL+clicking the target tile. The selected unit will bombard the tile, then all other units of the same type on the same tile will automatically bombard the target as well. Stack bombard is pretty smart and will stop bombarding once it can no longer do any damage, and it knows about lethal bombard, that you can't damage air units in an airfield, etc.

OTHER STACK UNIT COMMANDS:
Hold the control key to turn most unit command buttons into stack buttons that when pressed will issue a command to all units of the same type on the same tile. This works for fortify, upgrade, and disband, as well as all worker commands.

DISORDER WARNING:
If you try to end the turn with unhappy cities, the domestic advisor will pop up to warn you and give you the option to continue that turn.

AI ENHANCEMENTS:
Numerous changes have been made to improve the AI's behavior, especially in combat. It can now use its artillery units in the field, i.e., it will take them out of its cities to bombard enemy cities or incoming enemy units. The AI's production of artillery has been significantly increased so that it can take advantage of this ability. The other major change is that the AI can now use armies properly, it builds them when it can and fills them with units, usually the strongest available. There are many smaller changes as well to fix bugs and improve heuristics. Some more details are available as comments in the config file.

TRADE SCREEN IMPROVEMENTS:
When negotiating with the AI, you can quickly switch back and forth between civs using the added arrow buttons (arrow keys work as well). When asking for or offering gold, the set amount popup will appear with the best amount already filled in. Best amount means, when asking for gold on an acceptable trade, the most you could get, and when offering gold on an unacceptable trade, the least you need to pay.

ADJUSTABLE MOVEMENT RULES:
The functions governing unit movement have been modified to enable various adjustments to the game's movement rules not possible through the editor. As with all other engine extensions, the rules are not changed from vanilla Conquests unless the config file is edited.

Railroad movement can be limited to a certain number of tiles by setting the "limit_railroad_movement" variable. The limitation works like in Civ 4, i.e., moving along a railroad consumes movement points like moving along a road except the cost of moving along a railroad is scaled by the unit's total moves so all units are limited to the same distance. Be advised, because this setting affects how movement is measured, changing it in the middle of a turn (i.e. after some units have already moved) is likely to cause units to have extra or missing moves.

Enabling land/sea intersections allows sea units to travel over the thin isthmus that exists on the diagonal path between two land tiles. More specifically, imagine a diamond of four tiles with land terrain on the north & south tiles and water on the east & west ones. With land/sea intersections enabled, naval units can pass between the east and west tiles.

Disallowing trespassing prevents civs from entering each other's borders while at peace without a right of passage, similar to the rules in Civ 4. Invisible and hidden nationality units are exempted from the restriction.

PER-SCENARIO CONFIG:
Mod config settings can be set on a per-scenario basis. To do this, create a "scenario.c3x_config.ini" file inside the scenario's folder (the same place that would contain the scenario's Art and Text folders) and fill it in like the "default.c3x_config.ini" file included with the mod. The scenario settings will be layered on top of the default settings so you only need to include the ones that are relevant to the scenario. You can verify in-game that the scenario config was loaded by checking the mod info popup, it's accessible through the button on the top right of the preferences screen. For an example, see: https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/page-28#post-16212316.

SMALL WONDER FREE IMPROVEMENTS:
The free improvements wonder effect (granaries from Pyramids etc.) now works on small wonders. Note to modders, to set this effect you must use a third party editor like Quintillus' (https://forums.civfanatics.com/threads/cross-platform-editor-for-conquests-now-available.377188/) because the option is grayed out in the standard editor. Even worse, if you set the effect then work on the BIQ in the standard editor, the effect won't be saved, so you'd need to set it every time or work exclusively in Quintillus' editor.

ADJUSTABLE MIN CITY DISTANCE:
The minimum allowed distance between cities can be changed by modifying the minimum_city_separation config option. A value of 1 corresponds to the standard game rules. A value of zero allows cities to be founded directly adjacent to one another. Values greater than 1 force cities to be founded farther apart than normal. Additionally, the option disallow_founding_next_to_foreign_city can be used to disallow founding cities next to those of other civs even when the min separation is zero.

NO-RAZE:
The "NoRaze" mod has been re-implemented inside C3X. To enable it, edit the config file mentioned above. There are separate options to prevent autorazing and razing by player's choice.

HOW IT WORKS:
Some parts of the mod (bug fixes, no-raze, no unit limit) are really just hex edits that are applied to the Civ program code. The real secret sauce is a system to compile and inject arbitrary C code into the process which makes it practical to implement new features in the game. The heart of the system is TCC (Tiny C Compiler) and much work puzzling out the functions and structs inside the executable (and thanks to Antal1987 for figuring out most of the structs years before I came along).

C3X is open source. The C code that gets injected into the game's EXE is located in injected_code.c and the code to perform the injection is located in ep.c. You're invited to explore the source code if you're interested.

MORE INFO, QUESTIONS, COMMENTS:
See my thread on CivFanatics: https://forums.civfanatics.com/threads/c3x-exe-mod-including-bug-fixes-stack-bombard-and-much-more.666881/
The mod is also on GitHub: https://github.com/maxpetul/C3X

SPECIAL THANKS:
1. Antal1987 for his work reverse engineering Civ3. See: https://github.com/Antal1987/C3CPatchFramework
2. R�mulo Prado for his help testing the mod
3. Civinator for the German translation. See: https://www.civforum.de/showthread.php?113285-Der-Flintlock-Deutsch-Patch
4. Vaughn Parker for generously commissioning the port to the PCGames.de EXE and various other features
