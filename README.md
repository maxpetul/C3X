
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
6. I've received multiple reports that RUN.bat doesn't work while installing does, so know that installation is the more reliable option.
7. Rômulo Prado reports that RUN.bat started working for him after he installed the MS Visual C++ Redistributables versions 2005 and 2019 (while installing GOG Galaxy).

### Configuration
The mod's behavior is highly adjustable by editing the config file named "default.c3x_config.ini". Also that config file contains info about some mod features that aren't fully explained in this README.



Old:

**C3X** is a mod for Civ 3 Complete that modifies the game's executable to make changes not possible through the scenario editor. The mod's primary home is on [CivFanatics](https://forums.civfanatics.com/resources/c3x.28759/), look there for a full feature list and releases.

See also: [Antal1987's work](https://github.com/Antal1987/C3CPatchFramework), on which this mod is partially based.
