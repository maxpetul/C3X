#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH_LENGTH 1024
#define ID_BROWSE_BUTTON 101
#define ID_PATH_EDIT 102
#define MAX_CHUNKS 100
#define MAX_TRACKS 20
#define MAX_EVENTS 1000
#define MAX_ITEMS 10

// Global variables
char g_civ3MainPath[MAX_PATH_LENGTH] = {0};
char g_civ3ConquestsPath[MAX_PATH_LENGTH] = {0};
HWND g_hwndPathEdit = NULL;
HWND g_hwndMainWindow = NULL;
char g_currentAmbPath[MAX_PATH_LENGTH] = {0};

// AMB file structures
typedef struct {
    int size;
    int number;
    int flags;
    int maxRandomPitch;
    int minRandomPitch;
    int param1;
    int param2;
    char effectName[256];
    char varName[256];
} PrgmChunk;

typedef struct {
    unsigned char data[12];
    char wavFileName[256];
} KmapItem;

typedef struct {
    int size;
    int flags;
    int param1;
    int param2;
    char varName[256];
    int itemCount;
    int itemSize;
    KmapItem items[MAX_ITEMS];
} KmapChunk;

typedef struct {
    int size;
    int dataSize;
    unsigned char data[12];
    unsigned char extraData[512];
    int extraDataSize;
} GlblChunk;

typedef enum {
    EVENT_TRACK_NAME,
    EVENT_SMPTE_OFFSET,
    EVENT_TIME_SIGNATURE,
    EVENT_SET_TEMPO,
    EVENT_END_OF_TRACK,
    EVENT_CONTROL_CHANGE,
    EVENT_PROGRAM_CHANGE,
    EVENT_NOTE_OFF,
    EVENT_NOTE_ON,
    EVENT_UNKNOWN
} MidiEventType;

typedef struct {
    MidiEventType type;
    int deltaTime;
    
    // Data for different event types
    union {
        struct { char name[256]; } trackName;
        struct { int hr, mn, se, fr, ff; } smpteOffset;
        struct { int nn, dd, cc, bb; } timeSignature;
        struct { int microsecondsPerQuarterNote; } setTempo;
        struct { int channelNumber, controllerNumber, value; } controlChange;
        struct { int channelNumber, programNumber; } programChange;
        struct { int channelNumber, key, velocity; } noteOff;
        struct { int channelNumber, key, velocity; } noteOn;
        struct { unsigned char data[16]; int dataLength; } unknown;
    } data;
} MidiEvent;

typedef struct {
    int size;
    MidiEvent events[MAX_EVENTS];
    int eventCount;
} MidiTrack;

typedef struct {
    short format;
    short trackCount;
    short ticksPerQuarterNote;
    float secondsPerQuarterNote;
    MidiTrack tracks[MAX_TRACKS];
} MidiData;

typedef struct {
    char filePath[MAX_PATH_LENGTH];
    
    // Chunks
    PrgmChunk prgmChunks[MAX_CHUNKS];
    int prgmChunkCount;
    
    KmapChunk kmapChunks[MAX_CHUNKS];
    int kmapChunkCount;
    
    GlblChunk glblChunk;
    bool hasGlblChunk;
    
    MidiData midi;
} AmbFile;

AmbFile g_ambFile;

// Helper functions for file parsing
unsigned int ReadAmbInt(FILE *file, bool isUnsigned) {
    unsigned char buffer[4];
    fread(buffer, 1, 4, file);
    
    // Little endian
    unsigned int result = 
        (buffer[0]) |
        (buffer[1] << 8) |
        (buffer[2] << 16) |
        (buffer[3] << 24);
    
    if (!isUnsigned && (result & 0x80000000)) {
        // Convert to signed if needed
        return (int)result;
    }
    
    return result;
}

unsigned int ReadMidiInt(FILE *file, bool isUnsigned) {
    unsigned char buffer[4];
    fread(buffer, 1, 4, file);
    
    // Big endian
    unsigned int result = 
        (buffer[0] << 24) |
        (buffer[1] << 16) |
        (buffer[2] << 8) |
        (buffer[3]);
    
    if (!isUnsigned && (result & 0x80000000)) {
        // Convert to signed if needed
        return (int)result;
    }
    
    return result;
}

