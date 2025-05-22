#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "amb_file.c"
#include "preview.c"
#include "wav_file.c"

// Forward declarations for ListView functions
void AddListViewColumn(HWND hListView, int index, char *title, int width);
int AddListViewItem(HWND hListView, int index, const char *text);
void SetListViewItemText(HWND hListView, int row, int col, const char *text);
void ClearListView(HWND hListView);
void PopulateAmbListView(void);
BOOL ApplyEditToAmbFile(HWND hwnd, int row, int col, const char *newText, char * outFormattedText, int formattedTextBufferSize);
BOOL IsValidInteger(const char *str);
BOOL GetWavFileDuration(const char* filePath, float* outDuration);
void MatchDurationToWav(HWND hwndListView);
void LoadAmbFileWithDialog(HWND hwnd);
void SaveAmbFileWithDialog(HWND hwnd);

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
#define LVIS_SELECTED 0x0002
#define LVS_EX_GRIDLINES 0x00000001
#define LVS_EX_FULLROWSELECT 0x00000020
#define LVS_EX_CHECKBOXES 0x00000004
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 55)
#define LVN_FIRST               (-100)
#define LVN_BEGINLABELEDIT      (LVN_FIRST-5)
#define LVN_ENDLABELEDIT        (LVN_FIRST-6)
#define LVN_COLUMNCLICK         (LVN_FIRST-8)
#define LVM_GETEDITCONTROL      (LVM_FIRST + 24)
#define LVM_EDITLABEL           (LVM_FIRST + 23)
#define LVM_GETITEMTEXT         (LVM_FIRST + 45)
#define LVM_GETSUBITEMRECT      (LVM_FIRST + 56)
#define LVM_GETCOLUMN           (LVM_FIRST + 25)
#define EM_SETSEL               0x00B1
#define LVIR_BOUNDS             0
#define LVIR_ICON               1
#define LVIR_LABEL              2
#define LVIR_SELECTBOUNDS       3

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

typedef struct tagNMLVDISPINFOA {
    NMHDR hdr;
    LVITEMA item;
} NMLVDISPINFOA, *LPNMLVDISPINFOA;

typedef struct tagNMITEMACTIVATE {
    NMHDR hdr;
    int iItem;
    int iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM lParam;
    UINT uKeyFlags;
} NMITEMACTIVATE, *LPNMITEMACTIVATE;

#define NM_FIRST        (0U-  0U)
#define NM_DBLCLK       (NM_FIRST-3)
#define EN_KILLFOCUS    0x0200

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
#define IDM_FILE_SAVE       1002
#define IDM_FILE_DETAILS    1003
#define IDM_FILE_EXIT       1004
#define IDM_EDIT_UNDO       1005
#define IDM_EDIT_REDO       1006
#define IDM_EDIT_DELETE     1007
#define IDM_EDIT_ADD        1008
#define IDM_EDIT_MATCH_WAV  1009
#define IDM_HELP_ABOUT      1010

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

// Global variables
char g_civ3MainPath[MAX_PATH_LENGTH] = {0};
char g_civ3ConquestsPath[MAX_PATH_LENGTH] = {0};
HWND g_hwndPathEdit = NULL;
HWND g_hwndMainWindow = NULL;
HWND g_hwndPlayButton = NULL;
HWND g_hwndStopButton = NULL;
HWND g_hwndListView = NULL;
HBRUSH g_hBackgroundBrush = NULL;  // Brush for window background color

