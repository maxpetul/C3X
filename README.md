
**C3X** is a mod that makes many improvements to the Civ 3 Conquests EXE. It adds quality of life features including stack unit commands (shown below) and an end-of-turn warning for cities about to riot. C3X also fixes many long standing bugs, including the infamous submarine bug, deepens the AI's ability to fight by enabling it to use artillery and army units, and reduces turn times by correcting a major inefficiency in the game's trade network calculation. C3X also forms a platform for other mods by enabling various gameplay changes not possible through the editor. Examples include resource generation from buildings, era-specific names for leaders and civs, limited trespassing and railroad movement like in Civ 4, and a broad expansion of the zone of control and defensive bombard abilities.

|![Demo of stack unit commands in C3X. Holding the control key causes all workers on a tile to build a railroad and causes all bombers to bombard a target.](Misc%20Images/new%20stack%20demo/output_gimp_with_msgs_optimized.gif)|
|:--:|
|Stack unit commands. Holding the control key converts most unit action buttons into stack buttons, which issue their commands to all units of the same type on the same tile. Similarly, holding control when selecting a tile to bombard performs a stack bombard. Also note the grouping of units and movement indicators on the right-click menus.|

### Video Overview
See this video by Suede for a demonstration of how to install the mod and of some of its convenience features.
[![C3X: Incredible Quality of Life mod for Civ 3, Video by Suede CivIII](http://img.youtube.com/vi/VxQ5dVABJcQ/0.jpg)](http://www.youtube.com/watch?v=VxQ5dVABJcQ)

### Installation Details
Extract the mod, keeping it in its own folder, then copy that folder to your main Conquests directory (i.e. the folder containing Civ3Conquests.exe). Then activate the mod by double-clicking the INSTALL.bat script. You should get a message reporting that the installation was successful. You can also try RUN.bat, which launches a modded version of Civ 3 without installation, however I have received several reports that that script doesn't work for some people.

Notes about installation:
1. If your Civ 3 is installed inside Program Files then it may be necessary to run INSTALL.bat as administrator due to Windows restrictions on editing the contents of Program Files.
2. When installing, the mod will create a backup of the original unmodded executable named "Civ3Conquests-Unmodded.exe".
3. To uninstall the mod, delete the modded executable then rename the backed up version mentioned above to "Civ3Conquests.exe".
4. It is not necessary to uninstall the mod before installing a different version.
5. Even after installation, the mod still depends on some files in the mod folder, so you need to keep it around.
6. Rômulo Prado reports that RUN.bat started working for him after he installed the MS Visual C++ Redistributables versions 2005 and 2019 (while installing GOG Galaxy).

### Configuration
All aspects of C3X are configurable through INI text files. The basic INI file is called "default.c3x_config.ini" and is located in the mod folder. For example, if you want to turn off grouping of units on the right-click menu (that's what gives you "3x Spearman" instead of 3 identical Spearman entries), you could open that file in any text editor, find the line `group_units_on_right_click_menu = true`, and set it to `false`.

However, because the default config file gets updated with each new release of the mod, it's recommended to put changes like the one above in a new file named "custom.c3x\_config.ini". C3X supports up to three different config files, the default config, a scenario config named "scenario.c3x\_config.ini" located in a scenario's search folder, and a custom config. They are loaded in that order. Scenario configs are intended to contain rule settings relevant to a particular scenario and custom configs are intended to function like user preferences. For a quick example of a scenario config, see [this post](https://forums.civfanatics.com/threads/sub-bug-fix-and-other-adventures-in-exe-modding.666881/page-28#post-16212316).

### Compatibiity
C3X is compatible with the GOG and Steam versions of Civ 3 Complete and also with the DRM-free executable available through PCGames.de. If you have a CD version of the game, you can replace its EXE with the one from PCGames.de then install C3X on top of that. For more info about the PCGames.de executable, see this thread: [Civ 3, Windows Update KB3086255, & SafeDisc](https://forums.civfanatics.com/threads/civ-3-windows-update-kb3086255-safedisc.552308/).

For info about running C3X on Mac, see this thread: [Installing, Playing and Modding C3C on Apple Silicon](https://forums.civfanatics.com/threads/installing-playing-and-modding-c3c-on-apple-silicon.681540/)

C3X is compatible with existing saves and saves created with the mod active will still be loadable by the base game.

Online multiplayer is not officially supported but some features of the mod will work. Others will not, including stack unit commands. I have also received reports that C3X can cause crashes in online MP. In general, online play is something I'd like to support but haven't gotten around to yet.

### Feature Highlights
#### Disorder Warning:
![C3X disorder warning feature. The domestic advisor is popping up to warn about unhappy cities before the end of a turn.](Misc%20Images/disorder_warning.png)
If you try to end the turn with unhappy cities, the domestic advisor will pop up to warn you and give you the option to continue that turn.

#### AI Enhancements:
Numerous changes have been made to improve the AI's behavior, especially in combat. It can now use its artillery units in the field, i.e., it will take them out of its cities to bombard enemy cities or incoming enemy units. The AI's production of artillery has been significantly increased so that it can take advantage of this ability. The other major change is that the AI can now use armies properly, it builds them when it can and fills them with units, usually the strongest available. There are many smaller changes as well to fix bugs and improve heuristics. Some more details are available as comments in the config file.

#### Trade Screen Improvements:
![C3X trade screen arrows. Shows the top of the leader window with arrows on either side.](Misc%20Images/c3x_trade_screen_arrows.jpg)
When negotiating with the AI, you can quickly switch back and forth between civs using the added arrow buttons (arrow keys work as well). When asking for or offering gold, the set amount popup will appear with the best amount already filled in. Best amount means, when asking for gold on an acceptable trade, the most you could get, and when offering gold on an unacceptable trade, the least you need to pay.

#### Optimization:
A major inefficiency in the game's sea trade computation has been fixed. This elimiates one major cause of slow turns in the late game, especially on large maps with many coastal cities and many wars. For details about the problem and how C3X solves it, see [this post](https://forums.civfanatics.com/threads/c3x-exe-mod-including-bug-fixes-stack-bombard-and-much-more.666881/page-83#post-16536108).

#### Adjustable Movement Rules:
The functions governing unit movement have been modified to enable various adjustments to the game's movement rules not possible through the editor. As with all other engine extensions, the rules are not changed from vanilla Conquests unless the config file is edited.

Railroad movement can be limited to a certain number of tiles by setting the "limit\_railroad\_movement" variable. The limitation works like in Civ 4, i.e., moving along a railroad consumes movement points like moving along a road except the cost of moving along a railroad is scaled by the unit's total moves so all units are limited to the same distance. Be advised, because this setting affects how movement is measured, changing it in the middle of a turn (i.e. after some units have already moved) is likely to cause units to have extra or missing moves.

Enabling land/sea intersections allows sea units to travel over the thin isthmus that exists on the diagonal path between two land tiles. More specifically, imagine a diamond of four tiles with land terrain on the north & south tiles and water on the east & west ones. With land/sea intersections enabled, naval units can pass between the east and west tiles.

Disallowing trespassing prevents civs from entering each other's borders while at peace without a right of passage, similar to the rules in Civ 4. Invisible and hidden nationality units are exempted from the restriction.

#### Small Wonder Free Improvements:
The free improvements wonder effect (granaries from Pyramids etc.) now works on small wonders. Note to modders, to set this effect you must use a third party editor like Quintillus' (https://forums.civfanatics.com/threads/cross-platform-editor-for-conquests-now-available.377188/) because the option is grayed out in the standard editor. Even worse, if you set the effect then work on the BIQ in the standard editor, the effect won't be saved, so you'd need to set it every time or work exclusively in Quintillus' editor.

#### No-raze & No Unit Limit:
The no-raze and no-unit-limit features of earlier modded EXEs have been re-implemented as part of C3X. To enable no-raze, edit the config. There are separate options to prevent autorazing (the forced destruction of size 1 cities) and razing by player's choice.





Old:

**C3X** is a mod for Civ 3 Complete that modifies the game's executable to make changes not possible through the scenario editor. The mod's primary home is on [CivFanatics](https://forums.civfanatics.com/resources/c3x.28759/), look there for a full feature list and releases.

See also: [Antal1987's work](https://github.com/Antal1987/C3CPatchFramework), on which this mod is partially based.
