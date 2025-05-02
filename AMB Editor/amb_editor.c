#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "preview.c"
#include "amb_file.c"

// Forward declarations for ListView functions
void AddListViewColumn(HWND hListView, int index, char *title, int width);
int AddListViewItem(HWND hListView, int index, const char *text);
void SetListViewItemText(HWND hListView, int row, int col, const char *text);
void FormatTimeString(char *buffer, int bufferSize, float timestamp);
void ClearListView(HWND hListView);
void PopulateAmbListView(void);

// Common Control definitions (normally from commctrl.h)
#define WC_LISTVIEW "SysListView32"
#define LVS_REPORT 0x0001
#define LVS_SHOWSELALWAYS 0x0008
#define LVS_SINGLESEL 0x0004
#define LVS_EDITLABELS 0x0200
#define LVS_NOHSCROLL 0x8000
#define LVM_INSERTCOLUMN (LVM_FIRST + 27)
#define LVM_INSERTITEM (LVM_FIRST + 7)
#define LVM_SETITEM (LVM_FIRST + 6)
#define LVM_SETITEMSTATE (LVM_FIRST + 43)
#define LVM_GETITEMCOUNT (LVM_FIRST + 4)
#define LVM_DELETEALLITEMS (LVM_FIRST + 9)
#define LVM_GETITEMSTATE (LVM_FIRST + 44)
#define LVM_FIRST 0x1000
#define LVCF_TEXT 0x0004
#define LVCF_WIDTH 0x0002
#define LVIF_TEXT 0x0001
#define LVIF_STATE 0x0008
#define LVIS_STATEIMAGEMASK 0xF000
#define LVS_EX_GRIDLINES 0x00000001
#define LVS_EX_FULLROWSELECT 0x00000020
#define LVS_EX_CHECKBOXES 0x00000004
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 55)

typedef struct tagLVCOLUMNA {
    UINT mask;
    int fmt;
    int cx;
    LPSTR pszText;
    int cchTextMax;
    int iSubItem;
    int iImage;
    int iOrder;
} LVCOLUMNA, *LPLVCOLUMNA;

typedef struct tagLVITEMA {
    UINT mask;
    int iItem;
    int iSubItem;
    UINT state;
    UINT stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent;
} LVITEMA, *LPLVITEMA;

// Minimal declarations for common dialogs (normally from commdlg.h)
#define OFN_PATHMUSTEXIST    0x00000800
#define OFN_FILEMUSTEXIST    0x00001000
#define OFN_HIDEREADONLY     0x00000004
#define OFN_NOCHANGEDIR      0x00000008
#define OFN_EXPLORER         0x00080000

typedef struct tagOFNA {
    DWORD        lStructSize;
    HWND         hwndOwner;
    HINSTANCE    hInstance;
    LPCSTR       lpstrFilter;
    LPSTR        lpstrCustomFilter;
    DWORD        nMaxCustFilter;
    DWORD        nFilterIndex;
    LPSTR        lpstrFile;
    DWORD        nMaxFile;
    LPSTR        lpstrFileTitle;
    DWORD        nMaxFileTitle;
    LPCSTR       lpstrInitialDir;
    LPCSTR       lpstrTitle;
    DWORD        Flags;
    WORD         nFileOffset;
    WORD         nFileExtension;
    LPCSTR       lpstrDefExt;
    LPARAM       lCustData;
    LPVOID       lpfnHook;
    LPCSTR       lpTemplateName;
    void*        pvReserved;
    DWORD        dwReserved;
    DWORD        FlagsEx;
} OPENFILENAMEA, *LPOPENFILENAMEA;

// Function declarations
BOOL WINAPI GetOpenFileNameA(LPOPENFILENAMEA lpofn);
BOOL WINAPI GetSaveFileNameA(LPOPENFILENAMEA lpofn);

// Menu IDs
#define IDM_FILE_OPEN       1001
#define IDM_FILE_EXIT       1002
#define IDM_HELP_ABOUT      1003

// Control IDs 
#define ID_PATH_EDIT        102
#define ID_PLAY_BUTTON      103
#define ID_STOP_BUTTON      104
#define ID_AMB_LISTVIEW     105

// Global variables
char g_civ3MainPath[MAX_PATH_LENGTH] = {0};
char g_civ3ConquestsPath[MAX_PATH_LENGTH] = {0};
HWND g_hwndPathEdit = NULL;
HWND g_hwndMainWindow = NULL;
HWND g_hwndPlayButton = NULL;
HWND g_hwndStopButton = NULL;
HWND g_hwndListView = NULL;
HBRUSH g_hBackgroundBrush = NULL;  // Brush for window background color