unsigned short ReadMidiShort(FILE *file, bool isUnsigned) {
    unsigned char buffer[2];
    fread(buffer, 1, 2, file);
    
    // Big endian
    unsigned short result = (buffer[0] << 8) | buffer[1];
    
    if (!isUnsigned && (result & 0x8000)) {
        // Convert to signed if needed
        return (short)result;
    }
    
    return result;
}

unsigned int ReadMidiVarInt(FILE *file) {
    unsigned int result = 0;
    unsigned char byte;
    
    do {
        fread(&byte, 1, 1, file);
        result = (result << 7) | (byte & 0x7F);
    } while (byte & 0x80);
    
    return result;
}

void ReadString(FILE *file, char *buffer, int maxLength) {
    int i = 0;
    char byte;
    
    while (i < maxLength - 1) {
        fread(&byte, 1, 1, file);
        if (byte == 0) {
            break;
        }
        buffer[i++] = byte;
    }
    
    buffer[i] = '\0';
}

// Parse AMB file chunks
bool ParsePrgmChunk(FILE *file, PrgmChunk *chunk) {
    chunk->size = ReadAmbInt(file, true);
    chunk->number = ReadAmbInt(file, true);
    chunk->flags = ReadAmbInt(file, false);
    chunk->maxRandomPitch = ReadAmbInt(file, false);
    chunk->minRandomPitch = ReadAmbInt(file, false);
    chunk->param1 = ReadAmbInt(file, false);
    chunk->param2 = ReadAmbInt(file, false);
    
    // Read end marker
    unsigned int endMarker = ReadAmbInt(file, true);
    if (endMarker != 0xFA) {
        MessageBox(NULL, "Expected 0xFA marker before strings in Prgm chunk", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    ReadString(file, chunk->effectName, sizeof(chunk->effectName));
    ReadString(file, chunk->varName, sizeof(chunk->varName));
    
    return true;
}

bool ParseKmapChunk(FILE *file, KmapChunk *chunk) {
    chunk->size = ReadAmbInt(file, true);
    chunk->flags = ReadAmbInt(file, true);
    chunk->param1 = ReadAmbInt(file, true);
    chunk->param2 = ReadAmbInt(file, true);
    
    ReadString(file, chunk->varName, sizeof(chunk->varName));
    
    chunk->itemCount = ReadAmbInt(file, true);
    
    if ((chunk->flags & 6) != 0) {
        chunk->itemSize = ReadAmbInt(file, true);
    } else {
        chunk->itemSize = 0;
    }
    
    // Read items
    for (int i = 0; i < chunk->itemCount && i < MAX_ITEMS; i++) {
        if ((chunk->flags & 6) == 0) {
            // Not used in Civ3
            int aint1 = ReadAmbInt(file, true);
            int aint2 = ReadAmbInt(file, true);
        } else {
            // Read the 12-byte data block
            fread(chunk->items[i].data, 1, chunk->itemSize, file);
        }
        
        ReadString(file, chunk->items[i].wavFileName, sizeof(chunk->items[i].wavFileName));
    }
    
    // Read end marker
    unsigned int endMarker = ReadAmbInt(file, true);
    if (endMarker != 0xFA) {
        MessageBox(NULL, "Expected 0xFA marker at end of Kmap chunk", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    return true;
}

bool ParseGlblChunk(FILE *file, GlblChunk *chunk) {
    chunk->size = ReadAmbInt(file, true);
    long startPos = ftell(file);
    
    chunk->dataSize = ReadAmbInt(file, true);
    fread(chunk->data, 1, chunk->dataSize, file);
    
    // Read any extra data
    long currentPos = ftell(file);
    long remainingSize = chunk->size - (currentPos - startPos);
    
    if (remainingSize > 0 && remainingSize < sizeof(chunk->extraData)) {
        fread(chunk->extraData, 1, remainingSize, file);
        chunk->extraDataSize = (int)remainingSize;
    } else {
        chunk->extraDataSize = 0;
    }
    
    return true;
}

bool ParseMidiTrackEvent(FILE *file, MidiTrack *track, MidiEvent *prevEvent) {
    MidiEvent event;
    memset(&event, 0, sizeof(MidiEvent));
    
    event.deltaTime = ReadMidiVarInt(file);
    
    unsigned char byte1;
    fread(&byte1, 1, 1, file);
    
    // Check for running status
    if (prevEvent != NULL && 
        (prevEvent->type == EVENT_CONTROL_CHANGE ||
         prevEvent->type == EVENT_PROGRAM_CHANGE ||
         prevEvent->type == EVENT_NOTE_OFF ||
         prevEvent->type == EVENT_NOTE_ON) && 
        (byte1 & 0x80) == 0) {
        
        // Use previous event type and channel
        event.type = prevEvent->type;
        
        // Put the byte back for reading by the specific event parser
        fseek(file, -1, SEEK_CUR);
        
        switch (event.type) {
            case EVENT_CONTROL_CHANGE:
                event.data.controlChange.channelNumber = prevEvent->data.controlChange.channelNumber;
                event.data.controlChange.controllerNumber = ReadMidiVarInt(file);
                event.data.controlChange.value = ReadMidiVarInt(file);
                break;
                
            case EVENT_PROGRAM_CHANGE:
                event.data.programChange.channelNumber = prevEvent->data.programChange.channelNumber;
                event.data.programChange.programNumber = ReadMidiVarInt(file);
                break;
                
            case EVENT_NOTE_OFF:
                event.data.noteOff.channelNumber = prevEvent->data.noteOff.channelNumber;
                event.data.noteOff.key = ReadMidiVarInt(file);
                event.data.noteOff.velocity = ReadMidiVarInt(file);
                break;
                
            case EVENT_NOTE_ON:
                event.data.noteOn.channelNumber = prevEvent->data.noteOn.channelNumber;
                event.data.noteOn.key = ReadMidiVarInt(file);
                event.data.noteOn.velocity = ReadMidiVarInt(file);
                break;
                
            default:
                // Should not happen
                event.type = EVENT_UNKNOWN;
                break;
        }
    } else if (byte1 == 0xFF) {
        // Meta event
        unsigned char metaType;
        fread(&metaType, 1, 1, file);
        
        if (metaType == 0x03) {
            // Track name
            event.type = EVENT_TRACK_NAME;
            unsigned int length = ReadMidiVarInt(file);
            fread(event.data.trackName.name, 1, length < sizeof(event.data.trackName.name) ? length : sizeof(event.data.trackName.name) - 1, file);
            event.data.trackName.name[length < sizeof(event.data.trackName.name) ? length : sizeof(event.data.trackName.name) - 1] = '\0';
        } else if (metaType == 0x2F) {
            // End of track
            unsigned char zero;
            fread(&zero, 1, 1, file);  // Should be zero
            event.type = EVENT_END_OF_TRACK;
        } else if (metaType == 0x51) {
            // Set tempo
            unsigned char length;
            fread(&length, 1, 1, file);  // Should be 3
            
            if (length == 3) {
                unsigned char tempo[3];
                fread(tempo, 1, 3, file);
                event.type = EVENT_SET_TEMPO;
                event.data.setTempo.microsecondsPerQuarterNote = (tempo[0] << 16) | (tempo[1] << 8) | tempo[2];
            } else {
                event.type = EVENT_UNKNOWN;
                // Skip data
                fseek(file, length, SEEK_CUR);
            }
        } else if (metaType == 0x54) {
            // SMPTE offset
            unsigned char length;
            fread(&length, 1, 1, file);  // Should be 5
            
            if (length == 5) {
                event.type = EVENT_SMPTE_OFFSET;
                event.data.smpteOffset.hr = fgetc(file);
                event.data.smpteOffset.mn = fgetc(file);
                event.data.smpteOffset.se = fgetc(file);
                event.data.smpteOffset.fr = fgetc(file);
                event.data.smpteOffset.ff = fgetc(file);
            } else {
                event.type = EVENT_UNKNOWN;
                // Skip data
                fseek(file, length, SEEK_CUR);
            }
        } else if (metaType == 0x58) {
            // Time signature
            unsigned char length;
            fread(&length, 1, 1, file);  // Should be 4
            
            if (length == 4) {
                event.type = EVENT_TIME_SIGNATURE;
                event.data.timeSignature.nn = fgetc(file);
                event.data.timeSignature.dd = fgetc(file);
                event.data.timeSignature.cc = fgetc(file);
                event.data.timeSignature.bb = fgetc(file);
            } else {
                event.type = EVENT_UNKNOWN;
                // Skip data
                fseek(file, length, SEEK_CUR);
            }
        } else {
            // Unknown meta event
            event.type = EVENT_UNKNOWN;
            unsigned int length = ReadMidiVarInt(file);
            // Skip data
            fseek(file, length, SEEK_CUR);
        }
    } else if ((byte1 >> 4) == 8) {
        // Note off
        event.type = EVENT_NOTE_OFF;
        event.data.noteOff.channelNumber = byte1 & 0x0F;
        event.data.noteOff.key = fgetc(file);
        event.data.noteOff.velocity = fgetc(file);
    } else if ((byte1 >> 4) == 9) {
        // Note on
        event.type = EVENT_NOTE_ON;
        event.data.noteOn.channelNumber = byte1 & 0x0F;
        event.data.noteOn.key = fgetc(file);
        event.data.noteOn.velocity = fgetc(file);
    } else if ((byte1 >> 4) == 0xB) {
        // Control change
        event.type = EVENT_CONTROL_CHANGE;
        event.data.controlChange.channelNumber = byte1 & 0x0F;
        event.data.controlChange.controllerNumber = fgetc(file);
        event.data.controlChange.value = fgetc(file);
    } else if ((byte1 >> 4) == 0xC) {
        // Program change
        event.type = EVENT_PROGRAM_CHANGE;
        event.data.programChange.channelNumber = byte1 & 0x0F;
        event.data.programChange.programNumber = fgetc(file);
    } else {
        // Unknown event
        event.type = EVENT_UNKNOWN;
        // Store the first byte
        event.data.unknown.data[0] = byte1;
        event.data.unknown.dataLength = 1;
    }
    
    // Add the event to the track
    if (track->eventCount < MAX_EVENTS) {
        track->events[track->eventCount++] = event;
        return true;
    }
    
    return false;
}

bool ParseMidiTrack(FILE *file, MidiTrack *track) {
    track->size = ReadMidiInt(file, true);
    long startPos = ftell(file);
    track->eventCount = 0;
    
    // Read track events until we reach the end of the track
    while (ftell(file) < startPos + track->size && track->eventCount < MAX_EVENTS) {
        MidiEvent *prevEvent = (track->eventCount > 0) ? &track->events[track->eventCount - 1] : NULL;
        if (!ParseMidiTrackEvent(file, track, prevEvent)) {
            return false;
        }
        
        // If we got an unknown event, skip to the end of the track
        if (track->events[track->eventCount - 1].type == EVENT_UNKNOWN) {
            fseek(file, startPos + track->size, SEEK_SET);
            break;
        }
    }
    
    return true;
}

bool ParseMidi(FILE *file, MidiData *midi) {
    // Read header size
    unsigned char sizeBytes[4];
    fread(sizeBytes, 1, 4, file);
    
    // Values for MIDI header should be big-endian (MSB first)
    int headerSize = (sizeBytes[0] << 24) | (sizeBytes[1] << 16) | (sizeBytes[2] << 8) | sizeBytes[3];
    
    // The header size should be 6 for standard MIDI files
    if (headerSize != 6) {
        MessageBox(NULL, "Invalid MIDI header size", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Read format, track count, and division
    unsigned char formatBytes[2], trackCountBytes[2], divisionBytes[2];
    
    fread(formatBytes, 1, 2, file);
    fread(trackCountBytes, 1, 2, file);
    fread(divisionBytes, 1, 2, file);
    
    // Interpret as big-endian values
    midi->format = (formatBytes[0] << 8) | formatBytes[1];
    midi->trackCount = (trackCountBytes[0] << 8) | trackCountBytes[1];
    midi->ticksPerQuarterNote = (divisionBytes[0] << 8) | divisionBytes[1];
    
    if (midi->format != 1) {
        MessageBox(NULL, "Unsupported MIDI format", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    if (midi->trackCount < 2 || midi->trackCount > MAX_TRACKS) {
        MessageBox(NULL, "Invalid MIDI track count", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    if ((midi->ticksPerQuarterNote & 0x8000) != 0) {
        MessageBox(NULL, "Unsupported MIDI time division format", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Read all tracks
    for (int i = 0; i < midi->trackCount; i++) {
        char trackTag[5] = {0};
        fread(trackTag, 1, 4, file);
        
        if (strcmp(trackTag, "MTrk") != 0) {
            MessageBox(NULL, "Invalid MIDI track header", "Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        if (!ParseMidiTrack(file, &midi->tracks[i])) {
            return false;
        }
    }
    
    // Get tempo from the first track
    for (int i = 0; i < midi->tracks[0].eventCount; i++) {
        MidiEvent *event = &midi->tracks[0].events[i];
        if (event->type == EVENT_SET_TEMPO) {
            midi->secondsPerQuarterNote = event->data.setTempo.microsecondsPerQuarterNote / 1000000.0f;
            break;
        }
    }
    
    return true;
}

// Load AMB file
bool LoadAmbFile(const char *filePath) {
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        MessageBox(NULL, "Failed to open AMB file", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Initialize AMB file structure
    memset(&g_ambFile, 0, sizeof(AmbFile));
    strcpy(g_ambFile.filePath, filePath);
    
    // Parse all chunks
    bool success = true;
    while (!feof(file)) {
        char tag[5] = {0};
        if (fread(tag, 1, 4, file) != 4) {
            break;  // End of file
        }
        
        if (strcmp(tag, "prgm") == 0) {
            if (g_ambFile.prgmChunkCount < MAX_CHUNKS) {
                if (!ParsePrgmChunk(file, &g_ambFile.prgmChunks[g_ambFile.prgmChunkCount++])) {
                    success = false;
                    break;
                }
            } else {
                MessageBox(NULL, "Too many Prgm chunks", "Error", MB_OK | MB_ICONERROR);
                success = false;
                break;
            }
        } else if (strcmp(tag, "kmap") == 0) {
            if (g_ambFile.kmapChunkCount < MAX_CHUNKS) {
                if (!ParseKmapChunk(file, &g_ambFile.kmapChunks[g_ambFile.kmapChunkCount++])) {
                    success = false;
                    break;
                }
            } else {
                MessageBox(NULL, "Too many Kmap chunks", "Error", MB_OK | MB_ICONERROR);
                success = false;
                break;
            }
        } else if (strcmp(tag, "glbl") == 0) {
            if (!g_ambFile.hasGlblChunk) {
                if (!ParseGlblChunk(file, &g_ambFile.glblChunk)) {
                    success = false;
                    break;
                }
                g_ambFile.hasGlblChunk = true;
            } else {
                MessageBox(NULL, "Multiple Glbl chunks not supported", "Error", MB_OK | MB_ICONERROR);
                success = false;
                break;
            }
        } else if (strcmp(tag, "MThd") == 0) {
            // MIDI header detected, parse the MIDI data
            // Don't need to go back, ParseMidi now expects to be positioned after the tag
            if (!ParseMidi(file, &g_ambFile.midi)) {
                success = false;
                break;
            }
        } else {
            char message[256];
            sprintf(message, "Unknown chunk tag: %s", tag);
            MessageBox(NULL, message, "Error", MB_OK | MB_ICONERROR);
            success = false;
            break;
        }
    }
    
    fclose(file);
    return success;
}

// Function to describe the loaded AMB file (for debugging)
void DescribeAmbFile(char *buffer, size_t bufferSize) {
    char *pos = buffer;
    int remainingSize = (int)bufferSize;
    
    // Describe file path
    int written = snprintf(pos, remainingSize, "File: %s\n\n", g_ambFile.filePath);
    pos += written;
    remainingSize -= written;
    
    // Describe Prgm chunks
    for (int i = 0; i < g_ambFile.prgmChunkCount && remainingSize > 0; i++) {
        PrgmChunk *chunk = &g_ambFile.prgmChunks[i];
        written = snprintf(pos, remainingSize, 
                    "Prgm #%d:\n"
                    "  Size: %d\n"
                    "  Number: %d\n"
                    "  Flags: %d\n"
                    "  Max Random Pitch: %d\n"
                    "  Min Random Pitch: %d\n"
                    "  Param1: %d\n"
                    "  Param2: %d\n"
                    "  Effect Name: '%s'\n"
                    "  Var Name: '%s'\n\n",
                    i + 1, chunk->size, chunk->number, chunk->flags,
                    chunk->maxRandomPitch, chunk->minRandomPitch,
                    chunk->param1, chunk->param2,
                    chunk->effectName, chunk->varName);
        pos += written;
        remainingSize -= written;
    }
    
    // Describe Kmap chunks
    for (int i = 0; i < g_ambFile.kmapChunkCount && remainingSize > 0; i++) {
        KmapChunk *chunk = &g_ambFile.kmapChunks[i];
        written = snprintf(pos, remainingSize, 
                    "Kmap #%d:\n"
                    "  Size: %d\n"
                    "  Flags: %d\n"
                    "  Param1: %d\n"
                    "  Param2: %d\n"
                    "  Var Name: '%s'\n"
                    "  Item Count: %d\n"
                    "  Item Size: %d\n",
                    i + 1, chunk->size, chunk->flags,
                    chunk->param1, chunk->param2,
                    chunk->varName, chunk->itemCount, chunk->itemSize);
        pos += written;
        remainingSize -= written;
        
        // Describe Kmap items
        for (int j = 0; j < chunk->itemCount && remainingSize > 0; j++) {
            written = snprintf(pos, remainingSize, 
                        "    Item #%d: WAV File: '%s'\n",
                        j + 1, chunk->items[j].wavFileName);
            pos += written;
            remainingSize -= written;
        }
        
        written = snprintf(pos, remainingSize, "\n");
        pos += written;
        remainingSize -= written;
    }
    
    // Describe Glbl chunk
    if (g_ambFile.hasGlblChunk && remainingSize > 0) {
        GlblChunk *chunk = &g_ambFile.glblChunk;
        written = snprintf(pos, remainingSize, 
                    "Glbl:\n"
                    "  Size: %d\n"
                    "  Data Size: %d\n\n",
                    chunk->size, chunk->dataSize);
        pos += written;
        remainingSize -= written;
    }
    
    // Describe MIDI data
    if (remainingSize > 0) {
        written = snprintf(pos, remainingSize, 
                    "MIDI:\n"
                    "  Format: %d\n"
                    "  Track Count: %d\n"
                    "  Ticks Per Quarter Note: %d\n"
                    "  Seconds Per Quarter Note: %.6f\n\n",
                    g_ambFile.midi.format, g_ambFile.midi.trackCount,
                    g_ambFile.midi.ticksPerQuarterNote, g_ambFile.midi.secondsPerQuarterNote);
        pos += written;
        remainingSize -= written;
        
        // Describe each track
        for (int i = 0; i < g_ambFile.midi.trackCount && remainingSize > 0; i++) {
            MidiTrack *track = &g_ambFile.midi.tracks[i];
            written = snprintf(pos, remainingSize, 
                        "  Track #%d:\n"
                        "    Size: %d\n"
                        "    Event Count: %d\n",
                        i + 1, track->size, track->eventCount);
            pos += written;
            remainingSize -= written;
            
            // Describe a few events for each track
            float timestamp = 0.0f;
            float secondsPerTick = g_ambFile.midi.secondsPerQuarterNote / g_ambFile.midi.ticksPerQuarterNote;
            
            for (int j = 0; j < track->eventCount && j < 20 && remainingSize > 0; j++) {
                MidiEvent *event = &track->events[j];
                timestamp += event->deltaTime * secondsPerTick;
                
                switch (event->type) {
                    case EVENT_TRACK_NAME:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: TrackName '%s'\n",
                                    timestamp, event->data.trackName.name);
                        break;
                    case EVENT_SET_TEMPO:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: SetTempo %d\n",
                                    timestamp, event->data.setTempo.microsecondsPerQuarterNote);
                        break;
                    case EVENT_TIME_SIGNATURE:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: TimeSignature %d %d %d %d\n",
                                    timestamp, event->data.timeSignature.nn, 
                                    event->data.timeSignature.dd, 
                                    event->data.timeSignature.cc, 
                                    event->data.timeSignature.bb);
                        break;
                    case EVENT_CONTROL_CHANGE:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: ControlChange %d %d %d\n",
                                    timestamp, event->data.controlChange.channelNumber, 
                                    event->data.controlChange.controllerNumber, 
                                    event->data.controlChange.value);
                        break;
                    case EVENT_PROGRAM_CHANGE:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: ProgramChange %d %d\n",
                                    timestamp, event->data.programChange.channelNumber, 
                                    event->data.programChange.programNumber);
                        break;
                    case EVENT_NOTE_ON:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: NoteOn %d %d %d\n",
                                    timestamp, event->data.noteOn.channelNumber, 
                                    event->data.noteOn.key, 
                                    event->data.noteOn.velocity);
                        break;
                    case EVENT_NOTE_OFF:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: NoteOff %d %d %d\n",
                                    timestamp, event->data.noteOff.channelNumber, 
                                    event->data.noteOff.key, 
                                    event->data.noteOff.velocity);
                        break;
                    case EVENT_END_OF_TRACK:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: EndOfTrack\n",
                                    timestamp);
                        break;
                    case EVENT_SMPTE_OFFSET:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: SMPTEOffset %d %d %d %d %d\n",
                                    timestamp, event->data.smpteOffset.hr, 
                                    event->data.smpteOffset.mn, 
                                    event->data.smpteOffset.se, 
                                    event->data.smpteOffset.fr, 
                                    event->data.smpteOffset.ff);
                        break;
                    case EVENT_UNKNOWN:
                        written = snprintf(pos, remainingSize, 
                                    "      %.3f: Unknown event\n",
                                    timestamp);
                        break;
                }
                
                pos += written;
                remainingSize -= written;
            }
            
            if (track->eventCount > 20 && remainingSize > 0) {
                written = snprintf(pos, remainingSize, "      ... (more events)\n");
                pos += written;
                remainingSize -= written;
            }
            
            if (remainingSize > 0) {
                written = snprintf(pos, remainingSize, "\n");
                pos += written;
                remainingSize -= written;
            }
        }
    }
}

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

// Test AMB file loading
void TestAmbLoading(HWND hwnd)
{
    // Find a sample AMB file to load
    if (strlen(g_civ3MainPath) > 0) {
        char ambFilePath[MAX_PATH_LENGTH];
        strcpy(ambFilePath, g_civ3MainPath);
        PathAppend(ambFilePath, "Art");
        PathAppend(ambFilePath, "Units");
        PathAppend(ambFilePath, "Archer");
        PathAppend(ambFilePath, "ArcherAttack.amb");
        
        if (PathFileExists(ambFilePath)) {
            if (LoadAmbFile(ambFilePath)) {
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
bool ValidateCiv3Path(HWND hwnd)
{
    char path[MAX_PATH_LENGTH];
    
    // Get text from edit control
    GetWindowText(g_hwndPathEdit, path, MAX_PATH_LENGTH);
    
    // Check if it's a valid Civ3 directory
    if (IsCiv3MainFolder(path)) {
        strcpy(g_civ3MainPath, path);
        
        // Assume Conquests is in a subfolder named "Conquests"
        strcpy(g_civ3ConquestsPath, g_civ3MainPath);
        PathAppend(g_civ3ConquestsPath, "Conquests");
        
        return true;
    } else {
        MessageBox(hwnd, "The entered folder does not appear to be a valid Civilization III installation.", 
                  "Invalid Path", MB_OK | MB_ICONWARNING);
    }
    
    return false;
}

// Create a simple path input dialog
void CreatePathInputDialog(HWND hwnd)
{
    // Create a label
    HWND hwndLabel = CreateWindow(
        "STATIC", 
        "Enter Civilization III installation path:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 50, 300, 20,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    
    // Create a text box
    g_hwndPathEdit = CreateWindow(
        "EDIT", 
        "", 
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        20, 70, 350, 25,
        hwnd, (HMENU)ID_PATH_EDIT, GetModuleHandle(NULL), NULL
    );
    
    // Create a button
    HWND hwndButton = CreateWindow(
        "BUTTON", 
        "Validate", 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        380, 70, 80, 25,
        hwnd, (HMENU)ID_BROWSE_BUTTON, GetModuleHandle(NULL), NULL
    );
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
        // Create dialog to enter path
        CreatePathInputDialog(hwnd);
    }
    
    return false;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            // Find Civ3 installation when window is created
            FindCiv3Installation(hwnd);
            g_hwndMainWindow = hwnd;
            
            // Directly test AMB loading after the window is created
            TestAmbLoading(hwnd);
            return 0;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BROWSE_BUTTON) {
                if (ValidateCiv3Path(hwnd)) {
                    // Test AMB loading with the found installation
                    TestAmbLoading(hwnd);
                }
            }
            return 0;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Register the window class
    const char CLASS_NAME[] = "AMB Editor Window Class";
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClass(&wc);
    
    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                          // Optional window styles
        CLASS_NAME,                 // Window class
        "AMB Editor",               // Window title
        WS_OVERLAPPEDWINDOW,        // Window style
        
        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        
        NULL,       // Parent window    
        NULL,       // Menu
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
