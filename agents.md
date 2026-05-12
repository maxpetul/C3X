# AGENTS.md

## Explanation
- This is a project for modding the game Civilization 3 in C, called C3X.
- `civ_prog_objects.csv` shows memory addresses for patched functions. Do not change this file.
- `ref/Civ3Conquests.h` shows Civ3-native enums and classes. Do not change this file.
- `ep.c` is where most of the actual mod code is for changing the game logic. Add your code to this file.
- `C3X.h` contains classes and enums specific to the mod. It is ok to change this if needed, but generally you should not need to do so. The `injected_state` struct is important, and is injected into `ep.c` as `is`.
- `ref/Civ3Conquests_master.exe.c` is the decompiled code to Civilization 3, for education purposes.
- A function you find in `ref/Civ3Conquests_master.exe.c` like `Leader::do_something`, would, if patched, have a corresponding patch name like `patch_Leader_do_something` in `injected_code.c`. If only *called* but not patched in `injected_code.c`, the name would be `Leader_do_something`.

## Implementation
- Behind the scenes, districts are tile improvements but implemented as mines (the source code assumes a hard-coded number of possible improvements, so would be very challening to add one more to)
- The code to intercept calls to draw a mine is in `patch_Map_Renderer_m12_Draw_Tile_Buildings`.
- Don't worry about looking into how `ep.c` works and do not make changes to `civ_prog_objects.csv`. Focus on 
  1. Reading the existing logic in `injected_code.c` and findings ways it can be adapted and updated for a given use case.
  2. If changing existing functions & patches in `injected_code.c` is insufficient, find relevant source code in `Civ3Conquests_master.exe.c` which may be adapted. Functions and virtual methods may be patched, inled, and so on. Only write code for `injected_code.c` (prefacing with `patch_` if appropriate), and also report back what functions were added to be patched.
- For example, if a new feature can be implemented solely by updating an existing patched function (e.g., `patch_Unit_do_something`), do so. Else, if `patch_Unit_do_something` has not been implemented in `injected_code.c`, but you see it in `Civ3Conquests_master.exe.c` as `Unit::do_something` and note that you could add the feature by implementing `patch_Unit_do_something`, go ahead and do so. Then report back that an entry for `Unit_do_something` needs to be added to `civ_prog_objects.csv` (the human will do this afterward).
- The only files you may change are `C3X.h` and `injected_code.c`. If you feel another file should change, mention this but do not make the changes yourself. Particularly for `civ_prog_objects.csv`, simply mention what entries should be added (which the human will do afterward) and go ahead and implement the necessary logic as patches in `injected_code.c`.
- Do not add new #include statements/libraries or #define functions, if at all avoidable.
- The code injection process runs linearly start to finish, so function declaration should go top to bottom, with functions that rely on other functions declared below them.
- If you need a global variable (or at least, visible across functions) those should be created in the injected_state in C3X.h. Please do not declare global variables in injected_code.c.
- Don't try to compile the code, the human will do that.
- Ignore IDE diagnostics.
- Try to avoid creating many functions in `injected_code.c` that do very little, or unnecessary bookkeeping variables in injected_state in `C3X.h`. Keep it as simple and minimal as possible to accomplish the change (even if hacky).
- Don't use static functions
- Try to minimize excessive bookkeeping and keep solutions as lightweight as possible