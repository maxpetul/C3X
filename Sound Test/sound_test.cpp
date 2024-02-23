
#include <cstdio>
#include <cstring>

#include "windows.h"
#include "strsafe.h"

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

struct SoundCore;

struct SoundCoreVTable {
	void * omitted[7];
	int (__thiscall * play) (SoundCore *);
	void * omitted_2[69];
};

struct SoundCore {
	SoundCoreVTable * vtable;
	// many more fields omitted
};

#define SCT_WAV 1
typedef int (__cdecl * InitSoundTimer) (int param_1, int param_2);
typedef int (__cdecl * CreateSound) (SoundCore ** out_sound_core, char * file_path, int sound_core_type);
typedef int (__cdecl * CreateWaveDevice) (void ** out_wave_device, unsigned param_2);

int
main ()
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

	init_sound_timer (0, 0); // don't know what these params are for, but Civ 3 passes zero for both

	void * wave_device;
	create_wave_device (&wave_device, 0); // second parameter is not used

	char * hawk_path = "..\\Sounds\\Ambience Sfx\\Hawk.wav";
	SoundCore * core;
	int result = create_sound (&core, hawk_path, SCT_WAV);
	printf ("create_sound result: %d\n", result);

	result = core->vtable->play (core);
	printf ("play result: %d\n", result);
	Sleep (3000);

	printf ("ok");
	return 0;
}
