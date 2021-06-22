C3X: Executable Mod for Civ 3 Complete
Release 7

INCLUDES (* = new in latest version):
* Buttons on trade screen to quickly switch between civs
* Ask/offer gold popup autofills best amount
* Adjustable minimum city distance
Stack bombard
Disorder warning
Stack worker buttons
AI enhancements:
  - Allow AI to use artillery in the field
  - Force AI to build more artillery and bombers
  - Replace leader unit AI to fix bugs and improve behavior
  - Fix bug preventing AI from filling its armies
  - Improve AI army composition to discourage mixing types & exclude HN units
Detailed city production info
Option to limit railroad movement
Removed unit limit
Enable free improvements from small wonders
Bugs fixed:
  - AI pathfinding collides with invisible units (called the "submarine bug")
  - Science age beakers not actually awarded
  - Pink line in Civilopedia
  - Fix for immobile unit disembark crash
  - Fix for for houseboat bug
Option to prevent autoraze and razing by AIs
Stealth attack activates even when there's only one target
Skip repeated popups asking to replace a tile improvement

INSTALLATION AND USAGE:
Begin by extracting the mod and copying its folder into the Civ install directory (that is, the folder containing Civ3Conquests.exe). Then activate the mod by double-clicking the INSTALL.bat or RUN.bat scripts. INSTALL.bat will install the mod into Civ3Conquests.exe, RUN.bat will launch Civ 3 then apply the mod to the program in memory. The mod's behavior is highly adjustible by editing a text configuration file. It is named "default.c3x_config.ini".

Notes about installation:
1. When installing, the original executable will be copied over to "Civ3Conquests-Unmodded.exe".
2. To uninstall the mod, delete the modded executable then rename the backed up version mentioned above to "Civ3Conquests.exe".
3. It is not necessary to uninstall the mod before installing a different version; the installer knows to check for the backed up EXE.
4. Even after installation, the mod still depends on some files in the mod folder, specifically the config file and the Art & Text folders.
5. I've received multiple reports that RUN.bat doesn't work while installing does, so know that installation is the more reliable option.
6. Rômulo Prado reports that RUN.bat started working for him after he installed the MS Visual C++ Redistributables versions 2005 and 2019 (while installing GOG Galaxy).

COMPATIBILITY:
The mod is only officially compatible with the GOG and Steam versions of Civ 3 Complete. It may be compatible with other versions if they use the same executable as one of those two. Multiplayer is untested. The mod works with existing save files and saves made with the mod active will still work in the base game.

STACK BOMBARD:
Activate stack bombard on any unit capable of bombarding by clicking the stack bombard button or by activating normal bombard then CTRL+clicking the target tile. The selected unit will bombard the tile, then all other units of the same type on the same tile will automatically bombard the target as well. Stack bombard is pretty smart and will stop bombarding once it can no longer do any damage, and it knows about lethal bombard, that you can't damage air units in an airfield, etc.

DISORDER WARNING:
If you try to end the turn with unhappy cities, the domestic advisor will pop up to warn you and give you the option to continue that turn. One minor annoyance is that the game does not recompute city happiness when you sign a deal to import a luxury, so doing so won't remove the warnings. To make the game recompute city happiness, simply bump the luxury slider back and forth. I hope to fix this annoyance for the next version (this time for sure).

STACK WORKER BUTTONS:
Hold the control key to turn all standard worker buttons into stack buttons. Clicking a stack button will issue the command to all workers on the same tile.

AI ENHANCEMENTS:
Numerous changes have been made to improve the AI's behavior, especially in combat. It can now use its artillery units in the field, i.e., it will take them out of its cities to bombard enemy cities or incoming enemy units. The AI's production of artillery has been significantly increased so that it can take advantage of this ability. The other major change is that the AI can now use armies properly, it builds them when it can and fills them with units, usually the strongest available. There are many smaller changes as well to fix bugs and improve heuristics. Some more details are available as comments in the config file.

TRADE SCREEN IMPROVEMENTS:
When negotiating with the AI, you can quickly switch back and forth between civs using the added arrow buttons (arrow keys work as well). When asking for or offering gold, the set amount popup will appear with the best amount already filled in. Best amount means, when asking for gold on an acceptable trade, the most you could get, and when offering gold on an unacceptable trade, the least you need to pay.

LIMITED RAILROAD MOVEMENT:
The mod adds the option to limit railroad movement to a certain number of tiles. To enable this, edit the config file. The limitation works like in Civ 4, i.e., moving along a railroad consumes movement points like moving along a road except the cost of moving along a railroad is scaled by the unit's total moves so all units are limited to the same distance. Be advised, because this setting affects how movement is calculated, changing it in the middle of a turn (i.e. after some units have already moved) is likely to cause units to have extra or missing moves.

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
3. Civinator for the German translation (still in progress)
