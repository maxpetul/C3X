C3X: Executable Mod for Civ 3 Complete
Release 10

INCLUDES (** = new in latest version):
** Mod config can be set separately for each scenario (see below for details)
** Mod info button on preferences screen
Convenience features:
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
  - ** Improved logic: now separates captured and native units, units carrying a princess, and more
 - Show coordinates and chopped status in tile info box
 - Show golden age turns remaining
 - No special king unit names in non-regicide games
 - Option to disable worker automation
AI enhancements:
 - Allow AI to use artillery in the field
 - Force AI to build more artillery and bombers
 - Replace leader unit AI to fix bugs and improve behavior
 - Fix bug preventing AI from filling its armies
 - Improve AI army composition to discourage mixing types & exclude HN units
 - AI routine for "pop units" that may appear in mods
Bugs fixed:
 - AI pathfinding collides with invisible units (called the "submarine bug")
 - Science age beakers not actually awarded
 - Pink line in Civilopedia
 - Crash when doing disembark-all on transport containing immobile unit(s)
 - Crash possible when AI civ is left alive with only a settler on a transport (called the "houseboat bug")
 - ** Resources beyond the first 32 share access records in cities not on the main trade network (called the "phantom resource bug")
 - ** Air units lose a turn after being set to intercept
Engine extensions:
 - Adjustable minimum city distance
 - Option to limit railroad movement
 - Removed unit limit
 - Enable free improvements from small wonders
 - Option to prevent autoraze and razing by AIs
 - Stealth attack activates even when there's only one target
 - Trespassing prevention (experimental)
 - Land/sea intersections
 - Adjustable anarchy length
 - "Perfume" units or improvements to control how likely the AI is to build them
 - Reveal AI logic
  - Press P in city screen to see AI point value for each available build
  - Press L on map to see how desirable the AI finds each tile as a city location
 - Corruption can be completely removed with "OFF" government setting
 - Disallow land settlers from founding on water
 - ** Option to let units move after airdropping

INSTALLATION AND USAGE:
Extract the mod to its own folder then copy that folder into your Civ install directory (i.e. the folder containing Civ3Conquests.exe). Then activate the mod by double-clicking the INSTALL.bat or RUN.bat scripts. INSTALL.bat will install the mod into Civ3Conquests.exe, RUN.bat will launch Civ 3 then apply the mod to the program in memory. The mod's behavior is highly adjustable by editing the config file named "default.c3x_config.ini". Also that config file contains info about some mod features that aren't fully explained in this README.

Notes about installation:
1. When installing, the mod will create a backup of the original unmodded executable named "Civ3Conquests-Unmodded.exe".
2. To uninstall the mod, delete the modded executable then rename the backed up version mentioned above to "Civ3Conquests.exe".
3. It is not necessary to uninstall the mod before installing a different version.
4. Even after installation, the mod still depends on some files in the mod folder, specifically the config file and the Art & Text folders.
5. I've received multiple reports that RUN.bat doesn't work while installing does, so know that installation is the more reliable option.
6. Rômulo Prado reports that RUN.bat started working for him after he installed the MS Visual C++ Redistributables versions 2005 and 2019 (while installing GOG Galaxy).

COMPATIBILITY:
The mod is only compatible with the GOG and Steam versions of Civ 3 Complete. It works with existing save files and saves made with the mod active will still work in the base game. Multiplayer is not officially supported but some features will work in MP, see this post: https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/page-16#post-16126470.

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

Disallowing trespassing prevents civs from entering each other's borders while at peace without a right of passage, similar to the rules in Civ 4. I consider this to be an experimental feature in this release. I have done some testing to verify that the rule is applied correctly, including on the AI, but I haven't checked that the AI can handle the rule properly, for example it might prevent the AI from declaring war.

** NEW ** PER-SCENARIO CONFIG:
Mod config settings can now be set on a per-scenario basis. To do this, create a "scenario.c3x_config.ini" file inside the scenario's folder (the same place that would contain the scenario's Art and Text folders) and fill it in like the "default.c3x_config.ini" file included with the mod. The scenario settings will be layered on top of the default settings so you only need to include the ones that are relevant to the scenario. You can verify in-game that the scenario config was loaded by checking the mod info popup, it's accessible through the button on the top right of the preferences screen. For an example, see: https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/page-28#post-16212316.

SMALL WONDER FREE IMPROVEMENTS:
The free improvements wonder effect (granaries from Pyramids etc.) now works on small wonders. Note to modders, to set this effect you must use a third party editor like Quintillus' (https://forums.civfanatics.com/threads/cross-platform-editor-for-conquests-now-available.377188/) because the option is grayed out in the standard editor. Even worse, if you set the effect then work on the BIQ in the standard editor, the effect won't be saved, so you'd need to set it every time or work exclusively in Quintillus' editor.

ADJUSTABLE MIN CITY DISTANCE:
Change the adjust_minimum_city_separation config value to add that many tiles to the minimum allowed distance for founding cities. For example, setting it to 2 means cities can only be founded with 3 tiles separating them. Setting it to a negative number means cities are allowed to be founded next to one another. Additionally, by community request, the option disallow_founding_next_to_foreign_city can be used to disallow founding cities next to those of other civs even when the min separation is zero or less (if the min separation has not been reduced this option has no effect).

NO-RAZE:
NoRaze has been re-implemented inside C3X but is not enabled by default. To enable it, edit the config file mentioned above.

HOW IT WORKS:
Some parts of the mod (bug fixes, no-raze, no unit limit) are really just hex edits that are applied to the Civ program code. The real secret sauce is a system to compile and inject arbitrary C code into the process which makes it practical to implement new features in the game. The heart of the system is TCC (Tiny C Compiler) and much work puzzling out the functions and structs inside the executable (and thanks to Antal1987 for figuring out most of the structs years before I came along).

The injected code, along with the rest of the mod, is fully open source. If you're curious how stack bombard was implemented, check out "patch_Main_Screen_Form_perform_action_on_tile" in "injected_code.c", I assure you the code is quite readable.

MORE INFO, QUESTIONS, COMMENTS:
See my thread on CivFanatics: https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/

SPECIAL THANKS:
1. Antal1987 for his work reverse engineering Civ3. See: https://github.com/Antal1987/C3CPatchFramework
2. Rômulo Prado for his help testing the mod
3. Civinator for the German translation. See: https://www.civforum.de/showthread.php?113285-Der-Flintlock-Deutsch-Patch
