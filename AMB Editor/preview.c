
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

bool previewPlayerIsInitialized = false;

HMODULE soundModule = NULL;
int (__cdecl * InitSoundTimer)(int param_1, int param_2) = NULL;
int (__cdecl * CreateSound)(SoundCore ** out_sound_core, char const * file_path, int sound_core_type) = NULL;
int (__cdecl * DeleteSound)(SoundCore * sound_core) = NULL;
int (__cdecl * CreateWaveDevice)(WaveDevice ** out, unsigned param_2) = NULL;
int (__cdecl * CreateMidiDevice)(MidiDevice ** out, unsigned param_2) = NULL;

WaveDevice * waveDevice = NULL;
MidiDevice * midiDevice = NULL;

SoundCore * playingCore = NULL;

void InitializePreviewPlayer(HWND window, char *conquestsInstallPath)
{
    if (previewPlayerIsInitialized)
        return;

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
    DeleteSound      = (void *)GetProcAddress(soundModule, "delete_sound");
    CreateWaveDevice = (void *)GetProcAddress(soundModule, (LPCSTR)5); // Use ordinal b/c name is mangled
    CreateMidiDevice = (void *)GetProcAddress(soundModule, (LPCSTR)7);

    InitSoundTimer(0, 0);

    CreateWaveDevice(&waveDevice, 0);
    waveDevice->vtable->Initialize(waveDevice, __, window, 2); // Civ 3 passes 2 for second param (flags)

    CreateMidiDevice(&midiDevice, 0);
    midiDevice->vtable->Initialize(midiDevice, __, window, 0);

done:
    if (strlen(errorMsg) > 0) {
        MessageBox(NULL, errorMsg, "Couldn't load sound.dll", MB_ICONERROR);
        previewPlayerIsInitialized = false;
    } else
        previewPlayerIsInitialized = true;
    SetCurrentDirectory(savedCWD);
}

void StopAmbPreview()
{
    if (playingCore != NULL) {
        playingCore->vtable->Stop(playingCore);
        DeleteSound(playingCore);
        playingCore = NULL;
    }
}

void DeinitializePreviewPlayer()
{
    if (previewPlayerIsInitialized) {
        StopAmbPreview();
        FreeLibrary(soundModule);
        previewPlayerIsInitialized = false;
    }
}

