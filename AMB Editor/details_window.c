// This file gets #included into amb_editor.c

// Global variables for the AMB details window
HWND g_hwndDetailsWindow = NULL;
HWND g_hwndDetailsTextbox = NULL;
WNDPROC g_origDetailsWindowProc = NULL;

// Window procedure for the details window
LRESULT CALLBACK DetailsWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE:
            // Resize the textbox to fill the window
            if (g_hwndDetailsTextbox != NULL) {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                SetWindowPos(g_hwndDetailsTextbox, NULL,
                             0, 0,
                             rcClient.right, rcClient.bottom,
                             SWP_NOZORDER);
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            g_hwndDetailsWindow = NULL;
            g_hwndDetailsTextbox = NULL;
            return 0;
    }

    return CallWindowProc(g_origDetailsWindowProc, hwnd, uMsg, wParam, lParam);
}

// Function to show the AMB details window
void ShowAmbDetailsWindow(HWND hwndParent)
{
    // Check if we have a valid AMB file loaded
    if (g_ambFile.filePath[0] == '\0') {
        MessageBox(hwndParent, "No AMB file loaded. Please open an AMB file first.",
                  "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // If the window already exists, just bring it to the front
    if (g_hwndDetailsWindow != NULL) {
        SetForegroundWindow(g_hwndDetailsWindow);
        return;
    }

    // Create a new window for the details
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};

    // Check if the window class is already registered
    if (!GetClassInfo(hInstance, "AMBDetailsWindowClass", &wc)) {
        // Register a window class for the details window
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = "AMBDetailsWindowClass";
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

    char windowTitle[100] = {0};
    snprintf(windowTitle, sizeof(windowTitle) - 1, "%s - AMB Details", fileName);

    // Create the details window
    g_hwndDetailsWindow = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "AMBDetailsWindowClass",
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
        hwndParent,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwndDetailsWindow == NULL) {
        MessageBox(hwndParent, "Failed to create details window", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Subclass the window to handle resize events
    g_origDetailsWindowProc = (WNDPROC)SetWindowLongPtr(g_hwndDetailsWindow, GWLP_WNDPROC, (LONG_PTR)DetailsWindowProc);

    // Create a multiline read-only text box to display the details
    g_hwndDetailsTextbox = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
        0, 0, 0, 0, // Will be resized in WM_SIZE handler
        g_hwndDetailsWindow,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwndDetailsTextbox == NULL) {
        MessageBox(hwndParent, "Failed to create textbox control", "Error", MB_OK | MB_ICONERROR);
        DestroyWindow(g_hwndDetailsWindow);
        g_hwndDetailsWindow = NULL;
        return;
    }

    // Set a monospaced font for the textbox
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
    if (hFont != NULL) {
        SendMessage(g_hwndDetailsTextbox, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    // Generate the AMB file details
    char *details = (char *)malloc(50000);
    if (details) {
        DescribeAmbFile(&g_ambFile, details, 50000);

        // Set the text in the textbox
        SetWindowText(g_hwndDetailsTextbox, details);

        free(details);
    } else {
        MessageBox(hwndParent, "Failed to allocate memory for details", "Error", MB_OK | MB_ICONERROR);
        DestroyWindow(g_hwndDetailsWindow);
        g_hwndDetailsWindow = NULL;
        return;
    }

    // Force the window to resize the textbox initially
    RECT rcClient;
    GetClientRect(g_hwndDetailsWindow, &rcClient);
    SetWindowPos(g_hwndDetailsTextbox, NULL,
                 0, 0,
                 rcClient.right, rcClient.bottom,
                 SWP_NOZORDER);

    // Show the window
    ShowWindow(g_hwndDetailsWindow, SW_SHOW);
    UpdateWindow(g_hwndDetailsWindow);
}
