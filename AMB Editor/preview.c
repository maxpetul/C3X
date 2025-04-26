
// This file gets #included into amb_editor.c

// Using fastcall to substitute for lack of thiscall on a C compiler, as usual. This is the dummy second param.
#define __ 0

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
    void * omitted_3[8];
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
int (__cdecl * CreateMidiDevice)(MidiDevice ** out, unsigned param_2) = NULL;

MidiDevice * midiDevice = NULL;

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
    CreateMidiDevice = (void *)GetProcAddress(soundModule, (LPCSTR)7); // Use ordinal b/c name is mangled

    InitSoundTimer(0, 0);

    CreateMidiDevice(&midiDevice, 0);
    midiDevice->vtable->Initialize(midiDevice, __, window, 0);

done:
    if (strlen(errorMsg) > 0)
            MessageBox(NULL, errorMsg, "Couldn't load sound.dll", MB_ICONERROR);
    SetCurrentDirectory(savedCWD);
}

void PreviewAmbFile(char *filePath)
{

}
