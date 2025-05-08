
// This file gets #included into amb_editor.c

// Forward declarations for functions defined in amb_editor.c
extern bool PathIsDirectory(const char* path);
extern bool PathFileExists(const char* path);

// Using fastcall to substitute for lack of thiscall on a C compiler, as usual. This is the dummy second param.
#define __ 0

typedef struct WaveDevice WaveDevice;

typedef struct {
    void * omitted[4];
    int (__fastcall * Initialize) (WaveDevice *, int, HWND, unsigned);
    void * omitted_2[38];
} WaveDeviceVTable;

struct WaveDevice {
    WaveDeviceVTable * vtable;
    // other fields omitted
};

typedef struct MidiDevice MidiDevice;

typedef struct {
    void * omitted[4];
    int (__fastcall * Initialize)(MidiDevice *, int, HWND, unsigned);
    void * omitted_2[38];
} MidiDeviceVTable;

struct MidiDevice {
    MidiDeviceVTable * vtable;
    // other fields omitted
};

typedef struct SoundCore SoundCore;

typedef struct {
    void * omitted[4];
    int (__fastcall * LoadFile)(SoundCore *, int, char const *); // takes file path
    void * omitted_2[2];
    int (__fastcall * Play)(SoundCore *);
    int (__fastcall * Stop)(SoundCore *);
    void * omitted_3[7];
    void (__fastcall * SetVolume)(SoundCore *, int, int); // volume is >= 0 and <= 127
    void * omitted_4[5];
    int (__fastcall * M22)(SoundCore *);
    void * omitted_5;
    int (__fastcall * M24)(SoundCore *);
    void * omitted_6[2];
    int (__fastcall * SetFlags)(SoundCore *, int, unsigned);
    void * omitted_7[22];
    int (__fastcall * M50)(SoundCore *);
    void * omitted_8[26];
} SoundCoreVTable;

struct SoundCore {
    SoundCoreVTable * vtable;
    // many more fields omitted
};

enum sound_core_type {
    SCT_DETECT_FROM_FILE_EXT = 0, // Does not work
    SCT_WAV,
    SCT_MIDI,
    SCT_AIF,
    SCT_4,
    SCT_AMB,
    SCT_6,
    SCT_7,
    SCT_8
};

HMODULE soundModule = NULL;
int (__cdecl * InitSoundTimer)(int param_1, int param_2) = NULL;
int (__cdecl * CreateSound)(SoundCore ** out_sound_core, char const * file_path, int sound_core_type) = NULL;
int (__cdecl * CreateWaveDevice)(WaveDevice ** out, unsigned param_2) = NULL;
int (__cdecl * CreateMidiDevice)(MidiDevice ** out, unsigned param_2) = NULL;

WaveDevice * waveDevice = NULL;
MidiDevice * midiDevice = NULL;

SoundCore * playingCore = NULL;

void InitializePreviewPlayer(HWND window, char *conquestsInstallPath)
{
    char errorMsg[1000] = {0};

    char savedCWD[1000];
    GetCurrentDirectory(sizeof savedCWD, savedCWD);

    SetCurrentDirectory(conquestsInstallPath);
    soundModule = LoadLibraryA("sound.dll");
    if (soundModule == NULL) {
        snprintf(errorMsg, (sizeof errorMsg) - 1, "GetLastError returns: %d", GetLastError());
        goto done;
    }

    InitSoundTimer   = (void *)GetProcAddress(soundModule, "init_sound_timer");
    CreateSound      = (void *)GetProcAddress(soundModule, "create_sound");
    CreateWaveDevice = (void *)GetProcAddress(soundModule, (LPCSTR)5); // Use ordinal b/c name is mangled
    CreateMidiDevice = (void *)GetProcAddress(soundModule, (LPCSTR)7);

    InitSoundTimer(0, 0);

    CreateWaveDevice(&waveDevice, 0);
    waveDevice->vtable->Initialize(waveDevice, __, window, 2); // Civ 3 passes 2 for second param (flags)

    CreateMidiDevice(&midiDevice, 0);
    midiDevice->vtable->Initialize(midiDevice, __, window, 0);

done:
    if (strlen(errorMsg) > 0)
        MessageBox(NULL, errorMsg, "Couldn't load sound.dll", MB_ICONERROR);
    SetCurrentDirectory(savedCWD);
}

