// This file gets #included into amb_editor.c

#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH_LENGTH 1024
#define MAX_CHUNKS 100
#define MAX_TRACKS 20
#define MAX_EVENTS 1000
#define MAX_ITEMS 10

// AMB file structures
typedef struct {
    int size;
    int number;
    int flags;
    int maxRandomSpeed;
    int minRandomSpeed;
    int maxRandomVolume;
    int minRandomVolume;
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

// Helper functions for file parsing and writing
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

// Helper functions for file writing
void WriteAmbInt(FILE *file, unsigned int value, bool isUnsigned) {
    unsigned char buffer[4];
    
    // Little endian
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
    
    fwrite(buffer, 1, 4, file);
}

void WriteMidiInt(FILE *file, unsigned int value, bool isUnsigned) {
    unsigned char buffer[4];
    
    // Big endian
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0xFF;
    buffer[3] = value & 0xFF;
    
    fwrite(buffer, 1, 4, file);
}

void WriteMidiShort(FILE *file, unsigned short value, bool isUnsigned) {
    unsigned char buffer[2];
    
    // Big endian
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
    
    fwrite(buffer, 1, 2, file);
}

void WriteMidiVarInt(FILE *file, unsigned int value) {
    unsigned char buffer[4];
    int numBytes = 0;
    
    // Build the bytes from least significant to most significant
    do {
        buffer[numBytes++] = (value & 0x7F) | (numBytes > 0 ? 0x80 : 0);
        value >>= 7;
    } while (value > 0 && numBytes < 4);
    
    // Write the bytes in reverse order
    for (int i = numBytes - 1; i >= 0; i--) {
        // Clear the continuation bit for the last byte
        if (i == 0) {
            buffer[i] &= 0x7F;
        }
        fwrite(&buffer[i], 1, 1, file);
    }
}

void WriteString(FILE *file, const char *str) {
    // Write null-terminated string
    fwrite(str, 1, strlen(str) + 1, file);
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
    chunk->maxRandomSpeed = ReadAmbInt(file, false);
    chunk->minRandomSpeed = ReadAmbInt(file, false);
    chunk->maxRandomVolume = ReadAmbInt(file, false);
    chunk->minRandomVolume = ReadAmbInt(file, false);
    
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
bool LoadAmbFile(const char *filePath, AmbFile * out) {
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        MessageBox(NULL, "Failed to open AMB file", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Initialize AMB file structure
    memset(out, 0, sizeof *out);
    strcpy(out->filePath, filePath);
    
    // Parse all chunks
    bool success = true;
    while (!feof(file)) {
        char tag[5] = {0};
        if (fread(tag, 1, 4, file) != 4) {
            break;  // End of file
        }
        
        if (strcmp(tag, "prgm") == 0) {
            if (out->prgmChunkCount < MAX_CHUNKS) {
                if (!ParsePrgmChunk(file, &out->prgmChunks[out->prgmChunkCount++])) {
                    success = false;
                    break;
                }
            } else {
                MessageBox(NULL, "Too many Prgm chunks", "Error", MB_OK | MB_ICONERROR);
                success = false;
                break;
            }
        } else if (strcmp(tag, "kmap") == 0) {
            if (out->kmapChunkCount < MAX_CHUNKS) {
                if (!ParseKmapChunk(file, &out->kmapChunks[out->kmapChunkCount++])) {
                    success = false;
                    break;
                }
            } else {
                MessageBox(NULL, "Too many Kmap chunks", "Error", MB_OK | MB_ICONERROR);
                success = false;
                break;
            }
        } else if (strcmp(tag, "glbl") == 0) {
            if (!out->hasGlblChunk) {
                if (!ParseGlblChunk(file, &out->glblChunk)) {
                    success = false;
                    break;
                }
                out->hasGlblChunk = true;
            } else {
                MessageBox(NULL, "Multiple Glbl chunks not supported", "Error", MB_OK | MB_ICONERROR);
                success = false;
                break;
            }
        } else if (strcmp(tag, "MThd") == 0) {
            // MIDI header detected, parse the MIDI data
            // Don't need to go back, ParseMidi now expects to be positioned after the tag
            if (!ParseMidi(file, &out->midi)) {
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

// Functions to write AMB file chunks
bool WritePrgmChunk(FILE *file, PrgmChunk const * chunk) {
    // Write tag
    fwrite("prgm", 1, 4, file);
    
    // Write chunk data
    WriteAmbInt(file, chunk->size, true);
    WriteAmbInt(file, chunk->number, true);
    WriteAmbInt(file, chunk->flags, false);
    WriteAmbInt(file, chunk->maxRandomSpeed, false);
    WriteAmbInt(file, chunk->minRandomSpeed, false);
    WriteAmbInt(file, chunk->maxRandomVolume, false);
    WriteAmbInt(file, chunk->minRandomVolume, false);
    
    // Write end marker before strings
    WriteAmbInt(file, 0xFA, true);
    
    // Write strings
    WriteString(file, chunk->effectName);
    WriteString(file, chunk->varName);
    
    return true;
}

bool WriteKmapChunk(FILE *file, KmapChunk const * chunk) {
    // Write tag
    fwrite("kmap", 1, 4, file);
    
    // Write chunk data
    WriteAmbInt(file, chunk->size, true);
    WriteAmbInt(file, chunk->flags, true);
    WriteAmbInt(file, chunk->param1, true);
    WriteAmbInt(file, chunk->param2, true);
    
    // Write variable name
    WriteString(file, chunk->varName);
    
    // Write item count
    WriteAmbInt(file, chunk->itemCount, true);
    
    // Write item size if needed
    if ((chunk->flags & 6) != 0) {
        WriteAmbInt(file, chunk->itemSize, true);
    }
    
    // Write items
    for (int i = 0; i < chunk->itemCount && i < MAX_ITEMS; i++) {
        if ((chunk->flags & 6) == 0) {
            // Not used in Civ3, but we should still write placeholder data
            WriteAmbInt(file, 0, true);  // Placeholder for aint1
            WriteAmbInt(file, 0, true);  // Placeholder for aint2
        } else {
            // Write the data block
            fwrite(chunk->items[i].data, 1, chunk->itemSize, file);
        }
        
        // Write WAV filename
        WriteString(file, chunk->items[i].wavFileName);
    }
    
    // Write end marker
    WriteAmbInt(file, 0xFA, true);
    
    return true;
}

bool WriteGlblChunk(FILE *file, GlblChunk const * chunk) {
    // Write tag
    fwrite("glbl", 1, 4, file);
    
    // Write chunk size
    WriteAmbInt(file, chunk->size, true);
    
    // Write data size
    WriteAmbInt(file, chunk->dataSize, true);
    
    // Write data
    fwrite(chunk->data, 1, chunk->dataSize, file);
    
    // Write extra data if present
    if (chunk->extraDataSize > 0) {
        fwrite(chunk->extraData, 1, chunk->extraDataSize, file);
    }
    
    return true;
}

bool WriteMidiEvent(FILE *file, MidiEvent const * event) {
    // Write delta time
    WriteMidiVarInt(file, event->deltaTime);
    
    // Write event data based on type
    switch (event->type) {
        case EVENT_TRACK_NAME:
            fputc(0xFF, file);  // Meta event
            fputc(0x03, file);  // Track name
            
            // Length of the name
            WriteMidiVarInt(file, strlen(event->data.trackName.name));
            
            // Name data
            fwrite(event->data.trackName.name, 1, strlen(event->data.trackName.name), file);
            break;
            
        case EVENT_SMPTE_OFFSET:
            fputc(0xFF, file);  // Meta event
            fputc(0x54, file);  // SMPTE offset
            fputc(0x05, file);  // Length is always 5
            
            // SMPTE data
            fputc(event->data.smpteOffset.hr, file);
            fputc(event->data.smpteOffset.mn, file);
            fputc(event->data.smpteOffset.se, file);
            fputc(event->data.smpteOffset.fr, file);
            fputc(event->data.smpteOffset.ff, file);
            break;
            
        case EVENT_TIME_SIGNATURE:
            fputc(0xFF, file);  // Meta event
            fputc(0x58, file);  // Time signature
            fputc(0x04, file);  // Length is always 4
            
            // Time signature data
            fputc(event->data.timeSignature.nn, file);
            fputc(event->data.timeSignature.dd, file);
            fputc(event->data.timeSignature.cc, file);
            fputc(event->data.timeSignature.bb, file);
            break;
            
        case EVENT_SET_TEMPO:
            fputc(0xFF, file);  // Meta event
            fputc(0x51, file);  // Set tempo
            fputc(0x03, file);  // Length is always 3
            
            // Tempo data (microseconds per quarter note)
            unsigned int tempo = event->data.setTempo.microsecondsPerQuarterNote;
            fputc((tempo >> 16) & 0xFF, file);
            fputc((tempo >> 8) & 0xFF, file);
            fputc(tempo & 0xFF, file);
            break;
            
        case EVENT_END_OF_TRACK:
            fputc(0xFF, file);  // Meta event
            fputc(0x2F, file);  // End of track
            fputc(0x00, file);  // Length is always 0
            break;
            
        case EVENT_CONTROL_CHANGE:
            // Control change status byte: 0xBn, where n is the channel number
            fputc(0xB0 | (event->data.controlChange.channelNumber & 0x0F), file);
            
            // Controller number and value
            fputc(event->data.controlChange.controllerNumber & 0x7F, file);
            fputc(event->data.controlChange.value & 0x7F, file);
            break;
            
        case EVENT_PROGRAM_CHANGE:
            // Program change status byte: 0xCn, where n is the channel number
            fputc(0xC0 | (event->data.programChange.channelNumber & 0x0F), file);
            
            // Program number
            fputc(event->data.programChange.programNumber & 0x7F, file);
            break;
            
        case EVENT_NOTE_OFF:
            // Note off status byte: 0x8n, where n is the channel number
            fputc(0x80 | (event->data.noteOff.channelNumber & 0x0F), file);
            
            // Key and velocity
            fputc(event->data.noteOff.key & 0x7F, file);
            fputc(event->data.noteOff.velocity & 0x7F, file);
            break;
            
        case EVENT_NOTE_ON:
            // Note on status byte: 0x9n, where n is the channel number
            fputc(0x90 | (event->data.noteOn.channelNumber & 0x0F), file);
            
            // Key and velocity
            fputc(event->data.noteOn.key & 0x7F, file);
            fputc(event->data.noteOn.velocity & 0x7F, file);
            break;
            
        case EVENT_UNKNOWN:
            // For unknown events, write the raw data
            if (event->data.unknown.dataLength > 0) {
                fwrite(event->data.unknown.data, 1, event->data.unknown.dataLength, file);
            }
            break;
    }
    
    return true;
}

bool WriteMidiTrack(FILE *file, MidiTrack const * track) {
    // Write track tag
    fwrite("MTrk", 1, 4, file);
    
    // We need to calculate track size, so we'll save the current position,
    // write a placeholder, write the events, then come back and update it
    long sizePos = ftell(file);
    
    // Write placeholder for size
    WriteMidiInt(file, 0, true);
    
    // Save position at start of track data
    long startPos = ftell(file);
    
    // Write track events
    for (int i = 0; i < track->eventCount; i++) {
        WriteMidiEvent(file, &track->events[i]);
    }
    
    // Get end position and calculate size
    long endPos = ftell(file);
    unsigned int trackSize = (unsigned int)(endPos - startPos);
    
    // Go back and write the actual size
    fseek(file, sizePos, SEEK_SET);
    WriteMidiInt(file, trackSize, true);
    
    // Return to the end of the track
    fseek(file, endPos, SEEK_SET);
    
    return true;
}

bool WriteMidi(FILE *file, MidiData const * midi) {
    // Write MIDI header
    fwrite("MThd", 1, 4, file);
    
    // Header size is always 6
    WriteMidiInt(file, 6, true);
    
    // Write format, track count, and division
    WriteMidiShort(file, midi->format, false);
    WriteMidiShort(file, midi->trackCount, false);
    WriteMidiShort(file, midi->ticksPerQuarterNote, false);
    
    // Write all tracks
    for (int i = 0; i < midi->trackCount; i++) {
        if (!WriteMidiTrack(file, &midi->tracks[i])) {
            return false;
        }
    }
    
    return true;
}

// Function to save AMB file to the specified path
bool SaveAmbFile(AmbFile const * amb, const char *filePath) {
    FILE *file = fopen(filePath, "wb");
    if (!file) {
        MessageBox(NULL, "Failed to create output AMB file", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    bool success = true;
    
    // Write Prgm chunks
    for (int i = 0; i < amb->prgmChunkCount; i++) {
        if (!WritePrgmChunk(file, &amb->prgmChunks[i])) {
            success = false;
            break;
        }
    }
    
    // Write Kmap chunks
    if (success) {
        for (int i = 0; i < amb->kmapChunkCount; i++) {
            if (!WriteKmapChunk(file, &amb->kmapChunks[i])) {
                success = false;
                break;
            }
        }
    }
    
    // Write Glbl chunk if present
    if (success && amb->hasGlblChunk) {
        if (!WriteGlblChunk(file, &amb->glblChunk)) {
            success = false;
        }
    }
    
    // Write MIDI data
    if (success) {
        if (!WriteMidi(file, &amb->midi)) {
            success = false;
        }
    }
    
    fclose(file);
    
    // Remove the file if we failed
    if (! success) {
        remove(filePath);
    }
    
    return success;
}

// Function to describe the loaded AMB file (for debugging)
void DescribeAmbFile(AmbFile const * amb, char *buffer, size_t bufferSize) {
    char *pos = buffer;
    int remainingSize = (int)bufferSize;
    
    // Describe file path
    int written = snprintf(pos, remainingSize, "File: %s\n\n", amb->filePath);
    pos += written;
    remainingSize -= written;
    
    // Describe Prgm chunks
    for (int i = 0; i < amb->prgmChunkCount && remainingSize > 0; i++) {
        PrgmChunk *chunk = &amb->prgmChunks[i];
        written = snprintf(pos, remainingSize, 
                    "Prgm #%d:\n"
                    "  Size: %d\n"
                    "  Number: %d\n"
                    "  Flags: %d\n"
                    "  Max Random Speed: %d\n"
                    "  Min Random Speed: %d\n"
                    "  Max Random Volume: %d\n"
                    "  Min Random Volume: %d\n"
                    "  Effect Name: '%s'\n"
                    "  Var Name: '%s'\n\n",
                    i + 1, chunk->size, chunk->number, chunk->flags,
                    chunk->maxRandomSpeed, chunk->minRandomSpeed,
                    chunk->maxRandomVolume, chunk->minRandomVolume,
                    chunk->effectName, chunk->varName);
        pos += written;
        remainingSize -= written;
    }
    
    // Describe Kmap chunks
    for (int i = 0; i < amb->kmapChunkCount && remainingSize > 0; i++) {
        KmapChunk *chunk = &amb->kmapChunks[i];
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
    if (amb->hasGlblChunk && remainingSize > 0) {
        GlblChunk const * chunk = &amb->glblChunk;
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
                    amb->midi.format, amb->midi.trackCount,
                    amb->midi.ticksPerQuarterNote, amb->midi.secondsPerQuarterNote);
        pos += written;
        remainingSize -= written;
        
        // Describe each track
        for (int i = 0; i < amb->midi.trackCount && remainingSize > 0; i++) {
            MidiTrack *track = &amb->midi.tracks[i];
            written = snprintf(pos, remainingSize, 
                        "  Track #%d:\n"
                        "    Size: %d\n"
                        "    Event Count: %d\n",
                        i + 1, track->size, track->eventCount);
            pos += written;
            remainingSize -= written;
            
            // Describe a few events for each track
            float timestamp = 0.0f;
            float secondsPerTick = amb->midi.secondsPerQuarterNote / amb->midi.ticksPerQuarterNote;
            
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