// Track info for each row in the ListView
AmbRowInfo g_rowInfo[MAX_SOUND_TRACKS];
int g_rowCount = 0;  // Number of rows in the ListView

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
        if (LoadAmbFile(ambFilePath, &g_ambFile) && g_hwndMainWindow != NULL) {
            ClearFileSnapshots();

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

// Browse for a file to save
BOOL BrowseForSaveFile(HWND hwnd, char *filePath, int maxLength)
{
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    
    // Initialize the filePath with current file if any
    if (g_ambFile.filePath[0] != '\0') {
        strncpy(filePath, g_ambFile.filePath, maxLength);
    }
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "AMB Files (*.amb)\0*.amb\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = maxLength;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;
    ofn.lpstrDefExt = "amb";
    ofn.lpstrTitle = "Save AMB File";
    
    return GetSaveFileNameA(&ofn);
}

// Save AMB file with a dialog
void SaveAmbFileWithDialog(HWND hwnd)
{
    if (g_ambFile.filePath[0] == '\0') {
        MessageBox(hwnd, "No AMB file loaded. Please load a file first.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    char filePath[MAX_PATH_LENGTH] = {0};
    
    if (BrowseForSaveFile(hwnd, filePath, MAX_PATH_LENGTH)) {
        // Try to save the AMB file
        if (SaveAmbFile(&g_ambFile, filePath)) {
            // Extract the filename part from the path
            char *fileName = strrchr(filePath, '\\');
            if (fileName) {
                fileName++; // Skip past the backslash
            } else {
                fileName = (char*)filePath; // No backslash found, use the whole path
            }
            
            char windowTitle[MAX_PATH_LENGTH + 20];
            snprintf(windowTitle, sizeof windowTitle, "%s - AMB Editor", fileName);
            windowTitle[(sizeof windowTitle) - 1] = '\0';
            SetWindowText(g_hwndMainWindow, windowTitle);
            
            // Show success message
            MessageBox(hwnd, "AMB file saved successfully.", "Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(hwnd, "Failed to save AMB file.", "Error", MB_OK | MB_ICONERROR);
        }
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

// Clear all items from the ListView
void ClearListView(HWND hListView) 
{
    SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);
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
    
    // Process each MIDI sound track
    int soundTrackCount = g_ambFile.midi.trackCount - 1; // Minus one to exclude the info track
    for (int trackIndex = 0; trackIndex < soundTrackCount; trackIndex++) {
        SoundTrack * track = &g_ambFile.midi.soundTracks[trackIndex];
        
        // Find corresponding Prgm chunk with the matching effect name
        PrgmChunk *matchingPrgm = NULL;
        int prgmIndex = -1;
        for (int i = 0; i < g_ambFile.prgmChunkCount; i++) {
            if (_stricmp(g_ambFile.prgmChunks[i].effectName, track->trackName.name) == 0) { // stricmp ignores case
                matchingPrgm = &g_ambFile.prgmChunks[i];
                prgmIndex = i;
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

        // Format time string
        char timeStr[32] = {0};
        snprintf(timeStr, (sizeof timeStr) - 1, "%04.3f", timestamp);
        
        // Format duration string
        char durationStr[32] = {0};
        snprintf(durationStr, (sizeof durationStr) - 1, "%04.3f", duration);
                
        // Format speed information
        char speedMaxStr[32];
        char speedMinStr[32];
        snprintf(speedMaxStr, sizeof(speedMaxStr), "%d", matchingPrgm->maxRandomSpeed);
        snprintf(speedMinStr, sizeof(speedMinStr), "%d", matchingPrgm->minRandomSpeed);
                
        // Format volume parameter information
        char volumeMaxStr[32];
        char volumeMinStr[32];
        snprintf(volumeMaxStr, sizeof(volumeMaxStr), "%d", matchingPrgm->maxRandomVolume);
        snprintf(volumeMinStr, sizeof(volumeMinStr), "%d", matchingPrgm->minRandomVolume);
                
        // Check flags for randomization settings (LSB = speed random, bit 1 = volume random)
        bool hasSpeedRandom  = (matchingPrgm->flags & 0x01) != 0;
        bool hasVolumeRandom = (matchingPrgm->flags & 0x02) != 0;

        // For each item in the kmap (usually just one WAV file per track)
        for (int j = 0; j < matchingKmap->itemCount; j++) {
            // Add the item to the ListView
            int row = AddListViewItem(g_hwndListView, 0x7FFFFFFF, timeStr);
            if (row >= 0) {
                // Store the track info for this row
                if (g_rowCount < MAX_SOUND_TRACKS) {
                    g_rowInfo[g_rowCount].trackIndex = trackIndex;
                    g_rowInfo[g_rowCount].kmapIndex = kmapIndex;
                    g_rowInfo[g_rowCount].prgmIndex = prgmIndex;
                    g_rowInfo[g_rowCount].kmapItemIndex = j;
                    g_rowCount++;
                }
                        
                // Set the duration
                SetListViewItemText(g_hwndListView, row, COL_DURATION, durationStr);
                        
                // Set the WAV file name
                SetListViewItemText(g_hwndListView, row, COL_WAV_FILE, matchingKmap->items[j].wavFileName);
                        
                // Set speed information
                SetListViewItemText(g_hwndListView, row, COL_SPEED_RANDOM, hasSpeedRandom ? "Yes" : "No");
                SetListViewItemText(g_hwndListView, row, COL_SPEED_MIN, speedMinStr);
                SetListViewItemText(g_hwndListView, row, COL_SPEED_MAX, speedMaxStr);
                        
                // Set volume information
                SetListViewItemText(g_hwndListView, row, COL_VOLUME_RANDOM, hasVolumeRandom ? "Yes" : "No");
                SetListViewItemText(g_hwndListView, row, COL_VOLUME_MIN, volumeMinStr);
                SetListViewItemText(g_hwndListView, row, COL_VOLUME_MAX, volumeMaxStr);
            }
        }
    }
}

// Handle the beginning of a ListView label edit
BOOL HandleBeginLabelEdit(HWND hwnd, NMLVDISPINFOA *pInfo)
{
    // You can reject edits for specific columns or rows if needed
    // For now, allow editing all cells
    return TRUE; // Return TRUE to allow the edit, FALSE to prevent it
}

// Apply an edit to the AMB file data structure. Returns TRUE if the edit was accepted or FALSE if it was invalid. If the edit was accepted, the new
// text is written to outFormattedText in a way that is consistent with the usual format of the ListView display (e.g. y/n converts to Yes/No for
// boolean fields.) outFormattedText must not be NULL and formattedTextBufferSize must be at least 1.
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
    
    // For debugging: Show which cell was edited with track info
    char debugMsg[512];
    sprintf(debugMsg, "Edited Row %d, Column %d: New Value = %s\n\nTrack Index: %d\nKmap Index: %d\nPrgm Index: %d\nKmap Item Index: %d", 
            row, col, newText, trackIndex, kmapIndex, prgmIndex, kmapItemIndex);
    MessageBox(hwnd, debugMsg, "Cell Edited", MB_OK | MB_ICONINFORMATION);
    
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
		    g_ambFile.midi.soundTracks[trackIndex].deltaTimeNoteOn = newTime * ticksPerSecond;

                    snprintf(outFormattedText, formattedTextBufferSize, "%04.3f", newTime);

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
                    g_ambFile.midi.soundTracks[trackIndex].deltaTimeNoteOff = newDuration * ticksPerSecond;

                    snprintf(outFormattedText, formattedTextBufferSize, "%04.3f", newDuration);

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
        // Shift all Kmap chunks after this one forward
        for (int i = kmapIndex; i < g_ambFile.kmapChunkCount - 1; i++) {
            memcpy(&g_ambFile.kmapChunks[i], &g_ambFile.kmapChunks[i + 1], sizeof(KmapChunk));
        }
        g_ambFile.kmapChunkCount--;
    }

    // Next, check if the Prgm chunk is referenced by other MIDI tracks
    PrgmChunk *prgm = &g_ambFile.prgmChunks[prgmIndex];
    const char *effectName = prgm->effectName;
    int prgmReferenceCount = 0;

    for (int i = 0; i < g_ambFile.midi.trackCount - 1; i++) { // Skip info track
        if (_stricmp(g_ambFile.midi.soundTracks[i].trackName.name, effectName) == 0) {
            prgmReferenceCount++;
        }
    }

    // Delete the Prgm chunk if it's only referenced by the track we're deleting
    if (prgmReferenceCount <= 1) {
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

    // Delete the MIDI track
    for (int i = trackIndex; i < g_ambFile.midi.trackCount - 2; i++) { // -2 because we skip info track (0) and want to leave room for the last valid track
        memcpy(&g_ambFile.midi.soundTracks[i], &g_ambFile.midi.soundTracks[i + 1], sizeof(SoundTrack));
    }
    g_ambFile.midi.trackCount--;

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
        SetListViewItemText(g_hwndListView, row, col, formattedText);
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
                            SetListViewItemText(g_hwndListView, g_editRow, g_editCol, formattedBuffer);
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

// Function to delete the currently selected row
void DeleteSelectedRow(HWND hwndListView)
{
    // Check if a row is selected
    int rowCount = SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
    
    for (int i = 0; i < rowCount; i++) {
        UINT state = SendMessage(hwndListView, LVM_GETITEMSTATE, i, LVIS_SELECTED);
        if (state & LVIS_SELECTED) {
            // Found a selected row, delete it
            DeleteRow(i);
            return;
        }
    }
    
    // If we get here, no row was selected
    MessageBox(NULL, "Please select a row to delete", "Information", MB_OK | MB_ICONINFORMATION);
}

// Function to match duration to WAV file for the selected row
void MatchDurationToWav(HWND hwndListView)
{
    // Check if a row is selected
    int rowCount = SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
    int selectedRow = -1;
    
    for (int i = 0; i < rowCount; i++) {
        UINT state = SendMessage(hwndListView, LVM_GETITEMSTATE, i, LVIS_SELECTED);
        if (state & LVIS_SELECTED) {
            selectedRow = i;
            break;
        }
    }
    
    if (selectedRow == -1) {
        MessageBox(NULL, "Please select a row to match duration", "Information", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
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
    char wavFilePath[MAX_PATH_LENGTH];
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
        SetListViewItemText(hwndListView, selectedRow, COL_DURATION, formattedText);
        MessageBox(NULL, "Duration matched to WAV file successfully", "Success", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBox(NULL, "Failed to update duration", "Error", MB_OK | MB_ICONERROR);
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

// Create and initialize the ListView control
void CreateAmbListView(HWND hwnd) 
{
    // Create ListView control below the playback buttons
    g_hwndListView = CreateWindow(
        WC_LISTVIEW,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOHSCROLL | LVS_EDITLABELS,
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
    AddListViewColumn(g_hwndListView, COL_TIME, "Time (sec.)", 85);
    AddListViewColumn(g_hwndListView, COL_DURATION, "Duration (sec.)", 95);
    AddListViewColumn(g_hwndListView, COL_WAV_FILE, "WAV File", 235);
    AddListViewColumn(g_hwndListView, COL_SPEED_RANDOM, "Speed Random", 95);
    AddListViewColumn(g_hwndListView, COL_SPEED_MIN, "Speed Min", 70);
    AddListViewColumn(g_hwndListView, COL_SPEED_MAX, "Speed Max", 70);
    AddListViewColumn(g_hwndListView, COL_VOLUME_RANDOM, "Volume Random", 95);
    AddListViewColumn(g_hwndListView, COL_VOLUME_MIN, "Volume Min", 70);
    AddListViewColumn(g_hwndListView, COL_VOLUME_MAX, "Volume Max", 70);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            // Handle keyboard shortcuts
            if (GetKeyState(VK_CONTROL) < 0) {  // Check if Ctrl key is pressed
                switch (wParam) {
                    case 'Z':  // Ctrl+Z for Undo
                        Undo();
                        return 0;
                        
                    case 'Y':  // Ctrl+Y for Redo
                        Redo();
                        return 0;
                }
            } else {
                switch (wParam) {
                    case VK_DELETE:  // Delete key to delete selected row
                        DeleteSelectedRow(g_hwndListView);
                        return 0;
                        
                    case VK_INSERT:  // Insert key to add a new row
                        AddRow();
                        return 0;
                }
            }
            break;
            
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
                    
                case IDM_FILE_SAVE:
                    // Show save file dialog to save an AMB file
                    SaveAmbFileWithDialog(hwnd);
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
                    // Delete the selected row
                    DeleteSelectedRow(g_hwndListView);
                    return 0;
                    
                case IDM_EDIT_MATCH_WAV:
                    // Match duration to WAV file
                    MatchDurationToWav(g_hwndListView);
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
                    if (g_ambFile.filePath[0] != '\0') {
                        PreviewAmbFile(&g_ambFile);
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
                                SetListViewItemText(g_hwndListView, g_editRow, g_editCol, formattedBuffer);
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
                                            SetListViewItemText(g_hwndListView, nia->iItem, nia->iSubItem, formattedText);
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
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open AMB File...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE, "&Save AMB File...");
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
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
        
        NULL,       // Parent window    
        hMenu,      // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );
    
    if (hwnd == NULL) {
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    
    // Run the message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