void StopAmbPreview()
{
    if (playingCore != NULL) {
        playingCore->vtable->Stop(playingCore);
        // TODO: Figure out how to deallocate Sound Core objects. Right now we just leak the memory. Attempting to reuse a core causes a crash.
        playingCore = NULL;
    }
}

#define MAX_WAV_FILES 100
#define TEMP_DIR "AMBEditorTemp"
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// Create a temporary directory for AMB previewing
BOOL CreateTempDirectory(char *tempDirPath, size_t tempDirPathSize)
{
    char tempPath[MAX_PATH];
    DWORD result = GetTempPath(MAX_PATH, tempPath);
    if (result == 0 || result > MAX_PATH) {
        return FALSE;
    }
    
    // Generate a unique subdirectory for our editor
    snprintf(tempDirPath, tempDirPathSize, "%s%s", tempPath, TEMP_DIR);
    
    // Create the directory if it doesn't exist
    if (!PathIsDirectory(tempDirPath)) {
        if (!CreateDirectory(tempDirPath, NULL)) {
            return FALSE;
        }
    }
    
    return TRUE;
}

// Copy WAV files referenced by the AMB to the temp directory
BOOL CopyWavFilesToTempDir(AmbFile *ambFile, char *tempDirPath)
{
    char sourcePath[MAX_PATH_LENGTH];
    char targetPath[MAX_PATH_LENGTH];
    BOOL success = TRUE;
    
    // Get the directory of the original AMB file to find the WAV files
    char ambDir[MAX_PATH_LENGTH];
    strncpy(ambDir, ambFile->filePath, MAX_PATH_LENGTH);
    ambDir[MAX_PATH_LENGTH - 1] = '\0';
    
    // Find the last backslash in the path
    char *lastBackslash = strrchr(ambDir, '\\');
    if (lastBackslash != NULL) {
        *lastBackslash = '\0'; // Truncate the path at the last backslash
    }
    
    // For each Kmap chunk, copy any referenced WAV files
    for (int i = 0; i < ambFile->kmapChunkCount; i++) {
        KmapChunk *chunk = &ambFile->kmapChunks[i];
        
        // For each item in the Kmap chunk, copy its WAV file
        for (int j = 0; j < chunk->itemCount && j < MAX_ITEMS; j++) {
            const char *wavFileName = chunk->items[j].wavFileName;
            
            // Skip if no WAV file is specified
            if (strlen(wavFileName) == 0) {
                continue;
            }
            
            // Construct the source and target paths
            snprintf(sourcePath, MAX_PATH_LENGTH, "%s\\%s", ambDir, wavFileName);
            snprintf(targetPath, MAX_PATH_LENGTH, "%s\\%s", tempDirPath, wavFileName);
            
            // Copy the file
            if (!CopyFile(sourcePath, targetPath, FALSE)) {
                success = FALSE;
            }
        }
    }
    
    return success;
}

void PreviewAmbFile(char *filePath)
{
    StopAmbPreview();
    
    // If we have an AMB loaded in memory, save it to a temporary file first
    if (g_ambFile.filePath[0] != '\0') {
        char tempDirPath[MAX_PATH_LENGTH];
        char tempFilePath[MAX_PATH_LENGTH];
        
        // Create a temporary directory for our files
        if (CreateTempDirectory(tempDirPath, MAX_PATH_LENGTH)) {
            // Generate a temp file path for the AMB
            snprintf(tempFilePath, MAX_PATH_LENGTH, "%s\\temp.amb", tempDirPath);
            
            // Save the current AMB to the temp file
            if (SaveAmbFile(tempFilePath)) {
                // Copy all WAV files referenced by the AMB to the temp directory
                if (CopyWavFilesToTempDir(&g_ambFile, tempDirPath)) {
                    // Show debug message with temp file path
                    char debugMsg[MAX_PATH_LENGTH + 100];
                    snprintf(debugMsg, sizeof(debugMsg), "Previewing from temp file:\n%s", tempFilePath);
                    MessageBox(NULL, debugMsg, "Debug Info", MB_OK | MB_ICONINFORMATION);
                    
                    // Use this temp file for previewing
                    filePath = tempFilePath;
                }
            }
        }
    }
    
    // Play the AMB file (either the original or our temp copy)
    CreateSound(&playingCore, NULL, SCT_AMB);
    if (playingCore != NULL) {
        playingCore->vtable->LoadFile(playingCore, __, filePath);
        playingCore->vtable->Play(playingCore);
    }
}
