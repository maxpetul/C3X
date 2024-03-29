
#include <cstdio>
#include <cstring>
#include <string>

#include "windows.h"
#include "strsafe.h"

HWND CreateSimpleWindow(const char* title);
char* GetLastErrorAsString();

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
	int (__thiscall * load_file) (SoundCore *, char const *); // takes file path
	void * omitted_2[2];
	int (__thiscall * play) (SoundCore *);
	void * omitted_3[8];
	void (__thiscall * set_volume) (SoundCore *, int); // volume is >= 0 and <= 127
	void * omitted_4[5];
	int (__thiscall * m22) (SoundCore *);
	void * omitted_5;
	int (__thiscall * m24) (SoundCore *);
	void * omitted_6[2];
	int (__thiscall * set_flags) (SoundCore *, unsigned);
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
typedef int (__cdecl * CreateSound) (SoundCore ** out_sound_core, char const * file_path, int sound_core_type);
typedef int (__cdecl * CreateWaveDevice) (WaveDevice ** out, unsigned param_2);

int APIENTRY
WinMain (HINSTANCE inst, HINSTANCE prev_inst, char * cmd_line, int show_cmd)
{
	// Go to Conquests directory.
	{
		char civ_3_install_path[1000] = {0};
		DWORD buf_size = (sizeof civ_3_install_path) - 1;
		HKEY reg_key;
		if (RegOpenKeyExA (HKEY_LOCAL_MACHINE, "SOFTWARE\\Infogrames Interactive\\Civilization III", 0, KEY_READ, &reg_key) == ERROR_SUCCESS) {
			RegQueryValueExA (reg_key, "Install_Path", NULL, NULL, (LPBYTE)civ_3_install_path, &buf_size);
			RegCloseKey (reg_key);
		}
		if (civ_3_install_path[0] != '\0') {
			char conquests_path[1000] = {0};
			snprintf (conquests_path, (sizeof conquests_path) - 1, "%s%s", civ_3_install_path, "Conquests");
			SetCurrentDirectory (conquests_path);
		} else {
			MessageBoxA (NULL, "Couldn't find Civ 3 install!", NULL, MB_ICONERROR);
			return 0;
		}
	}

	HMODULE sound_module = LoadLibraryA ("sound.dll");
	if (sound_module == NULL) {
		char error_msg[1000] = {0};
		snprintf (error_msg, (sizeof error_msg) - 1, "Couldn't load sound.dll: %s", GetLastErrorAsString ());
		MessageBoxA (NULL, error_msg, NULL, MB_ICONERROR);
		return 0;
	}

	InitSoundTimer   init_sound_timer   = reinterpret_cast<InitSoundTimer>  (GetProcAddress (sound_module, "init_sound_timer"));
	CreateSound      create_sound       = reinterpret_cast<CreateSound>     (GetProcAddress (sound_module, "create_sound"));
	CreateWaveDevice create_wave_device = reinterpret_cast<CreateWaveDevice>(GetProcAddress (sound_module, (LPCSTR)5)); // Name of Dll_Wave_Device::create_device is mangled so use ordinal here

	HWND window = CreateSimpleWindow ("Sound Test");

	init_sound_timer (0, 0); // don't know what these params are for, but Civ 3 passes zero for both

	WaveDevice * wave_device;
	create_wave_device (&wave_device, 0); // second parameter is not used
	wave_device->vtable->initialize (wave_device, window, 2); // pass 2 for flags. I think that's what Civ 3 uses.

	char const * hawk_path = "..\\Sounds\\Ambience Sfx\\Hawk.wav";
	SoundCore * core;
	int result = create_sound (&core, hawk_path, SCT_WAV);
	printf ("create_sound result: %d\n", result);

	result = core->vtable->m22 (core);
	printf ("m22 returned %d (expected 0)\n", result);

	result = core->vtable->set_flags (core, 0); // Civ 3 passes 0 for flags here
	printf ("set_flags returned %d (expected 0)\n", result);

	result = core->vtable->m24 (core);
	printf ("m24 returned %d (expected 0)\n", result);

	result = core->vtable->load_file (core, hawk_path);
	printf ("load_file returned %d (expected 0)\n", result);

	// I observed this function returning 1496 when running inside Civ 3. It does not return the same thing when run here. I don't what the return
	// value means. The function is a "getter" and does no actual work.
	// result = core->vtable->m50 (core);
	// printf ("m50 returned %d (don't know what to expect)\n", result);

	core->vtable->set_volume (core, 127);
	result = core->vtable->play (core);
	printf ("play result: %d\n", result);

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

char* GetLastErrorAsString() {
    // Retrieve the last error code
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        // No error message has been recorded
        return strdup("No error");
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorMessageID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0, nullptr);

    // Copy the error message into a std::string
    std::string message(messageBuffer, size);

    // Free the buffer allocated by the system
    LocalFree(messageBuffer);

    // Return a copy of the string as a char*
    return strdup(message.c_str());
}
