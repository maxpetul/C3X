
#include <cstdio>
#include <cstring>

#include "windows.h"
#include "strsafe.h"

HWND CreateSimpleWindow(const char* title);

// Copy-pasted from https://learn.microsoft.com/en-us/windows/win32/Debug/retrieving-the-last-error-code
void ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}

struct WaveDevice;

struct WaveDeviceVTable {
	void * omitted[4];
	int (__thiscall * initialize) (WaveDevice *, HWND, unsigned);
	void * omitted_2[38];
};

struct WaveDevice {
	WaveDeviceVTable * vtable;
	// other fields omitted
};

struct SoundCore;

struct SoundCoreVTable {
	void * omitted[4];
	int (__thiscall * m04) (SoundCore *, char *); // takes file path
	void * omitted_2[2];
	int (__thiscall * play) (SoundCore *);
	void * omitted_3[8];
	void (__thiscall * m16) (SoundCore *, int);
	void * omitted_4[5];
	int (__thiscall * m22) (SoundCore *);
	void * omitted_5;
	int (__thiscall * m24) (SoundCore *);
	void * omitted_6[2];
	int (__thiscall * m27) (SoundCore *, unsigned);
	void * omitted_7[22];
	int (__thiscall * m50) (SoundCore *);
	void * omitted_8[26];
};

struct SoundCore {
	SoundCoreVTable * vtable;
	// many more fields omitted
};

#define SCT_WAV 1
typedef int (__cdecl * InitSoundTimer) (int param_1, int param_2);
typedef int (__cdecl * CreateSound) (SoundCore ** out_sound_core, char * file_path, int sound_core_type);
typedef int (__cdecl * CreateWaveDevice) (WaveDevice ** out, unsigned param_2);

int APIENTRY
WinMain (HINSTANCE inst, HINSTANCE prev_inst, char * cmd_line, int show_cmd)
{
	// Go to Conquests directory
	// TODO: Load directory location from registry
	SetCurrentDirectory ("C:\\GOG Games\\Civilization III Complete\\Conquests");

	// char * sound_module_path = get_civ_3_path ("Conquests\\sound.dll");
	HMODULE sound_module = LoadLibraryA ("sound.dll");
	if (sound_module == NULL) {
		ErrorExit ("creating sound module");
	}

	InitSoundTimer init_sound_timer = reinterpret_cast<InitSoundTimer>(GetProcAddress (sound_module, "init_sound_timer"));
	if (init_sound_timer == NULL) {
		printf ("init_sound_timer is NULL");
		return 1;
	}

	CreateSound create_sound = reinterpret_cast<CreateSound>(GetProcAddress (sound_module, "create_sound"));
	if (create_sound == NULL) {
		printf ("create_sound is NULL");
		return 1;
	}

	// Name of Dll_Wave_Device::create_device is mangled so use ordinal here
	CreateWaveDevice create_wave_device = reinterpret_cast<CreateWaveDevice>(GetProcAddress (sound_module, (LPCSTR)5));
	if (create_wave_device == NULL) {
		printf ("create_wave_device is NULL");
		return 1;
	}

	HWND window = CreateSimpleWindow ("Sound Test");

	init_sound_timer (0, 0); // don't know what these params are for, but Civ 3 passes zero for both

	WaveDevice * wave_device;
	create_wave_device (&wave_device, 0); // second parameter is not used
	wave_device->vtable->initialize (wave_device, window, 2); // pass "2" for flags. I think that's what Civ 3 uses.

	char * hawk_path = "..\\Sounds\\Ambience Sfx\\Hawk.wav";
	SoundCore * core;
	int result = create_sound (&core, hawk_path, SCT_WAV);
	printf ("create_sound result: %d\n", result);

	result = core->vtable->m22 (core);
	printf ("m22 returned %d (expected 0)\n", result);

	result = core->vtable->m27 (core, 0); // 0 corresponds to some flags, Civ 3 passes 0
	printf ("m27 returned %d (expected 0)\n", result);

	result = core->vtable->m24 (core);
	printf ("m24 returned %d (expected 0)\n", result);

	result = core->vtable->m04 (core, hawk_path);
	printf ("m04 returned %d (expected 0)\n", result);

	result = core->vtable->m50 (core);
	printf ("m50 returned %d (don't know what to expect)\n", result); // Civ 3 returns 1496 here

	core->vtable->m16 (core, 127); // Pretty sure this sets the volume
	result = core->vtable->play (core);
	printf ("play result: %d\n", result);

	printf ("ok\n");

	// Run the window message loop
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Need to call TerminateProcess to close the program otherwise sound.dll will keep it alive in the background.
	TerminateProcess (GetCurrentProcess (), 0);
	return 0; // Unreachable
}



//
// Helper functions (all written by ChatGPT):
//

// Function prototype for the Window Procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Function to create and show a simple window
HWND CreateSimpleWindow(const char* title) {
    // Register the window class
    const char CLASS_NAME[] = "Simple Window Class";
    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        title,                          // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 200, 100,

        NULL,       // Parent window
        NULL,       // Menu
        wc.hInstance,  // Instance handle
        NULL        // Additional application data
    );

    ShowWindow(hwnd, SW_SHOW);

    return hwnd;
}

// Window procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
