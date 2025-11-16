C3X: Executable Mod for Civ 3 Complete
Release 26 Preview 1

=== INSTALLATION ===
Extract the mod, keeping it in its own folder, then copy that folder to your main Conquests directory (i.e. the folder containing Civ3Conquests.exe). Then activate the mod by double-clicking the INSTALL.bat script. You should get a message reporting that the installation was successful. You can also try RUN.bat, which launches a modded version of Civ 3 without installation, however I have received several reports that that script doesn't work for some people.

Notes about installation:
1. If your Civ 3 is installed inside Program Files then it may be necessary to run INSTALL.bat as administrator due to Windows restrictions on editing the contents of Program Files.
2. When installing, the mod will create a backup of the original unmodded executable named "Civ3Conquests-Unmodded.exe".
3. To uninstall the mod, delete the modded executable then rename the backed up version mentioned above to "Civ3Conquests.exe".
4. It is not necessary to uninstall the mod before installing a different version.
5. Even after installation, the mod still depends on some files in the mod folder, so you need to keep it around.
6. Rômulo Prado reports that RUN.bat started working for him after he installed the MS Visual C++ Redistributables versions 2005 and 2019 (while installing GOG Galaxy).

=== CONFIGURATION ===
All aspects of C3X are configurable through INI text files. The basic INI file is called "default.c3x_config.ini" and is located in the mod folder. For example, if you want to turn off grouping of units on the right-click menu (that's what gives you "3x Spearman" instead of 3 identical Spearman entries), you could open that file in any text editor, find the line `group_units_on_right_click_menu = true`, and set it to `false`.

However, because the default config file gets updated with each new release of the mod, it's recommended to put changes like the one above in a new file named "custom.c3x_config.ini". C3X supports up to three different config files, the default config, a scenario config named "scenario.c3x_config.ini" located in a scenario's search folder, and a custom config. They are loaded in that order. Scenario configs are intended to contain rule settings relevant to a particular scenario and custom configs are intended to function like user preferences. For a quick example of a scenario config, see this post: https://forums.civfanatics.com/threads/c3x-exe-mod-including-bug-fixes-stack-bombard-and-much-more.666881/page-28#post-16212316.

=== MORE INFO ===
The mod's main page on CivFanatics is here: https://forums.civfanatics.com/resources/c3x.28759/
The mod is hosted on GitHub here: https://github.com/maxpetul/C3X
For a list of all mod features and descriptions of most of them, see default.c3x_config.ini.
