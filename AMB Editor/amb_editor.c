#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Defines the things we need from commctrl.h, commdlg.h, and any other headers that are not available
#include "win32_selections.h"



#define PATH_BUFFER_SIZE 1024
typedef char Path[PATH_BUFFER_SIZE];

Path g_iniCiv3InstallPath;
Path g_iniSoundDLLPath;
Path g_iniTempDirectory;

void PathAppend(Path path, const char* append)
{
    size_t len = strlen(path);

    // No space at end of buffer
    if (len >= PATH_BUFFER_SIZE - 1)
        return;

    // Add backslash if needed
    if (len > 0 && path[len - 1] != '\\') {
        path[len] = '\\';
        path[len + 1] = '\0';
        len++;
    }
    
    // Append the new part
    strncpy(path + len, append, PATH_BUFFER_SIZE - len);
    path[PATH_BUFFER_SIZE - 1] = '\0';
}

bool PathRemoveFileSpec(Path path)
{
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
        return true;
    }
    return false;
}



#include "amb_file.c"
#include "preview.c"
#include "wav_file.c"



// Forward declarations for ListView functions
void AddListViewColumn(HWND hListView, int index, char *title, int width);
int AddListViewItem(HWND hListView, int index, const char *text);
void SetListViewItemText(HWND hListView, int row, int col, const char *text, BOOL allowRepopulation);
void ClearListView(HWND hListView);
void PopulateAmbListView(void);
BOOL ApplyEditToAmbFile(HWND hwnd, int row, int col, const char *newText, char * outFormattedText, int formattedTextBufferSize);
BOOL IsValidInteger(const char *str);
BOOL GetWavFileDuration(const char* filePath, float* outDuration);
BOOL CheckWavFileExists(const char* wavFileName);
BOOL HasMissingWavFiles(char* missingFiles, int bufferSize);
void MatchDurationToWav(HWND hwndListView);
void LoadAmbFileWithDialog(HWND hwnd);
void SaveAmbFileDirectly(HWND hwnd);
void SaveAmbFileAs(HWND hwnd);
void UpdateOverallDuration(void);

// Currently loaded AMB file
AmbFile g_ambFile = {0};

AmbFile g_fileSnapshots[100] = {0};
int g_snapshotCount = 0,
    g_redoCount = 0;

void ClearFileSnapshots()
{
    memset(g_fileSnapshots, 0, sizeof g_fileSnapshots);
    g_snapshotCount = g_redoCount = 0;
}

// Saves a copy of the currently loaded file onto the stack of snapshots so any changes to the file can be reverted. If the stack is full, drops the
// oldest item.
void SnapshotCurrentFile()
{
    g_redoCount = 0;

    int snapshotCapacity = (sizeof g_fileSnapshots) / (sizeof g_fileSnapshots[0]);
    if (g_snapshotCount == snapshotCapacity) {
        memmove(&g_fileSnapshots[0], &g_fileSnapshots[1], (snapshotCapacity - 1) * (sizeof g_fileSnapshots[0]));
        g_snapshotCount--;
    }

    memcpy(&g_fileSnapshots[g_snapshotCount], &g_ambFile, sizeof(AmbFile));
    g_snapshotCount++;
}

void Undo()
{
    static AmbFile tempAmb;

    if (g_snapshotCount > 0) {
        // Swap most recent snapshot with the current file
        memcpy(&tempAmb, &g_ambFile, sizeof(AmbFile));
        memcpy(&g_ambFile, &g_fileSnapshots[g_snapshotCount - 1], sizeof(AmbFile));
        memcpy(&g_fileSnapshots[g_snapshotCount - 1], &tempAmb, sizeof(AmbFile));

        g_snapshotCount--;
        g_redoCount++;

        PopulateAmbListView(); // Refresh the ListView to reflect changes
    } else
        MessageBeep(MB_ICONERROR);
}

void Redo()
{
    static AmbFile tempAmb;

    if (g_redoCount > 0) {
        // Swap oldest redo (stored one above the newest undo on the stack) with the current file
        memcpy(&tempAmb, &g_fileSnapshots[g_snapshotCount], sizeof(AmbFile));
        memcpy(&g_fileSnapshots[g_snapshotCount], &g_ambFile, sizeof(AmbFile));
        memcpy(&g_ambFile, &tempAmb, sizeof(AmbFile));

        g_snapshotCount++;
        g_redoCount--;

        PopulateAmbListView(); // Refresh the ListView to reflect changes
    } else
        MessageBeep(MB_ICONERROR);
}

// Function declarations
BOOL WINAPI GetOpenFileNameA(LPOPENFILENAMEA lpofn);
BOOL WINAPI GetSaveFileNameA(LPOPENFILENAMEA lpofn);

// Menu IDs
#define IDM_FILE_OPEN       1001
#define IDM_FILE_SAVE       1002
#define IDM_FILE_SAVE_AS    1003
#define IDM_FILE_DETAILS    1004
#define IDM_FILE_EXIT       1005
#define IDM_EDIT_UNDO       1006
#define IDM_EDIT_REDO       1007
#define IDM_EDIT_DELETE     1008
#define IDM_EDIT_ADD        1009
#define IDM_EDIT_MATCH_WAV  1010
#define IDM_EDIT_PRUNE      1011
#define IDM_HELP_ABOUT      1012

// Accelerator table for hotkeys
ACCEL g_accelTable[] = {
    {FCONTROL | FVIRTKEY, 'O', IDM_FILE_OPEN},      // Ctrl+O
    {FCONTROL | FVIRTKEY, 'S', IDM_FILE_SAVE},      // Ctrl+S
    {FCONTROL | FVIRTKEY, 'Z', IDM_EDIT_UNDO},      // Ctrl+Z
    {FCONTROL | FVIRTKEY, 'Y', IDM_EDIT_REDO},      // Ctrl+Y
    {FVIRTKEY, VK_DELETE, IDM_EDIT_DELETE},         // Delete
    {FVIRTKEY, VK_INSERT, IDM_EDIT_ADD}             // Insert
};

// Control IDs 
#define ID_PATH_EDIT        102
#define ID_PLAY_BUTTON      103
#define ID_STOP_BUTTON      104
#define ID_AMB_LISTVIEW     105

// ListView Column Indices
typedef enum {
    COL_TIME = 0,
    COL_DURATION,
    COL_WAV_FILE,
    COL_SPEED_RANDOM,
    COL_SPEED_MIN,
    COL_SPEED_MAX,
    COL_VOLUME_RANDOM,
    COL_VOLUME_MIN,
    COL_VOLUME_MAX
} ListViewColumn;

// Track info structure to associate ListView rows with AMB data
typedef struct {
    int trackIndex;       // MIDI track index
    int kmapIndex;        // Index of Kmap chunk
    int prgmIndex;        // Index of Prgm chunk
    int kmapItemIndex;    // Index of item within Kmap
} AmbRowInfo;

// Temporary structure for sorting rows by timestamp
typedef struct {
    AmbRowInfo rowInfo;
    float timestamp;
    float duration;
    char timeStr[32];
    char durationStr[32];
    char wavFileName[256];
    char speedMaxStr[32];
    char speedMinStr[32];
    char volumeMaxStr[32];
    char volumeMinStr[32];
    bool hasSpeedRandom;
    bool hasVolumeRandom;
} RowData;

// Comparison function for sorting rows by timestamp
int CompareRowsByTimestamp(const void *a, const void *b)
{
    const RowData *rowA = (const RowData *)a;
    const RowData *rowB = (const RowData *)b;
    
    if (rowA->timestamp < rowB->timestamp) return -1;
    if (rowA->timestamp > rowB->timestamp) return 1;
    return 0;
}

// Global variables
Path g_civ3MainPath = {0};
HWND g_hwndPathEdit = NULL;
HWND g_hwndMainWindow = NULL;
HWND g_hwndPlayButton = NULL;
HWND g_hwndStopButton = NULL;
HWND g_hwndListView = NULL;
HWND g_hwndDurationLabel = NULL;
HBRUSH g_hBackgroundBrush = NULL;  // Brush for window background color

// Track info for each row in the ListView
AmbRowInfo g_rowInfo[MAX_SOUND_TRACKS];
int g_rowCount = 0;  // Number of rows in the ListView

