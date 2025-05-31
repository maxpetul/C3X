
=== Usage ===

Simply double-click RUN.bat to start the editor. It is not necessary to install C3X to use the editor.

=== Editor Columns ===

* Time (sec.): Starting play time for each WAV sound. The table is sorted by this column.
* Duration (sec.): Time between the note on and note off events for this row's sound track. Setting the duration shorter than the WAV file's length does not stop its sound early. If a duration is set too long, it can extend the length of the overall AMB sound. You can match the duration to the WAV file's length using Edit -> Match duration to WAV.
* WAV File: The name of the WAV file including the .wav extension. If the entered name does not match any WAV file in the AMB's folder, the name will be shown in red.
* Speed Rnd: Whether or not to randomize playback speed
* Speed Min/Max: Upper and lower bounds for playback speed randomization. 100 points corresponds to about 6%.
* Vol. Rnd: Whether or not to randomize volume
* Vol. Min/Max: Upper and lower bounds for volume randomization. May be negative.

=== Editor Controls ===

* Play Preview: Plays the current AMB sound. This requires sound.dll from a Civ 3 installation. If the editor can't find your Civ 3 install location automatically, it will prompt you to select the location manually. Previews can only be played if all needed WAV files are present in the same folder as the AMB file itself.
* Stop: Previews play in a loop until this button is clicked.
* File -> View AMB Details: Opens a window with full details about the current AMB data, including all chunks and MIDI data
* Edit -> Add Row: Adds a new WAV sound to the AMB data
* Edit -> Delete Row: Deletes the currently selected WAV sound from the AMB data
* Edit -> Match duration to WAV: Sets the duration of the currently selected row to match the length of the row's WAV file
* Edit -> Prune: Deletes any MIDI sound tracks, Prgm chunks, or Kmap chunks that cannot be heard either because they are invalid or because they are not referenced by other parts of the AMB. Such superfluous elements appear in a small number of AMB files included with Civ 3 including GalleyAttack.amb, HopliteAttackA.amb, WarChariotAttack.amb, and AntiTankAttack.amb. Outside of such broken files, this option is not expected to do anything.

=== AMB Format Overview ===

AMB files are special sound files created by Firaxis for Civilization III. As far as I know, they have never been used anywhere else. AMB files specify sounds as combinations of WAV files with randomized playback speeds and volumes. More details about AMB internals are below, however it is not necessary to know this information in order to use the editor.

The important pieces of an AMB file are Prgm chunks, Kmap chunks, and MIDI data:
* The MIDI data specifies when each component sound is to be played. Specifically, it contains a number of sound tracks, each of which has a program number and note on event. The program number corresponds to a Prgm chunk and the time of the note on event determines when it's played.
* The Prgm chunks determine the random effects applied to each component sound and which sounds are played using variable names. Each Prgm chunk has switches for randomizing playback speed and volume, as well as two fields for each, specifying the maximum and minimum bounds for randomization. Each Prgm chunk also has a variable name which matches a Kmap chunk.
* The Kmap chunks name the WAV files. Each one contains a variable name (matching a name from a Prgm chunk) and a WAV file name.

Summing it up, AMB files contain a number of MIDI tracks specifying when sounds are to be played. Each track refers to a Prgm chunk which contains randomized effects, and each Prgm chunk refers to a Kmap chunk which determines the name of the WAV file to be played.

The AMB editor combines one MIDI track + Prgm chunk + Kmap chunk into each row of the table. If you want to see the internals of a particular AMB file, including all its data chunks and tracks, open it in the editor and choose File -> View AMB Details. For a full breakdown of the AMB format, see this GitHub repo: http://github.com/maxpetul/Civ3AMBAnalysis.

=== Links ===

C3X main page: https://forums.civfanatics.com/resources/c3x.28759/
C3X GitHub: https://github.com/maxpetul/C3X