// Custom path helpers 
bool PathFileExists(const char* path)
{
    DWORD attr = GetFileAttributes(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool PathIsDirectory(const char* path)
{
    DWORD attr = GetFileAttributes(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

void PathAppend(char* path, const char* append)
{
    size_t len = strlen(path);
    
    // Add backslash if needed
    if (len > 0 && path[len - 1] != '\\') {
        path[len] = '\\';
        path[len + 1] = '\0';
        len++;
    }
    
    // Append the new part
    strcpy(path + len, append);
}

bool PathRemoveFileSpec(char* path)
{
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
        return true;
    }
    return false;
}

// Function to browse for an AMB file to open
bool BrowseForAmbFile(HWND hwnd, char *filePath, int bufferSize)
{
    // Initialize the OPENFILENAME structure
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    
    // Set up buffer
    ZeroMemory(filePath, bufferSize);
    
    // Setup the open file dialog
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = bufferSize;
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

// Function to browse for a directory (used for Civ3 installation)
bool BrowseForDirectory(HWND hwnd, char *dirPath, int bufferSize, const char *title)
{
    // A trick: use the file open dialog but look for a file we know will exist
    // in the directory we want (like .exe files)
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    
    // Set up buffer
    ZeroMemory(dirPath, bufferSize);
    
    // Setup the open file dialog
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = dirPath;
    ofn.nMaxFile = bufferSize;
    ofn.lpstrFilter = "Executable Files\0*.exe\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    
    // Show the dialog and get result
    if (GetOpenFileNameA(&ofn)) {
        // Extract directory path from the selected file
        char* lastSlash = strrchr(dirPath, '\\');
        if (lastSlash) {
            *lastSlash = '\0'; // Truncate at last backslash to get directory
            return true;
        }
    }
    
    return false;
}

// Load an AMB file selected by the user
void LoadAmbFileWithDialog(HWND hwnd)
{
    char ambFilePath[MAX_PATH_LENGTH] = {0};
    
    if (BrowseForAmbFile(hwnd, ambFilePath, MAX_PATH_LENGTH)) {
        if (LoadAmbFile(ambFilePath) && g_hwndMainWindow != NULL) {
            // Extract the filename part from the path
            char *fileName = strrchr(ambFilePath, '\\');
            if (fileName) {
                fileName++; // Skip past the backslash
            } else {
                fileName = (char*)ambFilePath; // No backslash found, use the whole path
            }
            
            char windowTitle[MAX_PATH_LENGTH + 20];
            snprintf(windowTitle, sizeof windowTitle, "%s - AMB Editor", fileName);
            windowTitle[(sizeof windowTitle) - 1] = '\0';
            SetWindowText(g_hwndMainWindow, windowTitle);
            
            // Populate the ListView with the loaded AMB file data
            PopulateAmbListView();
        }
    }
}

// Test AMB file loading
void TestAmbLoading(HWND hwnd)
{
    // Find a sample AMB file to load
    if (strlen(g_civ3MainPath) > 0) {
        char ambFilePath[MAX_PATH_LENGTH];
        strcpy(ambFilePath, g_civ3MainPath);
        PathAppend(ambFilePath, "Art");
        PathAppend(ambFilePath, "Units");
        PathAppend(ambFilePath, "Infantry");
        PathAppend(ambFilePath, "InfantryAttack.amb");  // Use the same file from the example
        
        if (PathFileExists(ambFilePath)) {
            if (LoadAmbFile(ambFilePath)) {
                // Extract the filename part for the window title
                char *fileName = strrchr(ambFilePath, '\\');
                if (fileName) {
                    fileName++; // Skip past the backslash
                } else {
                    fileName = (char*)ambFilePath;
                }
                
                char windowTitle[MAX_PATH_LENGTH + 20];
                snprintf(windowTitle, sizeof windowTitle, "%s - AMB Editor", fileName);
                windowTitle[(sizeof windowTitle) - 1] = '\0';
                SetWindowText(g_hwndMainWindow, windowTitle);
                
                // Populate the ListView with the loaded AMB file data
                PopulateAmbListView();
                
                // Successfully loaded AMB file - write description to file
                char *description = (char *)malloc(50000);
                if (description) {
                    DescribeAmbFile(description, 50000);
                    
                    // Write to file
                    char outputPath[MAX_PATH_LENGTH];
                    GetCurrentDirectory(MAX_PATH_LENGTH, outputPath);
                    PathAppend(outputPath, "amb_info.txt");
                    
                    FILE *outFile = fopen(outputPath, "w");
                    if (outFile) {
                        fputs(description, outFile);
                        fclose(outFile);
                        
                        char message[MAX_PATH_LENGTH + 64];
                        sprintf(message, "AMB file information written to:\n%s", outputPath);
                        MessageBox(hwnd, message, "AMB File Loaded Successfully", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBox(hwnd, "Failed to write AMB info to file", "Error", MB_OK | MB_ICONERROR);
                    }
                    
                    free(description);
                }
            }
        } else {
            MessageBox(hwnd, "Sample AMB file not found", "Error", MB_OK | MB_ICONERROR);
        }
    } else {
        MessageBox(hwnd, "Civ 3 installation not found", "Error", MB_OK | MB_ICONERROR);
    }
}

// Function to check if a directory has the required Civ3 files
bool IsCiv3ConquestsFolder(const char* folderPath)
{
    char testPath[MAX_PATH_LENGTH];
    
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
bool IsCiv3MainFolder(const char* folderPath)
{
    char testPath[MAX_PATH_LENGTH];
    
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
bool FindCiv3InstallBySearch(const char* startPath)
{
    char testPath[MAX_PATH_LENGTH];
    char parentPath[MAX_PATH_LENGTH];
    bool foundConquests = false;
    
    // Start with working directory
    strcpy(testPath, startPath);
    
    // Go up up to 5 parent directories
    for (int i = 0; i < 5; i++) {
        // Check current directory and subdirectories for Conquests
        WIN32_FIND_DATA findData;
        HANDLE hFind;
        char searchPath[MAX_PATH_LENGTH];
        
        strcpy(searchPath, testPath);
        PathAppend(searchPath, "*");
        
        hFind = FindFirstFile(searchPath, &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
                    strcmp(findData.cFileName, ".") != 0 && 
                    strcmp(findData.cFileName, "..") != 0) {
                    
                    char subDirPath[MAX_PATH_LENGTH];
                    strcpy(subDirPath, testPath);
                    PathAppend(subDirPath, findData.cFileName);
                    
                    if (IsCiv3ConquestsFolder(subDirPath)) {
                        // Found Conquests directory
                        strcpy(g_civ3ConquestsPath, subDirPath);
                        foundConquests = true;
                        break;
                    }
                }
            } while (FindNextFile(hFind, &findData));
            FindClose(hFind);
        }
        
        if (foundConquests) {
            // Now look for the main Civ3 folder (should be parent of Conquests parent)
            strcpy(parentPath, g_civ3ConquestsPath);
            PathRemoveFileSpec(parentPath); // Go up to Conquests parent
            
            if (IsCiv3MainFolder(parentPath)) {
                strcpy(g_civ3MainPath, parentPath);
                return true;
            }
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
    char regValue[MAX_PATH_LENGTH];
    DWORD valueSize = sizeof(regValue);
    DWORD valueType;
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Infogrames Interactive\\Civilization III", 
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        if (RegQueryValueEx(hKey, "Install_Path", NULL, &valueType, 
                           (LPBYTE)regValue, &valueSize) == ERROR_SUCCESS) {
            
            if (valueType == REG_SZ) {
                strcpy(g_civ3MainPath, regValue);
                
                // Assume Conquests is in a subfolder named "Conquests"
                strcpy(g_civ3ConquestsPath, g_civ3MainPath);
                PathAppend(g_civ3ConquestsPath, "Conquests");
                
                RegCloseKey(hKey);
                return true;
            }
        }
        RegCloseKey(hKey);
    }
    
    return false;
}

// Validate a manually entered Civ3 folder path
bool BrowseForCiv3Install(HWND hwnd)
{
    // Get path from dialog
    char path[MAX_PATH_LENGTH] = {0};
    
    if (BrowseForDirectory(hwnd, path, MAX_PATH_LENGTH, "Select your Civilization III installation folder")) {
        // Check if it's a valid Civ3 directory
        if (IsCiv3MainFolder(path)) {
            strcpy(g_civ3MainPath, path);
            
            // Assume Conquests is in a subfolder named "Conquests"
            strcpy(g_civ3ConquestsPath, g_civ3MainPath);
            PathAppend(g_civ3ConquestsPath, "Conquests");
            
            return true;
        } else {
            MessageBox(hwnd, "The selected folder does not appear to be a valid Civilization III installation.", 
                      "Invalid Selection", MB_OK | MB_ICONWARNING);
        }
    }
    
    return false;
}

// Find Civ3 installation using all available methods
bool FindCiv3Installation(HWND hwnd)
{
    char workingDir[MAX_PATH_LENGTH];
    GetCurrentDirectory(MAX_PATH_LENGTH, workingDir);
    
    // Method 1: Search parent folders
    if (FindCiv3InstallBySearch(workingDir)) {
        return true;
    }
    
    // Method 2: Check registry
    if (FindCiv3InstallFromRegistry()) {
        return true;
    }
    
    // Method 3: Ask user to manually enter the path
    int result = MessageBox(hwnd, 
        "Civilization III installation not found. Would you like to enter the path manually?", 
        "Installation not found", MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES) {
        return BrowseForCiv3Install(hwnd);
    }
    
    return false;
}

// Create the play and stop buttons
void CreatePlaybackButtons(HWND hwnd)
{
    // Create Play button
    g_hwndPlayButton = CreateWindow(
        "BUTTON", 
        "Play File", 
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
void SetListViewItemText(HWND hListView, int row, int col, const char *text) 
{
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row;
    lvi.iSubItem = col;
    lvi.pszText = (LPSTR)text;
    
    SendMessage(hListView, LVM_SETITEM, 0, (LPARAM)&lvi);
}

// Format a time string from MIDI timing data
void FormatTimeString(char *buffer, int bufferSize, float timestamp) 
{
    int minutes = (int)(timestamp / 60.0f);
    float seconds = timestamp - (minutes * 60.0f);
    
    snprintf(buffer, bufferSize, "%d:%05.2f", minutes, seconds);
}

// Clear all items from the ListView
void ClearListView(HWND hListView) 
{
    SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);
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
    
    // Process each MIDI track (except track 0 which is metadata)
    for (int trackIndex = 1; trackIndex < g_ambFile.midi.trackCount; trackIndex++) {
        MidiTrack *track = &g_ambFile.midi.tracks[trackIndex];
        
        // Try to find the track name event to get the effect name
        char trackName[256] = {0};
        for (int i = 0; i < track->eventCount; i++) {
            if (track->events[i].type == EVENT_TRACK_NAME) {
                strcpy(trackName, track->events[i].data.trackName.name);
                break;
            }
        }
        
        if (trackName[0] == '\0') {
            continue; // Skip tracks without names
        }
        
        // Find corresponding Prgm chunk with the matching effect name
        PrgmChunk *matchingPrgm = NULL;
        for (int i = 0; i < g_ambFile.prgmChunkCount; i++) {
            if (strcmp(g_ambFile.prgmChunks[i].effectName, trackName) == 0) {
                matchingPrgm = &g_ambFile.prgmChunks[i];
                break;
            }
        }
        
        if (matchingPrgm == NULL) {
            continue; // Skip if no matching Prgm found
        }
        
        // Get the var name from the matching Prgm chunk
        const char *varName = matchingPrgm->varName;
        
        // Find corresponding Kmap chunk with the same var name
        KmapChunk *matchingKmap = NULL;
        for (int i = 0; i < g_ambFile.kmapChunkCount; i++) {
            if (strcmp(g_ambFile.kmapChunks[i].varName, varName) == 0) {
                matchingKmap = &g_ambFile.kmapChunks[i];
                break;
            }
        }
        
        if (matchingKmap == NULL || matchingKmap->itemCount == 0) {
            continue; // Skip if no matching Kmap found or no items in Kmap
        }
        
        // Add an entry for each track with note events
        float timestamp = 0.0f;
        for (int i = 0; i < track->eventCount; i++) {
            MidiEvent *event = &track->events[i];
            
            // Add delta time
            timestamp += event->deltaTime * secondsPerTick;
            
            // We're looking for the first note-on event to get the timestamp
            if (event->type == EVENT_NOTE_ON && event->data.noteOn.velocity > 0) {
                // Format time string
                char timeStr[32];
                FormatTimeString(timeStr, sizeof(timeStr), timestamp);
                
                // Format pitch information
                char pitchMinStr[32];
                char pitchMaxStr[32];
                snprintf(pitchMinStr, sizeof(pitchMinStr), "%d", matchingPrgm->minRandomPitch);
                snprintf(pitchMaxStr, sizeof(pitchMaxStr), "%d", matchingPrgm->maxRandomPitch);
                
                // Format effect parameter information
                char effectParam1Str[32];
                char effectParam2Str[32];
                snprintf(effectParam1Str, sizeof(effectParam1Str), "%d", matchingPrgm->param1);
                snprintf(effectParam2Str, sizeof(effectParam2Str), "%d", matchingPrgm->param2);
                
                // Check flags for randomization settings (LSB = pitch random, bit 1 = effect random)
                bool hasPitchRandom = (matchingPrgm->flags & 0x01) != 0;
                bool hasEffectRandom = (matchingPrgm->flags & 0x02) != 0;
                
                // For each item in the kmap (usually just one WAV file per track)
                for (int j = 0; j < matchingKmap->itemCount; j++) {
                    // Add the item to the ListView
                    int row = AddListViewItem(g_hwndListView, 0x7FFFFFFF, timeStr);
                    if (row >= 0) {
                        // Set the WAV file name
                        SetListViewItemText(g_hwndListView, row, 1, matchingKmap->items[j].wavFileName);
                        
                        // Set pitch information
                        SetListViewItemText(g_hwndListView, row, 2, hasPitchRandom ? "Yes" : "No");
                        SetListViewItemText(g_hwndListView, row, 3, pitchMinStr);
                        SetListViewItemText(g_hwndListView, row, 4, pitchMaxStr);
                        
                        // Set effect information
                        SetListViewItemText(g_hwndListView, row, 5, hasEffectRandom ? "Yes" : "No");
                        SetListViewItemText(g_hwndListView, row, 6, effectParam2Str);  // Param2 (Min)
                        SetListViewItemText(g_hwndListView, row, 7, effectParam1Str);  // Param1 (Max)
                    }
                }
                
                // Only process the first note-on event per track
                break;
            }
        }
    }
}

// Create and initialize the ListView control
void CreateAmbListView(HWND hwnd) 
{
    // Create ListView control below the playback buttons
    g_hwndListView = CreateWindow(
        WC_LISTVIEW,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOHSCROLL,
        20, 70, // x, y position (below the buttons)
        820, 460, // width, height (fill most of the window)
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
    AddListViewColumn(g_hwndListView, 0, "Time", 85);
    AddListViewColumn(g_hwndListView, 1, "WAV File", 235);
    AddListViewColumn(g_hwndListView, 2, "Pitch Random", 95);
    AddListViewColumn(g_hwndListView, 3, "Pitch Min", 70);
    AddListViewColumn(g_hwndListView, 4, "Pitch Max", 70);
    AddListViewColumn(g_hwndListView, 5, "Effect Random", 95);
    AddListViewColumn(g_hwndListView, 6, "Effect Param 2", 70);  // Min
    AddListViewColumn(g_hwndListView, 7, "Effect Param 1", 70);  // Max
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            // Find Civ3 installation when window is created
            FindCiv3Installation(hwnd);
            g_hwndMainWindow = hwnd;

            if (strlen(g_civ3MainPath) > 0) {
                    InitializePreviewPlayer(hwnd, g_civ3MainPath);
            }
            
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
                    
                case IDM_FILE_EXIT:
                    // Exit the application
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                    
                case IDM_HELP_ABOUT:
                    // Show about dialog
                    MessageBox(hwnd, 
                               "AMB Editor v0.1\nA tool for viewing and editing Civilization III AMB sound files.", 
                               "About AMB Editor", 
                               MB_OK | MB_ICONINFORMATION);
                    return 0;
                    
                case ID_PLAY_BUTTON:
                    // Handle Play button click
                    if (strlen(g_currentAmbPath) > 0) {
                        PreviewAmbFile(g_currentAmbPath);
                    } else {
                        MessageBox(hwnd, "No AMB file loaded. Please open an AMB file first.", 
                                   "Error", MB_OK | MB_ICONINFORMATION);
                    }
                    return 0;
                    
                case ID_STOP_BUTTON:
                    // Handle Stop button click
                    StopAmbPreview();
                    return 0;
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
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Create a simple menu
HMENU CreateAmbEditorMenu()
{
    HMENU hMenu, hFileMenu, hHelpMenu;
    
    hMenu = CreateMenu();
    
    // File menu
    hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open AMB File...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, "E&xit");
    
    // Help menu
    hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, "&About...");
    
    // Add menus to the main menu
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "&File");
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
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
        
        NULL,       // Parent window    
        hMenu,      // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );
    
    if (hwnd == NULL)
    {
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    
    // Run the message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}