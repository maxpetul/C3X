// This file gets #included into amb_editor.c

#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CHUNKS 30 // No AMB in Civ 3 has more than 12 Kmap or Prgm chunks
#define MAX_SOUND_TRACKS 30 // No AMB in Civ 3 has more than 12 sound tracks
#define MAX_CONTROL_CHANGES 2 // No AMB in Civ 3 has more than 2 control changes per track
#define MAX_ITEMS 2 // No AMB in Civ 3 has more than 1 item

// AMB file structures for Civ 3
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

// MIDI event types - specific to Civ3 AMB files
typedef struct {
    char name[256]; // Always "Seq-1" in Civ3 AMBs
} TrackNameEvent;

typedef struct {
    int hr, mn, se, fr, ff;
} SMPTEOffsetEvent;

typedef struct {
    int nn, dd, cc, bb;
} TimeSignatureEvent;

typedef struct {
    int microsecondsPerQuarterNote;
} SetTempoEvent;

typedef struct {
    int channelNumber, controllerNumber, value;
} ControlChangeEvent;

typedef struct {
    int channelNumber, programNumber;
} ProgramChangeEvent;

typedef struct {
    int channelNumber, key, velocity;
} NoteOffEvent;

typedef struct {
    int channelNumber, key, velocity;
} NoteOnEvent;

// Specific track structures for Civ3 AMB files
typedef struct {
    int size;
    int deltaTimeTrackName;
    TrackNameEvent trackName;
    int deltaTimeSMPTEOffset;
    SMPTEOffsetEvent smpteOffset;
    int deltaTimeTimeSignature;
    TimeSignatureEvent timeSignature;
    int deltaTimeSetTempo;
    SetTempoEvent setTempo;
    int deltaTimeEndOfTrack;
} InfoTrack;

typedef struct {
    int size;
    int deltaTimeTrackName;
    TrackNameEvent trackName;
    int controlChangeCount;
    int deltaTimeControlChanges[MAX_CONTROL_CHANGES];
    ControlChangeEvent controlChanges[MAX_CONTROL_CHANGES];
    int deltaTimeProgramChange;
    ProgramChangeEvent programChange;
    int deltaTimeNoteOn;
    NoteOnEvent noteOn;
    int deltaTimeNoteOff;
    NoteOffEvent noteOff;
    int deltaTimeEndOfTrack;
} SoundTrack;

typedef struct {
    short format;
    short trackCount;
    short ticksPerQuarterNote;
    float secondsPerQuarterNote;
    InfoTrack infoTrack;
    SoundTrack soundTracks[MAX_SOUND_TRACKS];
} MidiData;