#define TEMP_DIR "AMBEditorTemp"
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// Helper function to recursively delete directory contents
void DeleteDirectoryContents(const char *dirPath)
{
    WIN32_FIND_DATA findData;
    char searchPattern[MAX_PATH] = {0};
    char fullPath[MAX_PATH] = {0};
    
    snprintf(searchPattern, sizeof(searchPattern) - 1, "%s\\*.*", dirPath);
    HANDLE hFind = FindFirstFile(searchPattern, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Skip . and .. directory entries
            if (strcmp(findData.cFileName, ".") == 0 || 
                strcmp(findData.cFileName, "..") == 0) {
                continue;
            }
            
            snprintf(fullPath, sizeof(fullPath) - 1, "%s\\%s", dirPath, findData.cFileName);
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Recursively delete subdirectory contents
                DeleteDirectoryContents(fullPath);
                // Try to remove the empty directory
                RemoveDirectory(fullPath);
            } else {
                // Delete file
                DeleteFile(fullPath);
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
}

// Prepare temporary directory for AMB previewing, creating a numbered subfolder
BOOL PrepareTempDirectory(char *tempDirPath, size_t tempDirPathSize)
{
    char tempPath[MAX_PATH] = {0};
    char baseTempDir[MAX_PATH] = {0};
    
    // Get the system temp directory
    DWORD result = GetTempPath(MAX_PATH, tempPath);
    if (result == 0 || result > MAX_PATH) {
        return FALSE;
    }
    
    // Create our base temp folder in the system temp directory
    snprintf(baseTempDir, sizeof(baseTempDir) - 1, "%s%s", tempPath, TEMP_DIR);
    
    // Create the directory if it doesn't exist
    if (!PathIsDirectory(baseTempDir)) {
        if (!CreateDirectory(baseTempDir, NULL)) {
            return FALSE;
        }
    }
    
    // Delete everything within the base temp directory
    DeleteDirectoryContents(baseTempDir);
    
    // Find an unused numbered subfolder
    char numberedPath[MAX_PATH] = {0};
    int folderNumber = 1;
    
    while (folderNumber <= 9999) { // Reasonable upper limit
        snprintf(numberedPath, sizeof(numberedPath) - 1, "%s\\%d", baseTempDir, folderNumber);
        
        if (!PathIsDirectory(numberedPath)) {
            // This number is available, create the directory
            if (CreateDirectory(numberedPath, NULL)) {
                // Update the output path to the numbered folder
                snprintf(tempDirPath, tempDirPathSize - 1, "%s", numberedPath);
                return TRUE;
            } else {
                return FALSE;
            }
        }
        
        folderNumber++;
    }
    
    // If we couldn't find an unused number, fail
    return FALSE;
}

// Copy WAV files referenced by the AMB to the temp directory
BOOL CopyWavFilesToTempDir(AmbFile const * ambFile, char *tempDirPath)
{
    Path sourcePath = {0};
    Path targetPath = {0};
    BOOL success = TRUE;
    
    // Get the directory of the original AMB file to find the WAV files
    Path ambDir;
    strcpy(ambDir, ambFile->filePath);
    
    // Find the last backslash in the path
    char *lastBackslash = strrchr(ambDir, '\\');
    if (lastBackslash != NULL) {
        *lastBackslash = '\0'; // Truncate the path at the last backslash
    }
    
    // For each Kmap chunk, copy any referenced WAV files
    for (int i = 0; i < ambFile->kmapChunkCount; i++) {
        KmapChunk const * chunk = &ambFile->kmapChunks[i];
        
        // For each item in the Kmap chunk, copy its WAV file
        for (int j = 0; j < chunk->itemCount && j < MAX_ITEMS; j++) {
            const char *wavFileName = chunk->items[j].wavFileName;
            
            // Skip if no WAV file is specified
            if (strlen(wavFileName) == 0) {
                continue;
            }
            
            // Construct the source and target paths
            snprintf(sourcePath, PATH_BUFFER_SIZE - 1, "%s\\%s", ambDir, wavFileName);
            snprintf(targetPath, PATH_BUFFER_SIZE - 1, "%s\\%s", tempDirPath, wavFileName);
            
            // Copy the file
            if (!CopyFile(sourcePath, targetPath, FALSE)) {
                success = FALSE;
            }
        }
    }
    
    return success;
}

void PreviewAmbFile(AmbFile const * amb)
{
    if (! previewPlayerIsInitialized)
        return;

    StopAmbPreview();
    
    // Save the AMB to a temporary file first. Begin by preparing a temp directory
    Path tempFilePath = {0};
    Path tempDirPath = {0};
    bool success = false;
    if (PrepareTempDirectory(tempDirPath, PATH_BUFFER_SIZE)) {
        // Generate a temp file path for the AMB
        snprintf(tempFilePath, PATH_BUFFER_SIZE - 1, "%s\\temp.amb", tempDirPath);

        // Save the current AMB to the temp file
        if (SaveAmbFile(amb, tempFilePath)) {
            // Copy all WAV files referenced by the AMB to the temp directory
            if (CopyWavFilesToTempDir(amb, tempDirPath)) {
                success = true;
            }
        }
    }

    // Play the temp copy if it was created successfully
    if (success) {
        CreateSound(&playingCore, NULL, SCT_AMB);
        if (playingCore != NULL) {
            playingCore->vtable->LoadFile(playingCore, __, tempFilePath);
            playingCore->vtable->Play(playingCore);
        }
    }
}
