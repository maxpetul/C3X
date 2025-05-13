#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "amb_file.c"
#include "preview.c"

// Forward declarations for ListView functions
void AddListViewColumn(HWND hListView, int index, char *title, int width);
int AddListViewItem(HWND hListView, int index, const char *text);
void SetListViewItemText(HWND hListView, int row, int col, const char *text);
void FormatTimeString(char *buffer, int bufferSize, float timestamp);
void ClearListView(HWND hListView);
void PopulateAmbListView(void);
BOOL ApplyEditToAmbFile(HWND hwnd, int row, int col, const char *newText);
BOOL IsValidInteger(const char *str);
BOOL IsValidBoolean(const char *str, BOOL *value);
void LoadAmbFileWithDialog(HWND hwnd);
void SaveAmbFileWithDialog(HWND hwnd);

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
#define IDM_FILE_DESCRIBE   1003
#define IDM_FILE_EXIT       1004
#define IDM_HELP_ABOUT      1005

// Control IDs 
#define ID_PATH_EDIT        102
#define ID_PLAY_BUTTON      103
#define ID_STOP_BUTTON      104
#define ID_AMB_LISTVIEW     105

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
AmbRowInfo g_rowInfo[MAX_TRACKS];
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
    
    if (BrowseForSaveFile(hwnd, filePath, MAX_PATH_LENGTH))
    {
        // Try to save the AMB file
        if (SaveAmbFile(filePath))
        {
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
        }
        else
        {
            MessageBox(hwnd, "Failed to save AMB file.", "Error", MB_OK | MB_ICONERROR);
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

// Function to check if a string is a valid boolean value
BOOL IsValidBoolean(const char *str, BOOL *value)
{
    // Check for valid "true" values
    if (strcmp(str, "Yes") == 0 ||
        strcmp(str, "yes") == 0 ||
        strcmp(str, "Y") == 0 ||
        strcmp(str, "y") == 0 ||
        strcmp(str, "True") == 0 ||
        strcmp(str, "true") == 0 ||
        strcmp(str, "1") == 0) {
        if (value) *value = TRUE;
        return TRUE;
    }

    // Check for valid "false" values
    if (strcmp(str, "No") == 0 ||
        strcmp(str, "no") == 0 ||
        strcmp(str, "N") == 0 ||
        strcmp(str, "n") == 0 ||
        strcmp(str, "False") == 0 ||
        strcmp(str, "false") == 0 ||
        strcmp(str, "0") == 0) {
        if (value) *value = FALSE;
        return TRUE;
    }

    // Invalid boolean value
    return FALSE;
}

// Global variables for the AMB description window
HWND g_hwndDescWindow = NULL;
HWND g_hwndDescTextbox = NULL;
WNDPROC g_origDescWindowProc = NULL;

// Window procedure for the description window
LRESULT CALLBACK DescWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE:
            // Resize the textbox to fill the window
            if (g_hwndDescTextbox != NULL) {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                SetWindowPos(g_hwndDescTextbox, NULL,
                             0, 0,
                             rcClient.right, rcClient.bottom,
                             SWP_NOZORDER);
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            g_hwndDescWindow = NULL;
            g_hwndDescTextbox = NULL;
            return 0;
    }

    return CallWindowProc(g_origDescWindowProc, hwnd, uMsg, wParam, lParam);
}

// Function to show the AMB description window
void ShowAmbDescriptionWindow(HWND hwndParent)
{
    // Check if we have a valid AMB file loaded
    if (g_ambFile.filePath[0] == '\0') {
        MessageBox(hwndParent, "No AMB file loaded. Please open an AMB file first.",
                  "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // If the window already exists, just bring it to the front
    if (g_hwndDescWindow != NULL) {
        SetForegroundWindow(g_hwndDescWindow);
        return;
    }

    // Create a new window for the description
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};

    // Check if the window class is already registered
    if (!GetClassInfo(hInstance, "AMBDescWindowClass", &wc)) {
        // Register a window class for the description window
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = "AMBDescWindowClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

        if (!RegisterClass(&wc)) {
            MessageBox(hwndParent, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
            return;
        }
    }

    // Extract filename from the full path for the window title
    const char *fileName = strrchr(g_ambFile.filePath, '\\');
    if (fileName) {
        fileName++; // Skip past the backslash
    } else {
        fileName = g_ambFile.filePath; // No backslash found, use the whole path
    }

    char windowTitle[MAX_PATH_LENGTH + 30];
    snprintf(windowTitle, sizeof(windowTitle), "%s - AMB Description", fileName);

    // Create the description window
    g_hwndDescWindow = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "AMBDescWindowClass",
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
        hwndParent,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwndDescWindow == NULL) {
        MessageBox(hwndParent, "Failed to create description window", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Subclass the window to handle resize events
    g_origDescWindowProc = (WNDPROC)SetWindowLongPtr(g_hwndDescWindow, GWLP_WNDPROC, (LONG_PTR)DescWindowProc);

    // Create a multiline read-only text box to display the description
    g_hwndDescTextbox = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
        0, 0, 0, 0, // Will be resized in WM_SIZE handler
        g_hwndDescWindow,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwndDescTextbox == NULL) {
        MessageBox(hwndParent, "Failed to create textbox control", "Error", MB_OK | MB_ICONERROR);
        DestroyWindow(g_hwndDescWindow);
        g_hwndDescWindow = NULL;
        return;
    }

    // Set a monospaced font for the textbox
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
    if (hFont != NULL) {
        SendMessage(g_hwndDescTextbox, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    // Generate the AMB file description
    char *description = (char *)malloc(50000);
    if (description) {
        DescribeAmbFile(description, 50000);

        // Set the text in the textbox
        SetWindowText(g_hwndDescTextbox, description);

        free(description);
    } else {
        MessageBox(hwndParent, "Failed to allocate memory for description", "Error", MB_OK | MB_ICONERROR);
        DestroyWindow(g_hwndDescWindow);
        g_hwndDescWindow = NULL;
        return;
    }

    // Force the window to resize the textbox initially
    RECT rcClient;
    GetClientRect(g_hwndDescWindow, &rcClient);
    SetWindowPos(g_hwndDescTextbox, NULL,
                 0, 0,
                 rcClient.right, rcClient.bottom,
                 SWP_NOZORDER);

    // Show the window
    ShowWindow(g_hwndDescWindow, SW_SHOW);
    UpdateWindow(g_hwndDescWindow);
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
        int prgmIndex = -1;
        for (int i = 0; i < g_ambFile.prgmChunkCount; i++) {
            if (strcmp(g_ambFile.prgmChunks[i].effectName, trackName) == 0) {
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
                        if (g_rowCount < MAX_TRACKS) {
                            g_rowInfo[g_rowCount].trackIndex = trackIndex;
                            g_rowInfo[g_rowCount].kmapIndex = kmapIndex;
                            g_rowInfo[g_rowCount].prgmIndex = prgmIndex;
                            g_rowInfo[g_rowCount].kmapItemIndex = j;
                            g_rowCount++;
                        }
                        
                        // Set the WAV file name
                        SetListViewItemText(g_hwndListView, row, 1, matchingKmap->items[j].wavFileName);
                        
                        // Set speed information
                        SetListViewItemText(g_hwndListView, row, 2, hasSpeedRandom ? "Yes" : "No");
                        SetListViewItemText(g_hwndListView, row, 3, speedMinStr);
                        SetListViewItemText(g_hwndListView, row, 4, speedMaxStr);
                        
                        // Set volume information
                        SetListViewItemText(g_hwndListView, row, 5, hasVolumeRandom ? "Yes" : "No");
                        SetListViewItemText(g_hwndListView, row, 6, volumeMinStr);
                        SetListViewItemText(g_hwndListView, row, 7, volumeMaxStr);
                    }
                }
                
                // Only process the first note-on event per track
                break;
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

// Apply an edit to the AMB file data structure
BOOL ApplyEditToAmbFile(HWND hwnd, int row, int col, const char *newText)
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
    switch (col) {
        case 0: // Time column - can't edit this directly as it's MIDI timing data
            break;
            
        case 1: // WAV filename
            // Update the WAV filename in the Kmap item
            strncpy(kmap->items[kmapItemIndex].wavFileName, newText, sizeof(kmap->items[kmapItemIndex].wavFileName) - 1);
            kmap->items[kmapItemIndex].wavFileName[sizeof(kmap->items[kmapItemIndex].wavFileName) - 1] = '\0';
            break;
            
        case 2: // Speed Random flag
            {
                BOOL boolValue;
                if (IsValidBoolean(newText, &boolValue)) {
                    // Valid boolean value - update the flag
                    if (boolValue) {
                        prgm->flags |= 0x01;  // Set bit 0
                    } else {
                        prgm->flags &= ~0x01;  // Clear bit 0
                    }
                    
                    // Update the displayed text for consistency
                    SetListViewItemText(g_hwndListView, row, col, (prgm->flags & 0x01) ? "Yes" : "No");
                } else {
                    // Invalid boolean - play error sound and reject edit
                    MessageBeep(MB_ICONERROR);
                    
                    // Restore the original value in the ListView
                    SetListViewItemText(g_hwndListView, row, col, (prgm->flags & 0x01) ? "Yes" : "No");
                    
                    return FALSE;
                }
            }
            break;
            
        case 3: // Speed Min
            if (IsValidInteger(newText)) {
                prgm->minRandomSpeed = atoi(newText);
            } else {
                // Invalid integer - play error sound and reject edit
                MessageBeep(MB_ICONERROR);
                
                // Get the current value and restore it in the ListView
                char buffer[16];
                sprintf(buffer, "%d", prgm->minRandomSpeed);
                SetListViewItemText(g_hwndListView, row, col, buffer);
                
                return FALSE;
            }
            break;
            
        case 4: // Speed Max
            if (IsValidInteger(newText)) {
                prgm->maxRandomSpeed = atoi(newText);
            } else {
                // Invalid integer - play error sound and reject edit
                MessageBeep(MB_ICONERROR);
                
                // Get the current value and restore it in the ListView
                char buffer[16];
                sprintf(buffer, "%d", prgm->maxRandomSpeed);
                SetListViewItemText(g_hwndListView, row, col, buffer);
                
                return FALSE;
            }
            break;
            
        case 5: // Volume Random flag
            {
                BOOL boolValue;
                if (IsValidBoolean(newText, &boolValue)) {
                    // Valid boolean value - update the flag
                    if (boolValue) {
                        prgm->flags |= 0x02;  // Set bit 1
                    } else {
                        prgm->flags &= ~0x02;  // Clear bit 1
                    }
                    
                    // Update the displayed text for consistency
                    SetListViewItemText(g_hwndListView, row, col, (prgm->flags & 0x02) ? "Yes" : "No");
                } else {
                    // Invalid boolean - play error sound and reject edit
                    MessageBeep(MB_ICONERROR);
                    
                    // Restore the original value in the ListView
                    SetListViewItemText(g_hwndListView, row, col, (prgm->flags & 0x02) ? "Yes" : "No");
                    
                    return FALSE;
                }
            }
            break;
            
        case 6: // Volume Min
            if (IsValidInteger(newText)) {
                prgm->minRandomVolume = atoi(newText);
            } else {
                // Invalid integer - play error sound and reject edit
                MessageBeep(MB_ICONERROR);
                
                // Get the current value and restore it in the ListView
                char buffer[16];
                sprintf(buffer, "%d", prgm->minRandomVolume);
                SetListViewItemText(g_hwndListView, row, col, buffer);
                
                return FALSE;
            }
            break;
            
        case 7: // Volume Max
            if (IsValidInteger(newText)) {
                prgm->maxRandomVolume = atoi(newText);
            } else {
                // Invalid integer - play error sound and reject edit
                MessageBeep(MB_ICONERROR);
                
                // Get the current value and restore it in the ListView
                char buffer[16];
                sprintf(buffer, "%d", prgm->maxRandomVolume);
                SetListViewItemText(g_hwndListView, row, col, buffer);
                
                return FALSE;
            }
            break;
    }
    
    return TRUE;
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
    return ApplyEditToAmbFile(hwnd, row, col, newText);
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
                        
                        // Update the ListView item with the new text
                        SetListViewItemText(g_hwndListView, g_editRow, g_editCol, buffer);
                        
                        // Apply the edit to the AMB file data structure
                        ApplyEditToAmbFile(GetParent(GetParent(hwnd)), g_editRow, g_editCol, buffer);
                        
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

// Function to programmatically start editing a ListView subitem
void EditListViewSubItem(HWND hwndListView, int row, int col)
{
    // First select the item
    LVITEMA lvi = {0};
    lvi.stateMask = LVIS_SELECTED;
    lvi.state = LVIS_SELECTED;
    SendMessage(hwndListView, LVM_SETITEMSTATE, row, (LPARAM)&lvi);
    
    // For the first column (col=0), we can use the built-in edit control
    if (col == 0) {
        SendMessage(hwndListView, LVM_EDITLABEL, row, 0);
        return;
    }
    
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
    AddListViewColumn(g_hwndListView, 0, "Time", 85);
    AddListViewColumn(g_hwndListView, 1, "WAV File", 235);
    AddListViewColumn(g_hwndListView, 2, "Speed Random", 95);
    AddListViewColumn(g_hwndListView, 3, "Speed Min", 70);
    AddListViewColumn(g_hwndListView, 4, "Speed Max", 70);
    AddListViewColumn(g_hwndListView, 5, "Volume Random", 95);
    AddListViewColumn(g_hwndListView, 6, "Volume Min", 70);  // Min
    AddListViewColumn(g_hwndListView, 7, "Volume Max", 70);  // Max
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
                    
                case IDM_FILE_SAVE:
                    // Show save file dialog to save an AMB file
                    SaveAmbFileWithDialog(hwnd);
                    return 0;

                case IDM_FILE_DESCRIBE:
                    // Show window with AMB file description
                    ShowAmbDescriptionWindow(hwnd);
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
                            
                            // Update the ListView item with the new text
                            SetListViewItemText(g_hwndListView, g_editRow, g_editCol, buffer);
                            
                            // Apply the edit to the AMB file data structure
                            ApplyEditToAmbFile(hwnd, g_editRow, g_editCol, buffer);
                            
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
                                    // Start editing the subitem that was clicked
                                    EditListViewSubItem(g_hwndListView, nia->iItem, nia->iSubItem);
                                    
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
    HMENU hMenu, hFileMenu, hHelpMenu;
    
    hMenu = CreateMenu();
    
    // File menu
    hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open AMB File...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE, "&Save AMB File...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_DESCRIBE, "&View AMB Description");
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