typedef struct {
    Path filePath;
    
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

// Parse TrackName event from MIDI file
bool ParseTrackNameEvent(FILE *file, TrackNameEvent *event) {
    unsigned char metaType;
    fread(&metaType, 1, 1, file);
    
    if (metaType != 0x03) {
        MessageBox(NULL, "Expected TrackName event (0x03)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    unsigned int length = ReadMidiVarInt(file);
    memset(event->name, 0, sizeof(event->name));
    fread(event->name, 1, length < sizeof(event->name) ? length : sizeof(event->name) - 1, file);
    
    return true;
}

// Parse SMPTEOffset event from MIDI file
bool ParseSMPTEOffsetEvent(FILE *file, SMPTEOffsetEvent *event) {
    unsigned char metaType;
    fread(&metaType, 1, 1, file);
    
    if (metaType != 0x54) {
        MessageBox(NULL, "Expected SMPTEOffset event (0x54)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    unsigned char length;
    fread(&length, 1, 1, file);
    
    if (length != 5) {
        MessageBox(NULL, "Invalid SMPTEOffset event length (expected 5)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    event->hr = fgetc(file);
    event->mn = fgetc(file);
    event->se = fgetc(file);
    event->fr = fgetc(file);
    event->ff = fgetc(file);
    
    return true;
}

// Parse TimeSignature event from MIDI file
bool ParseTimeSignatureEvent(FILE *file, TimeSignatureEvent *event) {
    unsigned char metaType;
    fread(&metaType, 1, 1, file);
    
    if (metaType != 0x58) {
        MessageBox(NULL, "Expected TimeSignature event (0x58)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    unsigned char length;
    fread(&length, 1, 1, file);
    
    if (length != 4) {
        MessageBox(NULL, "Invalid TimeSignature event length (expected 4)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    event->nn = fgetc(file);
    event->dd = fgetc(file);
    event->cc = fgetc(file);
    event->bb = fgetc(file);
    
    return true;
}

// Parse SetTempo event from MIDI file
bool ParseSetTempoEvent(FILE *file, SetTempoEvent *event) {
    unsigned char metaType;
    fread(&metaType, 1, 1, file);
    
    if (metaType != 0x51) {
        MessageBox(NULL, "Expected SetTempo event (0x51)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    unsigned char length;
    fread(&length, 1, 1, file);
    
    if (length != 3) {
        MessageBox(NULL, "Invalid SetTempo event length (expected 3)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    unsigned char tempo[3];
    fread(tempo, 1, 3, file);
    event->microsecondsPerQuarterNote = (tempo[0] << 16) | (tempo[1] << 8) | tempo[2];
    
    return true;
}

// Parse EndOfTrack event from MIDI file
bool ParseEndOfTrackEvent(FILE *file) {
    unsigned char metaType;
    fread(&metaType, 1, 1, file);
    
    if (metaType != 0x2F) {
        MessageBox(NULL, "Expected EndOfTrack event (0x2F)", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    unsigned char zero;
    fread(&zero, 1, 1, file);  // Should be zero
    
    return true;
}

// Parse ControlChange event from MIDI file
bool ParseControlChangeEvent(FILE *file, ControlChangeEvent *event, int *channelNumber) {
    unsigned char statusByte;
    fread(&statusByte, 1, 1, file);
    
    // Check if the status byte indicates a control change event (0xBn)
    if ((statusByte >> 4) != 0xB) {
        // If channelNumber pointer is provided, we're expecting running status
        if (channelNumber != NULL) {
            // Put the byte back for reading data
            fseek(file, -1, SEEK_CUR);
            event->channelNumber = *channelNumber;
        } else {
            MessageBox(NULL, "Expected ControlChange event (0xBn)", "Error", MB_OK | MB_ICONERROR);
            return false;
        }
    } else {
        event->channelNumber = statusByte & 0x0F;
        // Save channel number for possible running status
        if (channelNumber != NULL) {
            *channelNumber = event->channelNumber;
        }
    }
    
    event->controllerNumber = fgetc(file);
    event->value = fgetc(file);
    
    return true;
}

// Parse ProgramChange event from MIDI file
bool ParseProgramChangeEvent(FILE *file, ProgramChangeEvent *event, int *channelNumber) {
    unsigned char statusByte;
    fread(&statusByte, 1, 1, file);
    
    // Check if the status byte indicates a program change event (0xCn)
    if ((statusByte >> 4) != 0xC) {
        // If channelNumber pointer is provided, we're expecting running status
        if (channelNumber != NULL) {
            // Put the byte back for reading data
            fseek(file, -1, SEEK_CUR);
            event->channelNumber = *channelNumber;
        } else {
            MessageBox(NULL, "Expected ProgramChange event (0xCn)", "Error", MB_OK | MB_ICONERROR);
            return false;
        }
    } else {
        event->channelNumber = statusByte & 0x0F;
        // Save channel number for possible running status
        if (channelNumber != NULL) {
            *channelNumber = event->channelNumber;
        }
    }
    
    event->programNumber = fgetc(file);
    
    return true;
}

// Parse NoteOn event from MIDI file
bool ParseNoteOnEvent(FILE *file, NoteOnEvent *event, int *channelNumber) {
    unsigned char statusByte;
    fread(&statusByte, 1, 1, file);
    
    // Check if the status byte indicates a note on event (0x9n)
    if ((statusByte >> 4) != 0x9) {
        // If channelNumber pointer is provided, we're expecting running status
        if (channelNumber != NULL) {
            // Put the byte back for reading data
            fseek(file, -1, SEEK_CUR);
            event->channelNumber = *channelNumber;
        } else {
            MessageBox(NULL, "Expected NoteOn event (0x9n)", "Error", MB_OK | MB_ICONERROR);
            return false;
        }
    } else {
        event->channelNumber = statusByte & 0x0F;
        // Save channel number for possible running status
        if (channelNumber != NULL) {
            *channelNumber = event->channelNumber;
        }
    }
    
    event->key = fgetc(file);
    event->velocity = fgetc(file);
    
    return true;
}

// Parse NoteOff event from MIDI file
bool ParseNoteOffEvent(FILE *file, NoteOffEvent *event, int *channelNumber) {
    unsigned char statusByte;
    fread(&statusByte, 1, 1, file);
    
    // Check if the status byte indicates a note off event (0x8n)
    if ((statusByte >> 4) != 0x8) {
        // Check if it's a note on with velocity 0 (equivalent to note off)
        if ((statusByte >> 4) == 0x9) {
            event->channelNumber = statusByte & 0x0F;
            event->key = fgetc(file);
            event->velocity = fgetc(file);
            
            // Check if velocity is 0 (note off)
            if (event->velocity > 0) {
                MessageBox(NULL, "Expected NoteOff event but got NoteOn with velocity > 0", "Error", MB_OK | MB_ICONERROR);
                return false;
            }
            
            // Save channel number for possible running status
            if (channelNumber != NULL) {
                *channelNumber = event->channelNumber;
            }
            
            return true;
        }
        
        // If channelNumber pointer is provided, we're expecting running status
        if (channelNumber != NULL) {
            // Put the byte back for reading data
            fseek(file, -1, SEEK_CUR);
            event->channelNumber = *channelNumber;
        } else {
            MessageBox(NULL, "Expected NoteOff event (0x8n)", "Error", MB_OK | MB_ICONERROR);
            return false;
        }
    } else {
        event->channelNumber = statusByte & 0x0F;
        // Save channel number for possible running status
        if (channelNumber != NULL) {
            *channelNumber = event->channelNumber;
        }
    }
    
    event->key = fgetc(file);
    event->velocity = fgetc(file);
    
    return true;
}

// Parse InfoTrack from MIDI file
bool ParseInfoTrack(FILE *file, InfoTrack *track) {
    // Read the file tag
    char trackTag[5] = {0};
    fread(trackTag, 1, 4, file);
    
    if (strcmp(trackTag, "MTrk") != 0) {
        char errMsg[256];
        sprintf(errMsg, "Invalid MIDI info track header: expected 'MTrk', got '%.4s'", trackTag);
        MessageBox(NULL, errMsg, "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Read track chunk size
    track->size = ReadMidiInt(file, true);
    long startPos = ftell(file);
    
    // Read events in the InfoTrack
    
    // 1. TrackName event
    track->deltaTimeTrackName = ReadMidiVarInt(file);
    if (fgetc(file) != 0xFF) { // Meta event marker
        MessageBox(NULL, "Expected Meta event marker (0xFF) for TrackName", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseTrackNameEvent(file, &track->trackName)) return false;
    
    // 2. SMPTEOffset event
    track->deltaTimeSMPTEOffset = ReadMidiVarInt(file);
    if (fgetc(file) != 0xFF) { // Meta event marker
        MessageBox(NULL, "Expected Meta event marker (0xFF) for SMPTEOffset", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseSMPTEOffsetEvent(file, &track->smpteOffset)) return false;
    
    // 3. TimeSignature event
    track->deltaTimeTimeSignature = ReadMidiVarInt(file);
    if (fgetc(file) != 0xFF) { // Meta event marker
        MessageBox(NULL, "Expected Meta event marker (0xFF) for TimeSignature", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseTimeSignatureEvent(file, &track->timeSignature)) return false;
    
    // 4. SetTempo event
    track->deltaTimeSetTempo = ReadMidiVarInt(file);
    if (fgetc(file) != 0xFF) { // Meta event marker
        MessageBox(NULL, "Expected Meta event marker (0xFF) for SetTempo", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseSetTempoEvent(file, &track->setTempo)) return false;
    
    // 5. EndOfTrack event
    track->deltaTimeEndOfTrack = ReadMidiVarInt(file);
    if (fgetc(file) != 0xFF) { // Meta event marker
        MessageBox(NULL, "Expected Meta event marker (0xFF) for EndOfTrack", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseEndOfTrackEvent(file)) return false;
    
    // Ensure we've read exactly the size of the track
    long currentPos = ftell(file);
    if (currentPos != startPos + track->size) {
        MessageBox(NULL, "Info track size mismatch", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    return true;
}

// Parse SoundTrack from MIDI file
bool ParseSoundTrack(FILE *file, SoundTrack *track) {
    // Read the file tag
    char trackTag[5] = {0};
    fread(trackTag, 1, 4, file);
    
    if (strcmp(trackTag, "MTrk") != 0) {
        char errMsg[256];
        sprintf(errMsg, "Invalid MIDI sound track header: expected 'MTrk', got '%.4s'", trackTag);
        MessageBox(NULL, errMsg, "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Read track chunk size
    track->size = ReadMidiInt(file, true);
    long startPos = ftell(file);
    
    // Initialize control change count
    track->controlChangeCount = 0;
    
    // 1. TrackName event
    track->deltaTimeTrackName = ReadMidiVarInt(file);
    if (fgetc(file) != 0xFF) { // Meta event marker
        MessageBox(NULL, "Expected Meta event marker (0xFF) for TrackName", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseTrackNameEvent(file, &track->trackName)) return false;
    
    // 2. ControlChange events (1 or 2)
    unsigned char nextByte;
    int currentChannelNumber = -1; // For running status
    
    // Peek at the next byte to determine event type
    long peekPos = ftell(file);
    track->deltaTimeControlChanges[0] = ReadMidiVarInt(file);
    nextByte = fgetc(file);
    fseek(file, peekPos, SEEK_SET); // Go back to where we were
    
    // Check for control change events (0xBn)
    while (((nextByte >> 4) == 0xB || currentChannelNumber != -1) && 
           track->controlChangeCount < MAX_CONTROL_CHANGES) {
        
        // Read the delta time
        track->deltaTimeControlChanges[track->controlChangeCount] = ReadMidiVarInt(file);
        
        // Parse the control change event
        if (!ParseControlChangeEvent(file, &track->controlChanges[track->controlChangeCount], &currentChannelNumber)) {
            return false;
        }
        
        track->controlChangeCount++;
        
        // Peek at the next byte to determine if there's another control change
        if (track->controlChangeCount < MAX_CONTROL_CHANGES) {
            peekPos = ftell(file);
            int testDeltaTime = ReadMidiVarInt(file);
            nextByte = fgetc(file);
            fseek(file, peekPos, SEEK_SET); // Go back to where we were
            
            // If next byte is not a control change and not a running status, break
            if ((nextByte >> 4) != 0xB && (nextByte & 0x80) != 0) {
                currentChannelNumber = -1; // Reset running status
                break;
            }
        }
    }
    
    // 3. ProgramChange event
    track->deltaTimeProgramChange = ReadMidiVarInt(file);
    if (!ParseProgramChangeEvent(file, &track->programChange, NULL)) return false;
    
    // 4. NoteOn event
    track->deltaTimeNoteOn = ReadMidiVarInt(file);
    if (!ParseNoteOnEvent(file, &track->noteOn, NULL)) return false;
    
    // 5. NoteOff event
    track->deltaTimeNoteOff = ReadMidiVarInt(file);
    if (!ParseNoteOffEvent(file, &track->noteOff, NULL)) return false;
    
    // 6. EndOfTrack event
    track->deltaTimeEndOfTrack = ReadMidiVarInt(file);
    if (fgetc(file) != 0xFF) { // Meta event marker
        MessageBox(NULL, "Expected Meta event marker (0xFF) for EndOfTrack", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseEndOfTrackEvent(file)) return false;
    
    // Ensure we've read exactly the size of the track
    long currentPos = ftell(file);
    if (currentPos != startPos + track->size) {
        MessageBox(NULL, "Sound track size mismatch", "Error", MB_OK | MB_ICONERROR);
        return false;
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
    
    if (midi->trackCount < 2 || midi->trackCount > (MAX_SOUND_TRACKS + 1)) {
        MessageBox(NULL, "Invalid MIDI track count", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    if ((midi->ticksPerQuarterNote & 0x8000) != 0) {
        MessageBox(NULL, "Unsupported MIDI time division format", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Parse InfoTrack (first track)
    if (!ParseInfoTrack(file, &midi->infoTrack)) {
        return false;
    }
    
    // Set seconds per quarter note from the tempo in the InfoTrack
    midi->secondsPerQuarterNote = midi->infoTrack.setTempo.microsecondsPerQuarterNote / 1000000.0f;
    
    // Parse SoundTracks
    int soundTrackCount = midi->trackCount - 1; // First track is InfoTrack
    
    for (int i = 0; i < soundTrackCount && i < MAX_SOUND_TRACKS; i++) {
        if (!ParseSoundTrack(file, &midi->soundTracks[i])) {
            return false;
        }
    }
    
    return true;
}

// Load AMB file
bool LoadAmbFile(const Path filePath, AmbFile * out) {
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

int ComputePrgmChunkSize(PrgmChunk const * chunk) {
    // size = 28 bytes for 7 4-byte ints + 2 null terminators + lengths of the two strings
    // All Prgm chunks in the AMBs that ship with Civ 3 are confirmed to follow this rule
    return 30 + strlen(chunk->effectName) + strlen(chunk->varName);
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

int ComputeKmapChunkSize(KmapChunk const * chunk) {
    // size w/o items = 20 bytes for 5 4-byte ints + length of var name + null terminator + end indicator
    int size = 25 + strlen(chunk->varName);
    for (int i = 0; i < chunk->itemCount; i++)
        // size of each item = 12 bytes unknown data + length of wav file name + null terminator
        size += 12 + strlen(chunk->items[i].wavFileName) + 1;
    // This rule matches the size of all Kmap chunks in the Civ 3 AMBs except two, both in GalleyAttack.amb. Those chunks are broken, however, and
    // don't play any sound.
    return size;
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

// Function to write TrackName event to MIDI file
void WriteTrackNameEvent(FILE *file, const TrackNameEvent *event) {
    fputc(0x03, file);  // Track name meta event type
    
    // Length of the name
    WriteMidiVarInt(file, strlen(event->name));
    
    // Name data
    fwrite(event->name, 1, strlen(event->name), file);
}

// Function to write SMPTEOffset event to MIDI file
void WriteSMPTEOffsetEvent(FILE *file, const SMPTEOffsetEvent *event) {
    fputc(0x54, file);  // SMPTE offset meta event type
    fputc(0x05, file);  // Length is always 5
    
    // SMPTE data
    fputc(event->hr, file);
    fputc(event->mn, file);
    fputc(event->se, file);
    fputc(event->fr, file);
    fputc(event->ff, file);
}

// Function to write TimeSignature event to MIDI file
void WriteTimeSignatureEvent(FILE *file, const TimeSignatureEvent *event) {
    fputc(0x58, file);  // Time signature meta event type
    fputc(0x04, file);  // Length is always 4
    
    // Time signature data
    fputc(event->nn, file);
    fputc(event->dd, file);
    fputc(event->cc, file);
    fputc(event->bb, file);
}

// Function to write SetTempo event to MIDI file
void WriteSetTempoEvent(FILE *file, const SetTempoEvent *event) {
    fputc(0x51, file);  // Set tempo meta event type
    fputc(0x03, file);  // Length is always 3
    
    // Tempo data (microseconds per quarter note)
    unsigned int tempo = event->microsecondsPerQuarterNote;
    fputc((tempo >> 16) & 0xFF, file);
    fputc((tempo >> 8) & 0xFF, file);
    fputc(tempo & 0xFF, file);
}

// Function to write EndOfTrack event to MIDI file
void WriteEndOfTrackEvent(FILE *file) {
    fputc(0x2F, file);  // End of track meta event type
    fputc(0x00, file);  // Length is always 0
}

// Function to write ControlChange event to MIDI file
void WriteControlChangeEvent(FILE *file, const ControlChangeEvent *event) {
    // Control change status byte: 0xBn, where n is the channel number
    fputc(0xB0 | (event->channelNumber & 0x0F), file);
    
    // Controller number and value
    fputc(event->controllerNumber & 0x7F, file);
    fputc(event->value & 0x7F, file);
}

// Function to write ProgramChange event to MIDI file
void WriteProgramChangeEvent(FILE *file, const ProgramChangeEvent *event) {
    // Program change status byte: 0xCn, where n is the channel number
    fputc(0xC0 | (event->channelNumber & 0x0F), file);
    
    // Program number
    fputc(event->programNumber & 0x7F, file);
}

// Function to write NoteOn event to MIDI file
void WriteNoteOnEvent(FILE *file, const NoteOnEvent *event) {
    // Note on status byte: 0x9n, where n is the channel number
    fputc(0x90 | (event->channelNumber & 0x0F), file);
    
    // Key and velocity
    fputc(event->key & 0x7F, file);
    fputc(event->velocity & 0x7F, file);
}

// Function to write NoteOff event to MIDI file
void WriteNoteOffEvent(FILE *file, const NoteOffEvent *event) {
    // Note off status byte: 0x8n, where n is the channel number
    fputc(0x80 | (event->channelNumber & 0x0F), file);
    
    // Key and velocity
    fputc(event->key & 0x7F, file);
    fputc(event->velocity & 0x7F, file);
}

// Function to write InfoTrack to MIDI file
void WriteInfoTrack(FILE *file, const InfoTrack *track) {
    // Write track tag
    fwrite("MTrk", 1, 4, file);
    
    // We need to calculate track size, so we'll save the current position,
    // write a placeholder, write the events, then come back and update it
    long sizePos = ftell(file);
    
    // Write placeholder for size
    WriteMidiInt(file, 0, true);
    
    // Save position at start of track data
    long startPos = ftell(file);
    
    // Write all events in sequence
    
    // 1. TrackName event
    WriteMidiVarInt(file, track->deltaTimeTrackName);
    fputc(0xFF, file);  // Meta event marker
    WriteTrackNameEvent(file, &track->trackName);
    
    // 2. SMPTEOffset event
    WriteMidiVarInt(file, track->deltaTimeSMPTEOffset);
    fputc(0xFF, file);  // Meta event marker
    WriteSMPTEOffsetEvent(file, &track->smpteOffset);
    
    // 3. TimeSignature event
    WriteMidiVarInt(file, track->deltaTimeTimeSignature);
    fputc(0xFF, file);  // Meta event marker
    WriteTimeSignatureEvent(file, &track->timeSignature);
    
    // 4. SetTempo event
    WriteMidiVarInt(file, track->deltaTimeSetTempo);
    fputc(0xFF, file);  // Meta event marker
    WriteSetTempoEvent(file, &track->setTempo);
    
    // 5. EndOfTrack event
    WriteMidiVarInt(file, track->deltaTimeEndOfTrack);
    fputc(0xFF, file);  // Meta event marker
    WriteEndOfTrackEvent(file);
    
    // Get end position and calculate size
    long endPos = ftell(file);
    unsigned int trackSize = (unsigned int)(endPos - startPos);
    
    // Go back and write the actual size
    fseek(file, sizePos, SEEK_SET);
    WriteMidiInt(file, trackSize, true);
    
    // Return to the end of the track
    fseek(file, endPos, SEEK_SET);
}

// Function to write SoundTrack to MIDI file
unsigned int WriteSoundTrack(FILE *file, const SoundTrack *track) {
    // Write track tag
    fwrite("MTrk", 1, 4, file);
    
    // We need to calculate track size, so we'll save the current position,
    // write a placeholder, write the events, then come back and update it
    long sizePos = ftell(file);
    
    // Write placeholder for size
    WriteMidiInt(file, 0, true);
    
    // Save position at start of track data
    long startPos = ftell(file);
    
    // Write all events in sequence
    
    // 1. TrackName event
    WriteMidiVarInt(file, track->deltaTimeTrackName);
    fputc(0xFF, file);  // Meta event marker
    WriteTrackNameEvent(file, &track->trackName);
    
    // 2. ControlChange events (1 or 2)
    for (int i = 0; i < track->controlChangeCount; i++) {
        WriteMidiVarInt(file, track->deltaTimeControlChanges[i]);
        WriteControlChangeEvent(file, &track->controlChanges[i]);
    }
    
    // 3. ProgramChange event
    WriteMidiVarInt(file, track->deltaTimeProgramChange);
    WriteProgramChangeEvent(file, &track->programChange);
    
    // 4. NoteOn event
    WriteMidiVarInt(file, track->deltaTimeNoteOn);
    WriteNoteOnEvent(file, &track->noteOn);
    
    // 5. NoteOff event
    WriteMidiVarInt(file, track->deltaTimeNoteOff);
    WriteNoteOffEvent(file, &track->noteOff);
    
    // 6. EndOfTrack event
    WriteMidiVarInt(file, track->deltaTimeEndOfTrack);
    fputc(0xFF, file);  // Meta event marker
    WriteEndOfTrackEvent(file);
    
    // Get end position and calculate size
    long endPos = ftell(file);
    unsigned int trackSize = (unsigned int)(endPos - startPos);
    
    // Go back and write the actual size
    fseek(file, sizePos, SEEK_SET);
    WriteMidiInt(file, trackSize, true);
    
    // Return to the end of the track
    fseek(file, endPos, SEEK_SET);

    return trackSize;
}

int ComputeSoundTrackSize(SoundTrack const * track) {
    FILE *memFile = tmpfile();
    unsigned int size = WriteSoundTrack(memFile, track);
    fclose(memFile);
    return size;
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
    
    // Write InfoTrack
    WriteInfoTrack(file, &midi->infoTrack);
    
    // Write SoundTracks
    int soundTrackCount = midi->trackCount - 1; // First track is InfoTrack
    
    for (int i = 0; i < soundTrackCount && i < MAX_SOUND_TRACKS; i++) {
        WriteSoundTrack(file, &midi->soundTracks[i]);
    }
    
    return true;
}

// Function to save AMB file to the specified path
bool SaveAmbFile(AmbFile const * amb, const Path filePath) {
    FILE *file = fopen(filePath, "wb");
    if (!file) {
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
    if (!success) {
        remove(filePath);
    }
    
    return success;
}

// Utility function to get sound track count
int GetSoundTrackCount(const MidiData *midi) {
    return midi->trackCount - 1; // First track is InfoTrack
}

// Function to describe the loaded AMB file (for debugging)
void DescribeAmbFile(AmbFile const * amb, char *buffer, size_t bufferSize) {
    char *pos = buffer;
    int remainingSize = (int)bufferSize;
    
    // Describe file path
    int written = snprintf(pos, remainingSize, "File: %s\r\n\r\n", amb->filePath);
    pos += written;
    remainingSize -= written;
    
    // Describe Prgm chunks
    for (int i = 0; i < amb->prgmChunkCount && remainingSize > 0; i++) {
        PrgmChunk *chunk = &amb->prgmChunks[i];
        written = snprintf(pos, remainingSize, 
                    "Prgm #%d:\r\n"
                    "  Size: %d\r\n"
                    "  Number: %d\r\n"
                    "  Flags: %d\r\n"
                    "  Max Random Speed: %d\r\n"
                    "  Min Random Speed: %d\r\n"
                    "  Max Random Volume: %d\r\n"
                    "  Min Random Volume: %d\r\n"
                    "  Effect Name: '%s'\r\n"
                    "  Var Name: '%s'\r\n\r\n",
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
                    "Kmap #%d:\r\n"
                    "  Size: %d\r\n"
                    "  Flags: %d\r\n"
                    "  Param1: %d\r\n"
                    "  Param2: %d\r\n"
                    "  Var Name: '%s'\r\n"
                    "  Item Count: %d\r\n"
                    "  Item Size: %d\r\n",
                    i + 1, chunk->size, chunk->flags,
                    chunk->param1, chunk->param2,
                    chunk->varName, chunk->itemCount, chunk->itemSize);
        pos += written;
        remainingSize -= written;
        
        // Describe Kmap items
        for (int j = 0; j < chunk->itemCount && remainingSize > 0; j++) {
            written = snprintf(pos, remainingSize, 
                        "    Item #%d: WAV File: '%s'\r\n",
                        j + 1, chunk->items[j].wavFileName);
            pos += written;
            remainingSize -= written;
        }
        
        written = snprintf(pos, remainingSize, "\r\n");
        pos += written;
        remainingSize -= written;
    }
    
    // Describe Glbl chunk
    if (amb->hasGlblChunk && remainingSize > 0) {
        GlblChunk const * chunk = &amb->glblChunk;
        written = snprintf(pos, remainingSize, 
                    "Glbl:\r\n"
                    "  Size: %d\r\n"
                    "  Data Size: %d\r\n\r\n",
                    chunk->size, chunk->dataSize);
        pos += written;
        remainingSize -= written;
    }
    
    // Describe MIDI data
    if (remainingSize > 0) {
        written = snprintf(pos, remainingSize, 
                    "MIDI:\r\n"
                    "  Format: %d\r\n"
                    "  Track Count: %d\r\n"
                    "  Ticks Per Quarter Note: %d\r\n"
                    "  Seconds Per Quarter Note: %.6f\r\n\r\n",
                    amb->midi.format, amb->midi.trackCount,
                    amb->midi.ticksPerQuarterNote, amb->midi.secondsPerQuarterNote);
        pos += written;
        remainingSize -= written;
        
        // Describe InfoTrack
        if (remainingSize > 0) {
            InfoTrack const *infoTrack = &amb->midi.infoTrack;
            float secondsPerTick = amb->midi.secondsPerQuarterNote / amb->midi.ticksPerQuarterNote;
            float timestamp = 0.0f;
            
            written = snprintf(pos, remainingSize, 
                        "  InfoTrack:\r\n"
                        "    Size: %d\r\n",
                        infoTrack->size);
            pos += written;
            remainingSize -= written;
            
            // Track name event
            timestamp += infoTrack->deltaTimeTrackName * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: TrackName '%s'\r\n",
                        timestamp, infoTrack->trackName.name);
            pos += written;
            remainingSize -= written;
            
            // SMPTE offset event
            timestamp += infoTrack->deltaTimeSMPTEOffset * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: SMPTEOffset %d %d %d %d %d\r\n",
                        timestamp, infoTrack->smpteOffset.hr, 
                        infoTrack->smpteOffset.mn, 
                        infoTrack->smpteOffset.se, 
                        infoTrack->smpteOffset.fr, 
                        infoTrack->smpteOffset.ff);
            pos += written;
            remainingSize -= written;
            
            // Time signature event
            timestamp += infoTrack->deltaTimeTimeSignature * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: TimeSignature %d %d %d %d\r\n",
                        timestamp, infoTrack->timeSignature.nn, 
                        infoTrack->timeSignature.dd, 
                        infoTrack->timeSignature.cc, 
                        infoTrack->timeSignature.bb);
            pos += written;
            remainingSize -= written;
            
            // Set tempo event
            timestamp += infoTrack->deltaTimeSetTempo * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: SetTempo %d\r\n",
                        timestamp, infoTrack->setTempo.microsecondsPerQuarterNote);
            pos += written;
            remainingSize -= written;
            
            // End of track event
            timestamp += infoTrack->deltaTimeEndOfTrack * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: EndOfTrack\r\n\r\n",
                        timestamp);
            pos += written;
            remainingSize -= written;
        }
        
        // Describe each SoundTrack
        int soundTrackCount = GetSoundTrackCount(&amb->midi);
        for (int i = 0; i < soundTrackCount && remainingSize > 0; i++) {
            SoundTrack const *soundTrack = &amb->midi.soundTracks[i];
            float secondsPerTick = amb->midi.secondsPerQuarterNote / amb->midi.ticksPerQuarterNote;
            float timestamp = 0.0f;
            
            written = snprintf(pos, remainingSize, 
                        "  SoundTrack #%d:\r\n"
                        "    Size: %d\r\n",
                        i + 1, soundTrack->size);
            pos += written;
            remainingSize -= written;
            
            // Track name event
            timestamp += soundTrack->deltaTimeTrackName * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: TrackName '%s'\r\n",
                        timestamp, soundTrack->trackName.name);
            pos += written;
            remainingSize -= written;
            
            // Control change events
            for (int j = 0; j < soundTrack->controlChangeCount && remainingSize > 0; j++) {
                timestamp += soundTrack->deltaTimeControlChanges[j] * secondsPerTick;
                written = snprintf(pos, remainingSize, 
                            "    %.3f: ControlChange %d %d %d\r\n",
                            timestamp, soundTrack->controlChanges[j].channelNumber,
                            soundTrack->controlChanges[j].controllerNumber,
                            soundTrack->controlChanges[j].value);
                pos += written;
                remainingSize -= written;
            }
            
            // Program change event
            timestamp += soundTrack->deltaTimeProgramChange * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: ProgramChange %d %d\r\n",
                        timestamp, soundTrack->programChange.channelNumber,
                        soundTrack->programChange.programNumber);
            pos += written;
            remainingSize -= written;
            
            // Note on event
            timestamp += soundTrack->deltaTimeNoteOn * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: NoteOn %d %d %d\r\n",
                        timestamp, soundTrack->noteOn.channelNumber,
                        soundTrack->noteOn.key,
                        soundTrack->noteOn.velocity);
            pos += written;
            remainingSize -= written;
            
            // Note off event
            timestamp += soundTrack->deltaTimeNoteOff * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: NoteOff %d %d %d\r\n",
                        timestamp, soundTrack->noteOff.channelNumber,
                        soundTrack->noteOff.key,
                        soundTrack->noteOff.velocity);
            pos += written;
            remainingSize -= written;
            
            // End of track event
            timestamp += soundTrack->deltaTimeEndOfTrack * secondsPerTick;
            written = snprintf(pos, remainingSize, 
                        "    %.3f: EndOfTrack\r\n\r\n",
                        timestamp);
            pos += written;
            remainingSize -= written;
        }
    }
}