// Custom path helpers 
bool PathFileExists(const Path path)
{
    DWORD attr = GetFileAttributes(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool PathIsDirectory(const Path path)
{
    DWORD attr = GetFileAttributes(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

// Function to browse for an AMB file to open
bool BrowseForAmbFile(HWND hwnd, Path filePath)
{
    // Initialize the OPENFILENAME structure
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    
    // Set up buffer
    ZeroMemory(filePath, PATH_BUFFER_SIZE);
    
    // Setup the open file dialog
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = PATH_BUFFER_SIZE;
    ofn.lpstrFilter = "AMB Files\0*.amb\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Select an AMB file to open";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    
    // Set initial directory to Civ3 install path if found
    if (strlen(g_civ3MainPath) > 0) {
        ofn.lpstrInitialDir = g_civ3MainPath;
    }
    
    // Show the dialog and get result
    return GetOpenFileNameA(&ofn);
}

// Load an AMB file selected by the user
void LoadAmbFileWithDialog(HWND hwnd)
{
    Path ambFilePath = {0};
    
    if (BrowseForAmbFile(hwnd, ambFilePath)) {
        if (LoadAmbFile(ambFilePath, &g_ambFile) && g_hwndMainWindow != NULL) {
            ClearFileSnapshots();

            // Extract the filename part from the path
            char *fileName = strrchr(ambFilePath, '\\');
            if (fileName) {
                fileName++; // Skip past the backslash
            } else {
                fileName = (char*)ambFilePath; // No backslash found, use the whole path
            }
            
            char windowTitle[100];
            snprintf(windowTitle, sizeof windowTitle, "%s - AMB Editor", fileName);
            windowTitle[(sizeof windowTitle) - 1] = '\0';
            SetWindowText(g_hwndMainWindow, windowTitle);
            
            // Populate the ListView with the loaded AMB file data
            PopulateAmbListView();
        }
    }
}

// Browse for a file to save
BOOL BrowseForSaveFile(HWND hwnd, Path filePath)
{
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    
    // Initialize the filePath with current file if any
    if (g_ambFile.filePath[0] != '\0') {
        strcpy(filePath, g_ambFile.filePath);
    }
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "AMB Files (*.amb)\0*.amb\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = PATH_BUFFER_SIZE;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;
    ofn.lpstrDefExt = "amb";
    ofn.lpstrTitle = "Save AMB File";
    
    return GetSaveFileNameA(&ofn);
}

// Save AMB file directly (without dialog)
void SaveAmbFileDirectly(HWND hwnd)
{
    if (g_ambFile.filePath[0] == '\0') {
        MessageBox(hwnd, "No AMB file loaded. Please load a file first.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Try to save to the current file path
    if (SaveAmbFile(&g_ambFile, g_ambFile.filePath)) {
        // Success - file saved silently
        return;
    }
    
    // Failed to save (likely permissions issue) - fall back to Save As silently
    SaveAmbFileAs(hwnd);
}

// Save AMB file with Save As dialog
void SaveAmbFileAs(HWND hwnd)
{
    if (g_ambFile.filePath[0] == '\0') {
        MessageBox(hwnd, "No AMB file loaded. Please load a file first.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    Path filePath = {0};
    
    if (BrowseForSaveFile(hwnd, filePath)) {
        // Try to save the AMB file
        if (SaveAmbFile(&g_ambFile, filePath)) {
            // Update the current file path to the new location
            strcpy(g_ambFile.filePath, filePath);
            
            // Extract the filename part from the path and update window title
            char *fileName = strrchr(filePath, '\\');
            if (fileName) {
                fileName++; // Skip past the backslash
            } else {
                fileName = (char*)filePath; // No backslash found, use the whole path
            }
            
            char windowTitle[100];
            snprintf(windowTitle, sizeof windowTitle, "%s - AMB Editor", fileName);
            windowTitle[(sizeof windowTitle) - 1] = '\0';
            SetWindowText(g_hwndMainWindow, windowTitle);
        } else {
            MessageBox(hwnd, "Failed to save AMB file. Check file permissions and try again.", "Error", MB_OK | MB_ICONERROR);
        }
    }
}

// Function to check if a directory has the required Civ3 files
bool IsCiv3ConquestsFolder(const Path folderPath)
{
    Path testPath;
    
    // Check for Civ3Conquests.exe
    strcpy(testPath, folderPath);
    PathAppend(testPath, "Civ3Conquests.exe");
    if (!PathFileExists(testPath)) {
        return false;
    }
    
    // Check for Art folder
    strcpy(testPath, folderPath);
    PathAppend(testPath, "Art");
    if (!PathIsDirectory(testPath)) {
        return false;
    }
    
    return true;
}

// Function to check if a directory is the main Civ3 folder
bool IsCiv3MainFolder(const Path folderPath)
{
    Path testPath;
    
    // Check for Art folder
    strcpy(testPath, folderPath);
    PathAppend(testPath, "Art");
    if (!PathIsDirectory(testPath)) {
        return false;
    }
    
    // Check for civ3PTW folder
    strcpy(testPath, folderPath);
    PathAppend(testPath, "civ3PTW");
    if (!PathIsDirectory(testPath)) {
        return false;
    }
    
    return true;
}

// Function to search parent folders for Civ3 install
bool FindCiv3InstallBySearch(const Path startPath)
{
    Path testPath;
    
    // Start with working directory
    strcpy(testPath, startPath);
    
    // Go up up to 10 parent directories
    for (int i = 0; i < 10; i++) {
        // Check current directory and subdirectories for main Civ3 folder
        WIN32_FIND_DATA findData;
        HANDLE hFind;
        Path searchPath;
        
        strcpy(searchPath, testPath);
        PathAppend(searchPath, "*");
        
        hFind = FindFirstFile(searchPath, &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
                    strcmp(findData.cFileName, ".") != 0 && 
                    strcmp(findData.cFileName, "..") != 0) {
                    
                    Path subDirPath;
                    strcpy(subDirPath, testPath);
                    PathAppend(subDirPath, findData.cFileName);
                    
                    if (IsCiv3MainFolder(subDirPath)) {
                        // Found main Civ3 directory
                        strcpy(g_civ3MainPath, subDirPath);
                        FindClose(hFind);
                        return true;
                    }
                }
            } while (FindNextFile(hFind, &findData));
            FindClose(hFind);
        }
        
        // Move up a directory
        if (!PathRemoveFileSpec(testPath)) {
            break; // Cannot go up further
        }
    }
    
    return false;
}

// Function to find Civ3 install from registry
bool FindCiv3InstallFromRegistry()
{
    HKEY hKey;
    char regValue[PATH_BUFFER_SIZE];
    DWORD valueSize = sizeof(regValue);
    DWORD valueType;
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Infogrames Interactive\\Civilization III", 
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        if (RegQueryValueEx(hKey, "Install_Path", NULL, &valueType, 
                           (LPBYTE)regValue, &valueSize) == ERROR_SUCCESS) {
            
            if (valueType == REG_SZ) {
                regValue[(sizeof regValue) - 1] = '\0'; // RegQueryValueEx is not guaranteed to return null terminated strings
                strcpy(g_civ3MainPath, regValue);
                
                RegCloseKey(hKey);
                return true;
            }
        }
        RegCloseKey(hKey);
    }
    
    return false;
}

bool GetSoundDLLPath(HWND hwnd, Path outSoundDLLPath)
{
    // First, check if INI has a sound.dll path specified
    if (strlen(g_iniSoundDLLPath) > 0) {
        strcpy(outSoundDLLPath, g_iniSoundDLLPath);
        return true;
    }
    
    // Otherwise, check if Conquests\sound.dll exists in the main Civ3 install
    if (strlen(g_civ3MainPath) > 0) {
        Path conquestsSoundPath;
        strcpy(conquestsSoundPath, g_civ3MainPath);
        PathAppend(conquestsSoundPath, "Conquests");
        PathAppend(conquestsSoundPath, "sound.dll");
        
        if (PathFileExists(conquestsSoundPath)) {
            strcpy(outSoundDLLPath, conquestsSoundPath);
            return true;
        }
    }
    
    // If neither works, ask the user to locate sound.dll
    int result = MessageBox(hwnd,
                           "Playing a preview requires sound.dll from your Civ 3 Conquests install. This could not be found automatically. "
                           "Would you like to locate sound.dll manually?",
                           "DLL not found", MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES) {
        Path path = {0};

        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = path;
        ofn.nMaxFile = PATH_BUFFER_SIZE;
        ofn.lpstrFilter = "DLL Files\0*.dll\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrTitle = "Select sound.dll from your Civ 3 Conquests install";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

        if (GetOpenFileNameA(&ofn)) {
            if (PathFileExists(path)) {
                strcpy(outSoundDLLPath, path);
                return true;
            }
        }
    }
    
    return false;
}

// Find Civ3 installation using all available methods
bool FindCiv3Installation(HWND hwnd)
{
    Path cwdPath;
    GetCurrentDirectory(PATH_BUFFER_SIZE, cwdPath);
    
    // Search parent folders first. If that doesn't work, check the registry.
    if (FindCiv3InstallBySearch(cwdPath)) {
        return true;
    } else
        return FindCiv3InstallFromRegistry();
}

// Create the play and stop buttons
void CreatePlaybackButtons(HWND hwnd)
{
    // Create Play button
    g_hwndPlayButton = CreateWindow(
        "BUTTON", 
        "Play Preview",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 20, 100, 30,
        hwnd, (HMENU)ID_PLAY_BUTTON, GetModuleHandle(NULL), NULL
    );
    
    // Create Stop button
    g_hwndStopButton = CreateWindow(
        "BUTTON", 
        "Stop", 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        130, 20, 80, 30,
        hwnd, (HMENU)ID_STOP_BUTTON, GetModuleHandle(NULL), NULL
    );
}

// Add a column to the ListView
void AddListViewColumn(HWND hListView, int index, char *title, int width) 
{
    LVCOLUMNA lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.pszText = title;
    lvc.cx = width;
    
    SendMessage(hListView, LVM_INSERTCOLUMN, index, (LPARAM)&lvc);
}

// Add a row to the ListView
int AddListViewItem(HWND hListView, int index, const char *text) 
{
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = index;
    lvi.iSubItem = 0;
    lvi.pszText = (LPSTR)text;
    
    return SendMessage(hListView, LVM_INSERTITEM, 0, (LPARAM)&lvi);
}

// Set text in a ListView cell
void SetListViewItemText(HWND hListView, int row, int col, const char *text, BOOL allowRepopulation) 
{
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row;
    lvi.iSubItem = col;
    lvi.pszText = (LPSTR)text;
    
    SendMessage(hListView, LVM_SETITEM, 0, (LPARAM)&lvi);
    
    // Check if we need to repopulate when time changes
    if (allowRepopulation && col == COL_TIME) {
        PopulateAmbListView();
    }
}

// Clear all items from the ListView
void ClearListView(HWND hListView) 
{
    SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);
}

// Check if a WAV file exists in the AMB file's directory
BOOL CheckWavFileExists(const char* wavFileName)
{
    if (!wavFileName || strlen(wavFileName) == 0 || g_ambFile.filePath[0] == '\0') {
        return FALSE;
    }
    
    // Construct full path to WAV file
    Path wavFilePath;
    strcpy(wavFilePath, g_ambFile.filePath);
    PathRemoveFileSpec(wavFilePath);  // Remove filename, keep directory
    PathAppend(wavFilePath, wavFileName);
    
    // Check if file exists
    return PathFileExists(wavFilePath);
}

// Check if any WAV files are missing and return a list of missing files
BOOL HasMissingWavFiles(char* missingFiles, int bufferSize)
{
    if (!missingFiles || bufferSize == 0 || g_ambFile.filePath[0] == '\0') {
        return FALSE;
    }
    
    missingFiles[0] = '\0';  // Initialize as empty string
    BOOL hasMissing = FALSE;
    
    // Check each row in the ListView for missing WAV files
    for (int i = 0; i < g_rowCount; i++) {
        AmbRowInfo *rowInfo = &g_rowInfo[i];
        if (rowInfo->kmapIndex >= 0 && rowInfo->kmapIndex < g_ambFile.kmapChunkCount &&
            rowInfo->kmapItemIndex >= 0 && rowInfo->kmapItemIndex < g_ambFile.kmapChunks[rowInfo->kmapIndex].itemCount) {
            
            KmapChunk *kmap = &g_ambFile.kmapChunks[rowInfo->kmapIndex];
            const char *wavFileName = kmap->items[rowInfo->kmapItemIndex].wavFileName;
            
            // Ignore empty WAV file names since those don't interfere with playback
            if ((wavFileName[0] != '\0') && !CheckWavFileExists(wavFileName)) {
                hasMissing = TRUE;
                
                // Add to missing files list if there's space
                if (strlen(missingFiles) > 0) {
                    strncat(missingFiles, "\n", bufferSize - strlen(missingFiles) - 1);
                }
                strncat(missingFiles, wavFileName, bufferSize - strlen(missingFiles) - 1);
            }
        }
    }
    
    return hasMissing;
}

// Function to check if a string is a valid integer
BOOL IsValidInteger(const char *str)
{
    // Allow for a leading + or - sign
    if (*str == '+' || *str == '-') {
        str++;
    }
    
    // Empty string or just a sign isn't a valid integer
    if (*str == '\0') {
        return FALSE;
    }
    
    // Check that all characters are digits
    while (*str) {
        if (!isdigit(*str)) {
            return FALSE;
        }
        str++;
    }
    
    return TRUE;
}




// Defines void ShowAmbDetailsWindow(HWND hwndParent)
#include "details_window.c"



// Returns the index of the Prgm chunk referred to by the given sound track. If the track does not validly refer to any Prgm, returns -1. In order for
// the track to be valid, the channel numbers of all events must match and specify a valid Prgm index and the program change event must specify the
// corresponding program number.
int GetReferencedPrgmIfValid(int trackIndex)
{
    SoundTrack * track = &g_ambFile.midi.soundTracks[trackIndex];

    int prgmIndex = track->programChange.programNumber - 1;

    if ((prgmIndex < 0) || (prgmIndex >= g_ambFile.prgmChunkCount))
        return -1;

    for (int i = 0; i < track->controlChangeCount; i++)
        if (track->controlChanges[i].channelNumber != prgmIndex)
            return -1;

    if ((track->programChange.channelNumber != prgmIndex) ||
        (track->noteOn       .channelNumber != prgmIndex) ||
        (track->noteOff      .channelNumber != prgmIndex))
        return -1;
    
    return prgmIndex;
}

// Populate the ListView with AMB file data
void PopulateAmbListView(void) 
{
    if (g_hwndListView == NULL) {
        MessageBox(NULL, "ListView control is NULL", "Debug", MB_OK);
        return;
    }
    
    // Clear any existing data
    ClearListView(g_hwndListView);
    
    // Reset row info
    g_rowCount = 0;
    memset(g_rowInfo, 0, sizeof(g_rowInfo));
    
    // Check if we have valid AMB data
    if (g_ambFile.kmapChunkCount == 0 || g_ambFile.prgmChunkCount == 0 || g_ambFile.midi.trackCount == 0) {
        MessageBox(NULL, "AMB file has missing data or is invalid", "Debug", MB_OK);
        return;
    }

    // Calculate seconds per tick for timing
    float secondsPerTick = 0.0f;
    if (g_ambFile.midi.ticksPerQuarterNote > 0) {
        secondsPerTick = g_ambFile.midi.secondsPerQuarterNote / g_ambFile.midi.ticksPerQuarterNote;
    }
    
    // Collect all row data before sorting
    RowData rowData[MAX_SOUND_TRACKS];
    int rowDataCount = 0;
    
    // Process each MIDI sound track
    int soundTrackCount = g_ambFile.midi.trackCount - 1; // Minus one to exclude the info track
    for (int trackIndex = 0; trackIndex < soundTrackCount; trackIndex++) {
        SoundTrack * track = &g_ambFile.midi.soundTracks[trackIndex];
        
        int prgmIndex = GetReferencedPrgmIfValid(trackIndex);
        if (prgmIndex < 0)
            continue; // No valid Prgm, skip this track
        PrgmChunk *matchingPrgm = &g_ambFile.prgmChunks[prgmIndex];
        
        // Get the var name from the matching Prgm chunk
        const char *varName = matchingPrgm->varName;
        
        // Find corresponding Kmap chunk with the same var name
        KmapChunk *matchingKmap = NULL;
        int kmapIndex = -1;
        for (int i = 0; i < g_ambFile.kmapChunkCount; i++) {
            if (strcmp(g_ambFile.kmapChunks[i].varName, varName) == 0) {
                matchingKmap = &g_ambFile.kmapChunks[i];
                kmapIndex = i;
                break;
            }
        }
        
        if (matchingKmap == NULL || matchingKmap->itemCount == 0) {
            continue; // Skip if no matching Kmap found or no items in Kmap
        }

        float timestamp = track->deltaTimeNoteOn * secondsPerTick;
        float duration = track->deltaTimeNoteOff * secondsPerTick;

        // Check flags for randomization settings (LSB = speed random, bit 1 = volume random)
        bool hasSpeedRandom  = (matchingPrgm->flags & 0x01) != 0;
        bool hasVolumeRandom = (matchingPrgm->flags & 0x02) != 0;

        // For each item in the kmap (usually just one WAV file per track)
        for (int j = 0; j < matchingKmap->itemCount; j++) {
            if (rowDataCount >= MAX_SOUND_TRACKS) {
                break; // Prevent overflow
            }
            
            // Collect row data
            RowData *row = &rowData[rowDataCount];
            row->rowInfo.trackIndex = trackIndex;
            row->rowInfo.kmapIndex = kmapIndex;
            row->rowInfo.prgmIndex = prgmIndex;
            row->rowInfo.kmapItemIndex = j;
            row->timestamp = timestamp;
            row->duration = duration;
            row->hasSpeedRandom = hasSpeedRandom;
            row->hasVolumeRandom = hasVolumeRandom;
            
            // Format strings
            snprintf(row->timeStr, sizeof(row->timeStr), "%04.3f", timestamp);
            snprintf(row->durationStr, sizeof(row->durationStr), "%04.3f", duration);
            strncpy(row->wavFileName, matchingKmap->items[j].wavFileName, sizeof(row->wavFileName) - 1);
            row->wavFileName[sizeof(row->wavFileName) - 1] = '\0';
            snprintf(row->speedMaxStr, sizeof(row->speedMaxStr), "%d", matchingPrgm->maxRandomSpeed);
            snprintf(row->speedMinStr, sizeof(row->speedMinStr), "%d", matchingPrgm->minRandomSpeed);
            snprintf(row->volumeMaxStr, sizeof(row->volumeMaxStr), "%d", matchingPrgm->maxRandomVolume);
            snprintf(row->volumeMinStr, sizeof(row->volumeMinStr), "%d", matchingPrgm->minRandomVolume);
            
            rowDataCount++;
        }
    }
    
    // Sort rows by timestamp
    qsort(rowData, rowDataCount, sizeof(RowData), CompareRowsByTimestamp);
    
    // Add sorted rows to ListView
    for (int i = 0; i < rowDataCount; i++) {
        RowData *row = &rowData[i];
        
        // Add the item to the ListView
        int listRow = AddListViewItem(g_hwndListView, 0x7FFFFFFF, row->timeStr);
        if (listRow >= 0) {
            // Store the track info for this row
            g_rowInfo[g_rowCount] = row->rowInfo;
            g_rowCount++;
            
            // Set all the column values
            SetListViewItemText(g_hwndListView, listRow, COL_DURATION, row->durationStr, FALSE);
            SetListViewItemText(g_hwndListView, listRow, COL_WAV_FILE, row->wavFileName, FALSE);
            SetListViewItemText(g_hwndListView, listRow, COL_SPEED_RANDOM, row->hasSpeedRandom ? "Yes" : "No", FALSE);
            SetListViewItemText(g_hwndListView, listRow, COL_SPEED_MIN, row->speedMinStr, FALSE);
            SetListViewItemText(g_hwndListView, listRow, COL_SPEED_MAX, row->speedMaxStr, FALSE);
            SetListViewItemText(g_hwndListView, listRow, COL_VOLUME_RANDOM, row->hasVolumeRandom ? "Yes" : "No", FALSE);
            SetListViewItemText(g_hwndListView, listRow, COL_VOLUME_MIN, row->volumeMinStr, FALSE);
            SetListViewItemText(g_hwndListView, listRow, COL_VOLUME_MAX, row->volumeMaxStr, FALSE);
        }
    }
    
    // Update the overall duration display
    UpdateOverallDuration();
}

// Handle the beginning of a ListView label edit
BOOL HandleBeginLabelEdit(HWND hwnd, NMLVDISPINFOA *pInfo)
{
    // You can reject edits for specific columns or rows if needed
    // For now, allow editing all cells
    return TRUE; // Return TRUE to allow the edit, FALSE to prevent it
}

// Apply an edit to the AMB file data structure. Returns TRUE if the edit was accepted or FALSE if it was invalid. If the edit was accepted, the new
// text is written to outFormattedText in a way that is consistent with the usual format of the ListView display (e.g. float fields are formatted
// to 3 decimal places). If the new text exactly matches the current cell text, the edit is accepted but no snapshot is taken and no AMB data is modified.
// outFormattedText must not be NULL and formattedTextBufferSize must be at least 1.
BOOL ApplyEditToAmbFile(HWND hwnd, int row, int col, const char *newText, char * outFormattedText, int formattedTextBufferSize)
{
    // Make sure we have valid row info
    if (row >= g_rowCount) {
        MessageBox(hwnd, "Invalid row index", "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    // Get the track information for this row
    AmbRowInfo *rowInfo = &g_rowInfo[row];
    int trackIndex = rowInfo->trackIndex;
    int kmapIndex = rowInfo->kmapIndex;
    int prgmIndex = rowInfo->prgmIndex;
    int kmapItemIndex = rowInfo->kmapItemIndex;
    
    // Check if indices are valid
    if (trackIndex < 0 || trackIndex >= g_ambFile.midi.trackCount ||
        kmapIndex < 0 || kmapIndex >= g_ambFile.kmapChunkCount ||
        prgmIndex < 0 || prgmIndex >= g_ambFile.prgmChunkCount ||
        kmapItemIndex < 0 || kmapItemIndex >= g_ambFile.kmapChunks[kmapIndex].itemCount) {
        
        MessageBox(hwnd, "Invalid track indices", "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    // Check if new text matches the current value in the cell - if so, accept without changing anything
    char currentText[256] = {0};
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row;
    lvi.iSubItem = col;
    lvi.pszText = currentText;
    lvi.cchTextMax = sizeof(currentText);
    SendMessage(g_hwndListView, LVM_GETITEMTEXT, row, (LPARAM)&lvi);
    
    if (strcmp(newText, currentText) == 0) {
        // Text unchanged - accept edit but don't modify data or take snapshot
        strncpy(outFormattedText, newText, formattedTextBufferSize);
        outFormattedText[formattedTextBufferSize - 1] = '\0';
        return TRUE;
    }
    
    // Get references to the actual AMB data structures
    PrgmChunk *prgm = &g_ambFile.prgmChunks[prgmIndex];
    KmapChunk *kmap = &g_ambFile.kmapChunks[kmapIndex];
    
    // Update the appropriate field based on the column that was edited
    BOOL newTextIsValid = TRUE;
    switch (col) {
        case COL_TIME:
            {
                char * afterParsing;
                float newTime = strtod(newText, &afterParsing);
                if (afterParsing != newText) {
                    SnapshotCurrentFile();

		    float ticksPerSecond = g_ambFile.midi.ticksPerQuarterNote / g_ambFile.midi.secondsPerQuarterNote;
		    g_ambFile.midi.soundTracks[trackIndex].deltaTimeNoteOn = round(newTime * ticksPerSecond);

                    snprintf(outFormattedText, formattedTextBufferSize, "%04.3f", newTime);
                    
                    // Update overall duration since timestamp changed
                    UpdateOverallDuration();

                } else {
                    newTextIsValid = FALSE;
                }
            }
            break;
            
        case COL_DURATION:
            {
                char * afterParsing;
                float newDuration = strtod(newText, &afterParsing);
                if (afterParsing != newText) {
                    SnapshotCurrentFile();

                    float ticksPerSecond = g_ambFile.midi.ticksPerQuarterNote / g_ambFile.midi.secondsPerQuarterNote;
                    g_ambFile.midi.soundTracks[trackIndex].deltaTimeNoteOff = round(newDuration * ticksPerSecond);

                    snprintf(outFormattedText, formattedTextBufferSize, "%04.3f", newDuration);
                    
                    // Update overall duration since duration changed
                    UpdateOverallDuration();

                } else {
                    newTextIsValid = FALSE;
                }
            }
            break;
            
        case COL_WAV_FILE: // WAV filename
            // Reject the edit if the new text contains a slash
            if ((strchr(newText, '\\') == NULL) && (strchr(newText, '/') == NULL)) {
                SnapshotCurrentFile();

                // Write new text to kmap item
                strncpy(kmap->items[kmapItemIndex].wavFileName, newText, sizeof(kmap->items[kmapItemIndex].wavFileName) - 1);
                kmap->items[kmapItemIndex].wavFileName[sizeof(kmap->items[kmapItemIndex].wavFileName) - 1] = '\0';

                // Update Kmap size
                kmap->size = ComputeKmapChunkSize(kmap);

                // Write new text to formatted output
                strncpy(outFormattedText, newText, formattedTextBufferSize);
            } else {
                newTextIsValid = FALSE;
            }
            break;
            
        case COL_SPEED_RANDOM: // Speed Random flag
            {
                if (strcmp(newText, "Yes") == 0 || strcmp(newText, "No") == 0) {
                    SnapshotCurrentFile();

                    if (strcmp(newText, "Yes") == 0) {
                        prgm->flags |= 0x01;  // Set bit 0
                    } else {
                        prgm->flags &= ~0x01;  // Clear bit 0
                    }
                    
                    strncpy(outFormattedText, (prgm->flags & 0x01) ? "Yes" : "No", formattedTextBufferSize);
                } else {
                    newTextIsValid = FALSE;
                }
            }
            break;
            
        case COL_SPEED_MIN: // Speed Min
            if (IsValidInteger(newText)) {
                SnapshotCurrentFile();
                prgm->minRandomSpeed = atoi(newText);
                snprintf(outFormattedText, formattedTextBufferSize, "%d", prgm->minRandomSpeed);
            } else {
                newTextIsValid = FALSE;
            }
            break;
            
        case COL_SPEED_MAX: // Speed Max
            if (IsValidInteger(newText)) {
                SnapshotCurrentFile();
                prgm->maxRandomSpeed = atoi(newText);
                snprintf(outFormattedText, formattedTextBufferSize, "%d", prgm->maxRandomSpeed);
            } else {
                newTextIsValid = FALSE;
            }
            break;
            
        case COL_VOLUME_RANDOM: // Volume Random flag
            {
                if (strcmp(newText, "Yes") == 0 || strcmp(newText, "No") == 0) {
                    SnapshotCurrentFile();

                    if (strcmp(newText, "Yes") == 0) {
                        prgm->flags |= 0x02;  // Set bit 1
                    } else {
                        prgm->flags &= ~0x02;  // Clear bit 1
                    }
                    
                    strncpy(outFormattedText, (prgm->flags & 0x02) ? "Yes" : "No", formattedTextBufferSize);
                } else {
                    newTextIsValid = FALSE;
                }
            }
            break;
            
        case COL_VOLUME_MIN: // Volume Min
            if (IsValidInteger(newText)) {
                SnapshotCurrentFile();
                prgm->minRandomVolume = atoi(newText);
                snprintf(outFormattedText, formattedTextBufferSize, "%d", prgm->minRandomVolume);
            } else {
                newTextIsValid = FALSE;
            }
            break;
            
        case COL_VOLUME_MAX: // Volume Max
            if (IsValidInteger(newText)) {
                SnapshotCurrentFile();
                prgm->maxRandomVolume = atoi(newText);
                snprintf(outFormattedText, formattedTextBufferSize, "%d", prgm->maxRandomVolume);
            } else {
                newTextIsValid = FALSE;
            }
            break;
    }
    
    // Play error beep if new text didn't validate. If it did that means we wrote something to outFormattedText so make sure it's null terminated.
    if (! newTextIsValid)
        MessageBeep(MB_ICONERROR);
    else
        outFormattedText[formattedTextBufferSize - 1] = '\0';

    return newTextIsValid;
}

void DeleteKmapChunk(int kmapIndex)
{
    // Shift all Kmap chunks after this one forward
    for (int i = kmapIndex; i < g_ambFile.kmapChunkCount - 1; i++) {
        memcpy(&g_ambFile.kmapChunks[i], &g_ambFile.kmapChunks[i + 1], sizeof(KmapChunk));
    }
    g_ambFile.kmapChunkCount--;
}

void DeletePrgmChunk(int prgmIndex)
{
    // Shift all Prgm chunks after this one forward
    for (int i = prgmIndex; i < g_ambFile.prgmChunkCount - 1; i++) {
        memcpy(&g_ambFile.prgmChunks[i], &g_ambFile.prgmChunks[i + 1], sizeof(PrgmChunk));
    }
    g_ambFile.prgmChunkCount--;

    // Update the number field of any remaining Prgm chunks - they are 1-based
    for (int i = 0; i < g_ambFile.prgmChunkCount; i++) {
        g_ambFile.prgmChunks[i].number = i + 1;
    }
    
    // Update channelNumber in all MIDI sound tracks for any that reference 
    // Prgm chunks with indices greater than the one we deleted
    for (int i = 0; i < g_ambFile.midi.trackCount - 1; i++) { // Skip info track
        SoundTrack *track = &g_ambFile.midi.soundTracks[i];
        
        // Check all control change events
        for (int j = 0; j < track->controlChangeCount; j++) {
            if (track->controlChanges[j].channelNumber > prgmIndex) {
                track->controlChanges[j].channelNumber--;
            }
        }
        
        // Check program change event
        if (track->programChange.channelNumber > prgmIndex) {
            track->programChange.channelNumber--;
        }
        
        // Update programNumber if it references deleted program or those after it
        // Note: programNumbers are 1-based (matching the Prgm number field)
        if (track->programChange.programNumber == prgmIndex + 1) {
            // This is referencing the deleted program - we'd need to update it
            // to a valid program or potentially disable this track
            // For now, if there are other programs, use the first available one
            if (g_ambFile.prgmChunkCount > 0) {
                track->programChange.programNumber = 1; // Use first available program (1-based)
            }
        }
        else if (track->programChange.programNumber > prgmIndex + 1) {
            // This references a program that got shifted down, so update the index
            track->programChange.programNumber--;
        }
        
        // Check note on event
        if (track->noteOn.channelNumber > prgmIndex) {
            track->noteOn.channelNumber--;
        }
        
        // Check note off event
        if (track->noteOff.channelNumber > prgmIndex) {
            track->noteOff.channelNumber--;
        }
    }
}

void DeleteSoundTrack(int trackIndex)
{
    for (int i = trackIndex; i < g_ambFile.midi.trackCount - 2; i++) { // -2 because we skip info track (0) and want to leave room for the last valid track
        memcpy(&g_ambFile.midi.soundTracks[i], &g_ambFile.midi.soundTracks[i + 1], sizeof(SoundTrack));
    }
    g_ambFile.midi.trackCount--;
}

void DeleteRow(int row)
{
    // If no file loaded or row is invalid, do nothing
    if (g_ambFile.filePath[0] == '\0' || row < 0 || row >= g_rowCount) {
        MessageBox(NULL, "Invalid row or no AMB file loaded", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Get the references to the chunks and tracks we need to work with
    AmbRowInfo *rowInfo = &g_rowInfo[row];
    int trackIndex = rowInfo->trackIndex;
    int kmapIndex = rowInfo->kmapIndex;
    int prgmIndex = rowInfo->prgmIndex;

    // Take a snapshot before making changes
    SnapshotCurrentFile();

    // First, check if the Kmap chunk is referenced by other Prgm chunks
    KmapChunk *kmap = &g_ambFile.kmapChunks[kmapIndex];
    const char *kmapVarName = kmap->varName;
    int kmapReferenceCount = 0;

    for (int i = 0; i < g_ambFile.prgmChunkCount; i++) {
        if (strcmp(g_ambFile.prgmChunks[i].varName, kmapVarName) == 0) {
            kmapReferenceCount++;
        }
    }

    // Delete the Kmap chunk if it's only referenced by the Prgm we're deleting
    if (kmapReferenceCount <= 1) {
        DeleteKmapChunk(kmapIndex);
    }

    // Next, check if the Prgm chunk is referenced by other MIDI tracks
    int prgmReferenceCount = 0;
    for (int i = 0; i < g_ambFile.midi.trackCount - 1; i++) { // Skip info track
        if (GetReferencedPrgmIfValid(i) == prgmIndex)
            prgmReferenceCount++;
    }

    // Delete the Prgm chunk if it's only referenced by the track we're deleting
    if (prgmReferenceCount <= 1) {
        DeletePrgmChunk(prgmIndex);
    }

    // Delete the MIDI track
    DeleteSoundTrack(trackIndex);

    // Refresh the ListView to show updated AMB data
    PopulateAmbListView();
}

void AddRow()
{
    // If no file is loaded or we're already at the limit of how many sound tracks, prgm chunks, or kmap chunks we can have one file, report an
    // error then return.
    if (g_ambFile.filePath[0] == '\0') {
        MessageBox(NULL, "No AMB file loaded. Please load a file first.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (g_ambFile.kmapChunkCount >= MAX_CHUNKS) {
        MessageBox(NULL, "Cannot add more rows: Maximum number of Kmap chunks reached.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (g_ambFile.prgmChunkCount >= MAX_CHUNKS) {
        MessageBox(NULL, "Cannot add more rows: Maximum number of Prgm chunks reached.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (g_ambFile.midi.trackCount >= MAX_SOUND_TRACKS + 1) { // +1 because first track is info track
        MessageBox(NULL, "Cannot add more rows: Maximum number of sound tracks reached.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Snapshot file
    SnapshotCurrentFile();
    
    // Find a unique name
    char uniqueName[32];
    int nameCounter = 1;
    bool nameIsUnique;
    
    do {
        snprintf(uniqueName, sizeof(uniqueName), "NewTrack%d", nameCounter++);
        nameIsUnique = true;
        
        // Check if this name is already used as a track name
        for (int i = 0; i < g_ambFile.midi.trackCount - 1; i++) { // Skip info track
            if (strcmp(g_ambFile.midi.soundTracks[i].trackName.name, uniqueName) == 0) {
                nameIsUnique = false;
                break;
            }
        }
        
        // Check if this name is already used as a var name or effect name
        if (nameIsUnique) {
            for (int i = 0; i < g_ambFile.prgmChunkCount; i++) {
                if (strcmp(g_ambFile.prgmChunks[i].varName, uniqueName) == 0 ||
                    strcmp(g_ambFile.prgmChunks[i].effectName, uniqueName) == 0) {
                    nameIsUnique = false;
                    break;
                }
            }
        }
        
        // Check if this name is already used as a kmap var name
        if (nameIsUnique) {
            for (int i = 0; i < g_ambFile.kmapChunkCount; i++) {
                if (strcmp(g_ambFile.kmapChunks[i].varName, uniqueName) == 0) {
                    nameIsUnique = false;
                    break;
                }
            }
        }
    } while (!nameIsUnique && nameCounter < 1000); // Prevent infinite loop
    
    if (!nameIsUnique) {
        MessageBox(NULL, "Failed to generate a unique name for new row.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Create a Kmap chunk
    int kmapIndex = g_ambFile.kmapChunkCount;
    KmapChunk *kmap = &g_ambFile.kmapChunks[kmapIndex];
    
    // Initialize the Kmap chunk
    memset(kmap, 0, sizeof(KmapChunk));
    
    kmap->flags = 2;  // Standard flags value in Civ3 AMBs
    kmap->itemCount = 1;
    kmap->itemSize = 12;
    
    // Set the varName to our unique name
    strcpy(kmap->varName, uniqueName);
    
    // Initialize the Kmap item
    static const unsigned char kmapItemData[12] = { 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
    memcpy(kmap->items[0].data, kmapItemData, 12);
    strcpy(kmap->items[0].wavFileName, "<.wav file name>");
    
    // Calculate the size of the Kmap chunk
    kmap->size = ComputeKmapChunkSize(kmap);
    
    // Increment the Kmap count
    g_ambFile.kmapChunkCount++;
    
    // Create a Prgm chunk
    int prgmIndex = g_ambFile.prgmChunkCount;
    PrgmChunk *prgm = &g_ambFile.prgmChunks[prgmIndex];
    
    // Initialize the Prgm chunk
    memset(prgm, 0, sizeof(PrgmChunk));
    
    // Fill in names and set number (1-based)
    strcpy(prgm->effectName, uniqueName);
    strcpy(prgm->varName, uniqueName);
    prgm->number = prgmIndex + 1;
    
    // Calculate the size of the Prgm chunk
    prgm->size = ComputePrgmChunkSize(prgm);
    
    // Increment the Prgm count
    g_ambFile.prgmChunkCount++;
    
    // Create a sound track
    int trackIndex = g_ambFile.midi.trackCount - 1; // Index for the new track (after info track)
    SoundTrack *track = &g_ambFile.midi.soundTracks[trackIndex];
    
    // Initialize the sound track
    memset(track, 0, sizeof(SoundTrack));
    
    // Set track name
    track->deltaTimeTrackName = 0;
    strcpy(track->trackName.name, uniqueName);
    
    // Add one control change event
    track->controlChangeCount = 1;
    track->deltaTimeControlChanges[0] = 0;
    track->controlChanges[0].channelNumber = prgmIndex; // 0-based index
    track->controlChanges[0].controllerNumber = 32;
    track->controlChanges[0].value = 0;
    
    // Set program change event
    track->deltaTimeProgramChange = 0;
    track->programChange.channelNumber = prgmIndex; // 0-based index
    track->programChange.programNumber = prgmIndex + 1; // 1-based number
    
    // Find a unique key for the note on/off events
    int keyValue = 60; // Start at middle C
    bool keyIsUnique;
    
    do {
        keyIsUnique = true;
        
        // Check if this key is already used in any other track
        for (int i = 0; i < g_ambFile.midi.trackCount - 1; i++) { // Skip info track
            if (i != trackIndex && g_ambFile.midi.soundTracks[i].noteOn.key == keyValue) {
                keyIsUnique = false;
                keyValue++;
                break;
            }
        }
    } while (!keyIsUnique && keyValue < 127);
    
    // Set note on event
    track->deltaTimeNoteOn = 0;
    track->noteOn.channelNumber = prgmIndex; // 0-based index
    track->noteOn.key = keyValue;
    track->noteOn.velocity = 64; // Medium velocity
    
    // Set note off event - one second after note on
    float ticksPerSecond = g_ambFile.midi.ticksPerQuarterNote / g_ambFile.midi.secondsPerQuarterNote;
    track->deltaTimeNoteOff = (int)ticksPerSecond; // 1 second duration
    track->noteOff.channelNumber = prgmIndex; // 0-based index
    track->noteOff.key = keyValue;
    track->noteOff.velocity = 64; // Same as note on
    
    // Set end of track
    track->deltaTimeEndOfTrack = 0;
    
    // Calculate the size of the sound track
    track->size = ComputeSoundTrackSize(track);
    
    // Increment the track count
    g_ambFile.midi.trackCount++;
    
    // Refresh the ListView
    PopulateAmbListView();
}

void Prune()
{
    if (g_ambFile.filePath[0] == '\0') {
        MessageBox(NULL, "No AMB file loaded. Please load a file first.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Make sure the ListView is up to date. We're going to delete any Kmap or Prgm chunks and any sound tracks that don't appear in it.
    PopulateAmbListView();

    int unneededKmapCount = 0;
    int unneededPrgmCount = 0;
    int unneededTrackCount = 0;
    bool tookSnapshot = false;

    for (int kmapIndex = g_ambFile.kmapChunkCount - 1; kmapIndex >= 0; kmapIndex--) {
        bool inAnyRow = false;
        for (int i = 0; i < g_rowCount; i++) {
            if (g_rowInfo[i].kmapIndex == kmapIndex) {
                inAnyRow = true;
                break;
            }
        }
        if (! inAnyRow) {
            if (! tookSnapshot) {
                SnapshotCurrentFile();
                tookSnapshot = true;
            }
            DeleteKmapChunk(kmapIndex);
            unneededKmapCount++;
        }
    }

    for (int prgmIndex = g_ambFile.prgmChunkCount - 1; prgmIndex >= 0; prgmIndex--) {
        bool inAnyRow = false;
        for (int i = 0; i < g_rowCount; i++) {
            if (g_rowInfo[i].prgmIndex == prgmIndex) {
                inAnyRow = true;
                break;
            }
        }
        if (! inAnyRow) {
            if (! tookSnapshot) {
                SnapshotCurrentFile();
                tookSnapshot = true;
            }
            DeletePrgmChunk(prgmIndex);
            unneededPrgmCount++;
        }
    }

    for (int trackIndex = g_ambFile.midi.trackCount - 2; trackIndex >= 0; trackIndex--) { // Skip info track
        bool inAnyRow = false;
        for (int i = 0; i < g_rowCount; i++) {
            if (g_rowInfo[i].trackIndex == trackIndex) {
                inAnyRow = true;
                break;
            }
        }
        if (! inAnyRow) {
            if (! tookSnapshot) {
                SnapshotCurrentFile();
                tookSnapshot = true;
            }
            DeleteSoundTrack(trackIndex);
            unneededTrackCount++;
        }
    }

    if ((unneededKmapCount == 0) && (unneededPrgmCount == 0) && (unneededTrackCount == 0)) {
        MessageBox(NULL, "The AMB data has no superfluous parts.", "Nothing to do", MB_OK | MB_ICONINFORMATION);
    } else {
        char msg[300] = {0};
        snprintf (msg, (sizeof msg) - 1, "Deleted %d unused Kmap chunk%s, %d unused Prgm chunk%s, and %d unused or invalid MIDI track%s.",
                  unneededKmapCount , unneededKmapCount  == 1 ? "" : "s",
                  unneededPrgmCount , unneededPrgmCount  == 1 ? "" : "s",
                  unneededTrackCount, unneededTrackCount == 1 ? "" : "s");
        MessageBox(NULL, msg, "Pruning results", MB_OK | MB_ICONINFORMATION);

        PopulateAmbListView();
    }
}

// Handle the end of a ListView label edit
BOOL HandleEndLabelEdit(HWND hwnd, NMLVDISPINFOA *pInfo)
{
    // Check if the edit was cancelled
    if (pInfo->item.pszText == NULL) {
        return FALSE; // Edit was cancelled, no update needed
    }
    
    // Get the row and column (subitem) being edited
    int row = pInfo->item.iItem;
    int col = pInfo->item.iSubItem;
    char *newText = pInfo->item.pszText;
    
    // Apply the edit to the AMB file data structure
    char formattedText[256];
    BOOL editAccepted = ApplyEditToAmbFile(hwnd, row, col, newText, formattedText, sizeof formattedText);
    if (editAccepted) {
        SetListViewItemText(g_hwndListView, row, col, formattedText, TRUE);
    }
    return editAccepted;
}

// Global variables to track the current edit control's position
int g_editRow = -1;
int g_editCol = -1;
HWND g_hwndEdit = NULL;
WNDPROC g_oldEditProc = NULL;  // Original window procedure for the edit control

// Custom window procedure for the edit control to handle keyboard input
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            switch (wParam)
            {
                case VK_RETURN:
                    // Enter key - accept changes and destroy edit control
                    if (g_hwndEdit != NULL && g_editRow >= 0 && g_editCol >= 0) {
                        char buffer[256];
                        GetWindowText(g_hwndEdit, buffer, sizeof(buffer));
                        
                        // Apply the edit to the AMB file data structure and update the ListView with the formatted text
                        char formattedBuffer[256];
                        if (ApplyEditToAmbFile(GetParent(GetParent(hwnd)), g_editRow, g_editCol, buffer, formattedBuffer, sizeof formattedBuffer)) {
                            SetListViewItemText(g_hwndListView, g_editRow, g_editCol, formattedBuffer, TRUE);
                        }
                        
                        // Destroy the edit control
                        DestroyWindow(g_hwndEdit);
                        g_hwndEdit = NULL;
                        g_editRow = -1;
                        g_editCol = -1;
                    }
                    return 0;
                    
                case VK_ESCAPE:
                    // Escape key - cancel edit
                    if (g_hwndEdit != NULL) {
                        DestroyWindow(g_hwndEdit);
                        g_hwndEdit = NULL;
                        g_editRow = -1;
                        g_editCol = -1;
                    }
                    return 0;
            }
            break;
    }
    
    // Call the original window procedure for other messages
    return CallWindowProc(g_oldEditProc, hwnd, uMsg, wParam, lParam);
}

// Returns the currently selected row and whether or not any was selected. Optionally displays an error when none is selected.
bool GetSelectedRow(char const * errorMsg, int * outSelectedRow)
{
    int rowCount = SendMessage(g_hwndListView, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < rowCount; i++) {
        UINT state = SendMessage(g_hwndListView, LVM_GETITEMSTATE, i, LVIS_SELECTED);
        if (state & LVIS_SELECTED) {
            *outSelectedRow = i;
            return true;
        }
    }

    // If we get here, no row was selected
    if (errorMsg)
        MessageBox(NULL, errorMsg, "Information", MB_OK | MB_ICONINFORMATION);
    return false;
}

// Function to match duration to WAV file for the selected row
void MatchDurationToWav(HWND hwndListView)
{
    int selectedRow;
    if (! GetSelectedRow("Please select a row to match duration", &selectedRow))
        return;
    
    // Make sure we have valid row info
    if (selectedRow >= g_rowCount) {
        MessageBox(NULL, "Invalid row selection", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Get the WAV filename from the selected row
    char wavFileName[256] = {0};
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = selectedRow;
    lvi.iSubItem = COL_WAV_FILE;
    lvi.pszText = wavFileName;
    lvi.cchTextMax = sizeof(wavFileName);
    SendMessage(hwndListView, LVM_GETITEMTEXT, selectedRow, (LPARAM)&lvi);
    
    if (strlen(wavFileName) == 0) {
        MessageBox(NULL, "No WAV filename found in selected row", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Construct full path to WAV file (assuming it's in the same directory as the AMB file)
    Path wavFilePath;
    strcpy(wavFilePath, g_ambFile.filePath);
    PathRemoveFileSpec(wavFilePath);  // Remove filename, keep directory
    PathAppend(wavFilePath, wavFileName);
    
    // Get WAV file duration
    float wavDuration;
    if (!GetWavFileDuration(wavFilePath, &wavDuration)) {
        char errorMsg[512];
        snprintf(errorMsg, sizeof(errorMsg), "Could not read duration from WAV file:\n%s\n\nMake sure the file exists and is a valid WAV file.", wavFilePath);
        MessageBox(NULL, errorMsg, "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Convert duration to string and apply it to the AMB file
    char durationStr[32];
    snprintf(durationStr, sizeof(durationStr), "%.3f", wavDuration);
    
    char formattedText[256];
    if (ApplyEditToAmbFile(GetParent(hwndListView), selectedRow, COL_DURATION, durationStr, formattedText, sizeof(formattedText))) {
        SetListViewItemText(hwndListView, selectedRow, COL_DURATION, formattedText, TRUE);
    } else {
        MessageBeep(MB_ICONERROR);
    }
}

// Function to programmatically start editing a ListView subitem
void EditListViewSubItem(HWND hwndListView, int row, int col)
{
    // First select the item
    LVITEMA lvi = {0};
    lvi.stateMask = LVIS_SELECTED;
    lvi.state = LVIS_SELECTED;
    SendMessage(hwndListView, LVM_SETITEMSTATE, row, (LPARAM)&lvi);
    
    // For subitems, we need to create a custom edit control
    // First get the text of the subitem
    char buffer[256] = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row;
    lvi.iSubItem = col;
    lvi.pszText = buffer;
    lvi.cchTextMax = sizeof(buffer);
    SendMessage(hwndListView, LVM_GETITEMTEXT, row, (LPARAM)&lvi);
    
    // Create a structure for LVM_GETSUBITEMRECT
    // The struct needs to have subItem in top and flags in left
    RECT rect;
    memset(&rect, 0, sizeof(RECT));
    rect.top = col;  // subItem index
    rect.left = LVIR_BOUNDS;  // Type of rectangle (LVIR_BOUNDS for entire cell)
    SendMessage(hwndListView, LVM_GETSUBITEMRECT, row, (LPARAM)&rect);
    
    // If we already have an edit control, destroy it
    if (g_hwndEdit != NULL) {
        DestroyWindow(g_hwndEdit);
        g_hwndEdit = NULL;
    }
    
    // For column 0 (Time), we need to adjust the width to match the column width
    // instead of using the full row width that LVIR_BOUNDS returns
    if (col == COL_TIME) {
        // Get the width of column 0
        LVCOLUMNA lvc = {0};
        lvc.mask = LVCF_WIDTH;
        SendMessage(hwndListView, LVM_GETCOLUMN, COL_TIME, (LPARAM)&lvc);
        
        // Adjust the rectangle to match the column width
        rect.right = rect.left + lvc.cx;
    }
    
    // Create an edit control over the subitem
    g_hwndEdit = CreateWindow(
        "EDIT", buffer,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        hwndListView, (HMENU)1000, GetModuleHandle(NULL), NULL);
    
    if (g_hwndEdit) {
        // Store which row and column we're editing
        g_editRow = row;
        g_editCol = col;
        
        // Subclass the edit control to handle keyboard input
        g_oldEditProc = (WNDPROC)SetWindowLongPtr(g_hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
        
        // Set focus to the edit control and select all text
        SetFocus(g_hwndEdit);
        SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);
    }
}

// Calculate and update the overall duration display
void UpdateOverallDuration(void)
{
    if (g_hwndDurationLabel == NULL) {
        return;
    }
    
    float maxEndTime = 0.0f;
    
    // Check if we have valid AMB data
    if (g_ambFile.kmapChunkCount == 0 || g_ambFile.prgmChunkCount == 0 || g_ambFile.midi.trackCount == 0) {
        SetWindowText(g_hwndDurationLabel, "Overall Duration: 0.000 sec");
        return;
    }
    
    // Calculate seconds per tick for timing
    float secondsPerTick = 0.0f;
    if (g_ambFile.midi.ticksPerQuarterNote > 0) {
        secondsPerTick = g_ambFile.midi.secondsPerQuarterNote / g_ambFile.midi.ticksPerQuarterNote;
    }
    
    // Process each MIDI sound track to find the longest duration
    int soundTrackCount = g_ambFile.midi.trackCount - 1; // Minus one to exclude the info track
    for (int trackIndex = 0; trackIndex < soundTrackCount; trackIndex++) {
        SoundTrack * track = &g_ambFile.midi.soundTracks[trackIndex];
        
        int prgmIndex = GetReferencedPrgmIfValid(trackIndex);
        if (prgmIndex < 0)
            continue; // No valid Prgm, skip this track
        
        float timestamp = track->deltaTimeNoteOn * secondsPerTick;
        float duration = track->deltaTimeNoteOff * secondsPerTick;
        float endTime = timestamp + duration;
        
        if (endTime > maxEndTime) {
            maxEndTime = endTime;
        }
    }
    
    // Update the label text
    char durationText[64] = {0};
    snprintf(durationText, sizeof(durationText) - 1, "Overall Duration: %04.3f sec", maxEndTime);
    SetWindowText(g_hwndDurationLabel, durationText);
}

// Create and initialize the ListView control
void CreateAmbListView(HWND hwnd) 
{
    // Create ListView control below the playback buttons
    g_hwndListView = CreateWindow(
        WC_LISTVIEW,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOHSCROLL | LVS_EDITLABELS,
        20, 70, // x, y position (below the buttons)
        835, 460, // width, height (fill most of the window)
        hwnd,
        (HMENU)ID_AMB_LISTVIEW,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (g_hwndListView == NULL) {
        MessageBox(hwnd, "Could not create ListView control", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Set extended ListView styles
    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
    SendMessage(g_hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, exStyle);
    
    // Add columns to the ListView
    AddListViewColumn(g_hwndListView, COL_TIME, "Time (sec.)", 85);
    AddListViewColumn(g_hwndListView, COL_DURATION, "Duration (sec.)", 95);
    AddListViewColumn(g_hwndListView, COL_WAV_FILE, "WAV File", 235);
    AddListViewColumn(g_hwndListView, COL_SPEED_RANDOM, "Speed Rnd", 70);
    AddListViewColumn(g_hwndListView, COL_SPEED_MIN, "Speed Min", 70);
    AddListViewColumn(g_hwndListView, COL_SPEED_MAX, "Speed Max", 70);
    AddListViewColumn(g_hwndListView, COL_VOLUME_RANDOM, "Vol. Rnd", 70);
    AddListViewColumn(g_hwndListView, COL_VOLUME_MIN, "Vol. Min", 70);
    AddListViewColumn(g_hwndListView, COL_VOLUME_MAX, "Vol. Max", 70);
    
    // Create duration label below the ListView
    g_hwndDurationLabel = CreateWindow(
        "STATIC",
        "Overall Duration: 0.000 sec",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 540, // x, y position (below the ListView)
        250, 20, // width, height
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            // Read INI settings
            {
                Path iniFullPath;
                GetCurrentDirectory(PATH_BUFFER_SIZE, iniFullPath);
                PathAppend(iniFullPath, "amb_editor.ini");

                GetPrivateProfileString("Paths", "civ_3_install_path", "", g_iniCiv3InstallPath, PATH_BUFFER_SIZE, iniFullPath);
                GetPrivateProfileString("Paths", "sound_dll_path"    , "", g_iniSoundDLLPath   , PATH_BUFFER_SIZE, iniFullPath);
                GetPrivateProfileString("Paths", "temp_directory"    , "", g_iniTempDirectory  , PATH_BUFFER_SIZE, iniFullPath);
            }

            // If we've been given a specific Civ 3 install path in the INI, use that. Otherwise search for one.
            if (strlen(g_iniCiv3InstallPath) > 0) {
                strcpy(g_civ3MainPath, g_iniCiv3InstallPath);
            } else {
                FindCiv3Installation(hwnd);
            }

            g_hwndMainWindow = hwnd;
            
            // Create playback buttons
            CreatePlaybackButtons(hwnd);
            
            // Create the AMB ListView control
            CreateAmbListView(hwnd);

            return 0;
            
        case WM_COMMAND:
            // Handle menu commands
            switch (LOWORD(wParam))
            {
                case IDM_FILE_OPEN:
                    // Show open file dialog to load an AMB file
                    LoadAmbFileWithDialog(hwnd);
                    return 0;
                    
                case IDM_FILE_SAVE:
                    // Save AMB file directly
                    SaveAmbFileDirectly(hwnd);
                    return 0;
                    
                case IDM_FILE_SAVE_AS:
                    // Show save file dialog to save AMB file as
                    SaveAmbFileAs(hwnd);
                    return 0;

                case IDM_FILE_DETAILS:
                    // Show window with AMB file details
                    ShowAmbDetailsWindow(hwnd);
                    return 0;

                case IDM_FILE_EXIT:
                    // Exit the application
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                    
                case IDM_EDIT_UNDO:
                    // Perform undo operation
                    Undo();
                    return 0;
                    
                case IDM_EDIT_REDO:
                    // Perform redo operation
                    Redo();
                    return 0;
                    
                case IDM_EDIT_ADD:
                    // Add a new row
                    AddRow();
                    return 0;
                    
                case IDM_EDIT_DELETE:
                {
                    // Delete the selected row
                    int selectedRow;
                    if (GetSelectedRow("Please select a row to delete", &selectedRow))
                        DeleteRow(selectedRow);

                    return 0;
                }
                    
                case IDM_EDIT_MATCH_WAV:
                    // Match duration to WAV file
                    MatchDurationToWav(g_hwndListView);
                    return 0;
                    
                case IDM_EDIT_PRUNE:
                    Prune();
                    return 0;

                case IDM_HELP_ABOUT:
                    // Show about dialog
                    MessageBox(hwnd, 
                               "This is a tool for inspecting and modifying the AMB sound files used by Sid Meier's Civilization III.\n\n"
                               "Last updated in C3X version 23 Preview 2\n\n"
                               "For more info, see README.txt",
                               "About AMB Editor",
                               MB_OK | MB_ICONINFORMATION);
                    return 0;
                    
                case ID_PLAY_BUTTON:
                    // Handle Play button click
                    if (g_ambFile.filePath[0] != '\0') {
                        bool encounteredError = false;

                        // Check that the preview player is initialized. If it's not, try to initialize it.
                        if (! previewPlayerIsInitialized) {
                            Path soundDLLPath;
                            if (GetSoundDLLPath(hwnd, soundDLLPath)) {
                                InitializePreviewPlayer(hwnd, soundDLLPath);
                            }
                            if (! previewPlayerIsInitialized)
                                encounteredError = true;
                        }

                        // Check for missing WAV files before attempting preview
                        if (! encounteredError) {
                            char missingFiles[1024];
                            if (HasMissingWavFiles(missingFiles, sizeof(missingFiles))) {
                                char errorMsg[1280];
                                snprintf(errorMsg, sizeof(errorMsg),
                                         "Cannot preview AMB file because the following WAV files are missing:\n\n%s\n\nPlease ensure all WAV files are present in the AMB file's directory.", 
                                         missingFiles);
                                MessageBox(hwnd, errorMsg, "Missing WAV Files", MB_OK | MB_ICONERROR);
                            }
                        }

                        if (! encounteredError)
                            PreviewAmbFile(g_iniTempDirectory, &g_ambFile);
                    } else {
                        MessageBox(hwnd, "No AMB file loaded. Please open an AMB file first.", 
                                   "Error", MB_OK | MB_ICONINFORMATION);
                    }
                    return 0;
                    
                case ID_STOP_BUTTON:
                    // Handle Stop button click
                    StopAmbPreview();
                    return 0;
                
                // Check if the command is from the edit control created for subitem editing
                default:
                    if (LOWORD(wParam) == 1000 && HIWORD(wParam) == EN_KILLFOCUS) {
                        // When the edit control loses focus, get its text and update the list view item
                        if (g_hwndEdit != NULL && g_editRow >= 0 && g_editCol >= 0) {
                            char buffer[256];
                            GetWindowText(g_hwndEdit, buffer, sizeof(buffer));
                            
                            // Apply the edit to the AMB file data structure
                            char formattedBuffer[256];
                            if (ApplyEditToAmbFile(hwnd, g_editRow, g_editCol, buffer, formattedBuffer, sizeof formattedBuffer)) {
                                SetListViewItemText(g_hwndListView, g_editRow, g_editCol, formattedBuffer, TRUE);
                            }
                            
                            // Destroy the edit control
                            DestroyWindow(g_hwndEdit);
                            g_hwndEdit = NULL;
                            g_editRow = -1;
                            g_editCol = -1;
                        }
                        return 0;
                    }
            }
            break;
            
        case WM_NOTIFY:
            // Handle control notifications
            {
                NMHDR *nmhdr = (NMHDR *)lParam;
                
                // Check if the notification is from our ListView
                if (nmhdr->hwndFrom == g_hwndListView) {
                    switch (nmhdr->code) {
                        case LVN_BEGINLABELEDIT:
                            // Notification when a label edit begins
                            return HandleBeginLabelEdit(hwnd, (NMLVDISPINFOA *)lParam);
                            
                        case LVN_ENDLABELEDIT:
                            // Notification when a label edit ends
                            return HandleEndLabelEdit(hwnd, (NMLVDISPINFOA *)lParam);
                            
                        case NM_DBLCLK:
                            // Handle double-click on a ListView item to edit the cell
                            {
                                NMITEMACTIVATE *nia = (NMITEMACTIVATE *)lParam;
                                // Only process if a valid item was clicked
                                if (nia->iItem != -1) {
                                    // Check if this is a boolean column
                                    if (nia->iSubItem == COL_SPEED_RANDOM || nia->iSubItem == COL_VOLUME_RANDOM) {
                                        // Toggle the boolean value directly
                                        char currentText[32];
                                        LVITEMA lvi = {0};
                                        lvi.mask = LVIF_TEXT;
                                        lvi.iItem = nia->iItem;
                                        lvi.iSubItem = nia->iSubItem;
                                        lvi.pszText = currentText;
                                        lvi.cchTextMax = sizeof(currentText);
                                        SendMessage(g_hwndListView, LVM_GETITEMTEXT, nia->iItem, (LPARAM)&lvi);
                                        
                                        // Toggle: if current is "Yes", change to "No", otherwise change to "Yes"
                                        const char *newText = (strcmp(currentText, "Yes") == 0) ? "No" : "Yes";
                                        
                                        // Apply the toggle to the AMB file data structure
                                        char formattedText[256];
                                        if (ApplyEditToAmbFile(hwnd, nia->iItem, nia->iSubItem, newText, formattedText, sizeof(formattedText))) {
                                            SetListViewItemText(g_hwndListView, nia->iItem, nia->iSubItem, formattedText, TRUE);
                                        }
                                    } else {
                                        // For non-boolean columns, start editing the subitem that was clicked
                                        EditListViewSubItem(g_hwndListView, nia->iItem, nia->iSubItem);
                                    }
                                    
                                    // Tell the system we handled this message
                                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                                    return TRUE;
                                }
                            }
                            break;
                            
                        case NM_CUSTOMDRAW:
                            // Handle custom drawing for ListView
                            {
                                LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                                
                                switch (lplvcd->nmcd.dwDrawStage) {
                                    case CDDS_PREPAINT:
                                        // Tell Windows we want to be notified for each item
                                        return CDRF_NOTIFYITEMDRAW;
                                        
                                    case CDDS_ITEMPREPAINT:
                                        // Tell Windows we want to be notified for each subitem
                                        return CDRF_NOTIFYSUBITEMDRAW;
                                        
                                    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                                        // Custom draw for individual cells
                                        {
                                            int item = (int)lplvcd->nmcd.dwItemSpec;
                                            int subItem = lplvcd->iSubItem;
                                            
                                            // Check if this is the WAV file column
                                            if (subItem == COL_WAV_FILE) {
                                                // Get the WAV filename for this row
                                                char wavFileName[256] = {0};
                                                LVITEMA lvi = {0};
                                                lvi.mask = LVIF_TEXT;
                                                lvi.iItem = item;
                                                lvi.iSubItem = COL_WAV_FILE;
                                                lvi.pszText = wavFileName;
                                                lvi.cchTextMax = sizeof(wavFileName);
                                                SendMessage(g_hwndListView, LVM_GETITEMTEXT, item, (LPARAM)&lvi);
                                                
                                                // Check if WAV file exists
                                                if (!CheckWavFileExists(wavFileName)) {
                                                    // Set text color to red for missing files
                                                    lplvcd->clrText = RGB(255, 0, 0);
                                                } else {
                                                    // Use default text color
                                                    lplvcd->clrText = GetSysColor(COLOR_WINDOWTEXT);
                                                }
                                                
                                                return CDRF_NEWFONT;
                                            } else if (subItem == COL_SPEED_MIN || subItem == COL_SPEED_MAX || 
                                                      subItem == COL_VOLUME_MIN || subItem == COL_VOLUME_MAX) {
                                                // Check if min/max columns should be grayed out based on flags
                                                if (item < g_rowCount) {
                                                    AmbRowInfo *rowInfo = &g_rowInfo[item];
                                                    if (rowInfo->prgmIndex >= 0 && rowInfo->prgmIndex < g_ambFile.prgmChunkCount) {
                                                        PrgmChunk *prgm = &g_ambFile.prgmChunks[rowInfo->prgmIndex];
                                                        
                                                        BOOL shouldGrayOut = FALSE;
                                                        if (subItem == COL_SPEED_MIN || subItem == COL_SPEED_MAX) {
                                                            // Gray out if speed random is disabled (flags & 1 == 0)
                                                            shouldGrayOut = !(prgm->flags & 0x01);
                                                        } else {
                                                            // Gray out if volume random is disabled (flags & 2 == 0)
                                                            shouldGrayOut = !(prgm->flags & 0x02);
                                                        }
                                                        
                                                        if (shouldGrayOut) {
                                                            lplvcd->clrText = GetSysColor(COLOR_GRAYTEXT);
                                                        } else {
                                                            lplvcd->clrText = GetSysColor(COLOR_WINDOWTEXT);
                                                        }
                                                        
                                                        return CDRF_NEWFONT;
                                                    }
                                                }
                                                
                                                // Fallback to default color
                                                lplvcd->clrText = GetSysColor(COLOR_WINDOWTEXT);
                                                return CDRF_NEWFONT;
                                            } else {
                                                // For all other columns, use default text color
                                                lplvcd->clrText = GetSysColor(COLOR_WINDOWTEXT);
                                                return CDRF_NEWFONT;
                                            }
                                        }
                                }
                            }
                            return CDRF_DODEFAULT;
                    }
                }
            }
            break;
            
        case WM_ERASEBKGND:
            {
                // Handle window background erasure
                HDC hdc = (HDC)wParam;
                RECT rect;
                GetClientRect(hwnd, &rect);
                FillRect(hdc, &rect, g_hBackgroundBrush);
                return 1;  // Return non-zero to indicate we processed this message
            }
            
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            // Set background color for dialog, static controls, and buttons
            return (LRESULT)g_hBackgroundBrush;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            // Clean up resources
            if (g_hBackgroundBrush != NULL) {
                DeleteObject(g_hBackgroundBrush);
                g_hBackgroundBrush = NULL;
            }
            DeinitializePreviewPlayer();
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Create a simple menu
HMENU CreateAmbEditorMenu()
{
    HMENU hMenu, hFileMenu, hEditMenu, hHelpMenu;
    
    hMenu = CreateMenu();
    
    // File menu
    hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open AMB File...\tCtrl+O");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE, "&Save\tCtrl+S");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE_AS, "Save &As...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_DETAILS, "&View AMB Details");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, "E&xit");
    
    // Edit menu
    hEditMenu = CreatePopupMenu();
    AppendMenu(hEditMenu, MF_STRING, IDM_EDIT_UNDO, "&Undo\tCtrl+Z");
    AppendMenu(hEditMenu, MF_STRING, IDM_EDIT_REDO, "&Redo\tCtrl+Y");
    AppendMenu(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hEditMenu, MF_STRING, IDM_EDIT_ADD, "&Add Row\tIns");
    AppendMenu(hEditMenu, MF_STRING, IDM_EDIT_DELETE, "&Delete Row\tDel");
    AppendMenu(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hEditMenu, MF_STRING, IDM_EDIT_MATCH_WAV, "&Match duration to WAV");
    AppendMenu(hEditMenu, MF_STRING, IDM_EDIT_PRUNE, "&Prune");
    
    // Help menu
    hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, "&About...");
    
    // Add menus to the main menu
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "&File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hEditMenu, "&Edit");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, "&Help");
    
    return hMenu;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Register the window class
    const char CLASS_NAME[] = "AMB Editor Window Class";
    
    // Create a pleasant beige brush for the background - RGB(245, 245, 220) is a nice beige color
    g_hBackgroundBrush = CreateSolidBrush(RGB(245, 235, 200));
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_hBackgroundBrush;
    
    RegisterClass(&wc);
    
    // Create menu
    HMENU hMenu = CreateAmbEditorMenu();
    
    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                          // Optional window styles
        CLASS_NAME,                 // Window class
        "AMB Editor",               // Window title
        WS_OVERLAPPEDWINDOW,        // Window style
        
        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 895, 620,
        
        NULL,       // Parent window    
        hMenu,      // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );
    
    if (hwnd == NULL) {
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    
    // Create accelerator table
    HACCEL hAccel = CreateAcceleratorTable(g_accelTable, sizeof(g_accelTable) / sizeof(g_accelTable[0]));
    
    // Run the message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return (int)msg.wParam;
}
