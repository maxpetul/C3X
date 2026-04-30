#include <windows.h>
#include <stdbool.h>

// WAV file header structures
typedef struct {
    char riff[4];           // "RIFF"
    DWORD fileSize;         // File size - 8
    char wave[4];           // "WAVE"
} RiffHeader;

typedef struct {
    char fmt[4];            // "fmt "
    DWORD chunkSize;        // Size of fmt chunk data
    WORD audioFormat;       // 1 = PCM
    WORD numChannels;       // 1 = mono, 2 = stereo
    DWORD sampleRate;       // Sample rate (Hz)
    DWORD byteRate;         // Bytes per second
    WORD blockAlign;        // Bytes per sample frame
    WORD bitsPerSample;     // Bits per sample
} FmtChunk;

typedef struct {
    char data[4];           // "data"
    DWORD dataSize;         // Size of audio data
} DataChunkHeader;

// Read WAV file duration
// Returns TRUE on success, FALSE on failure
// Duration is returned in seconds through outDuration parameter
BOOL GetWavFileDuration(const char* filePath, float* outDuration)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    RiffHeader riffHeader;
    FmtChunk fmtChunk;
    DataChunkHeader dataHeader;
    DWORD bytesRead;
    BOOL success = FALSE;
    
    if (!filePath || !outDuration) {
        return FALSE;
    }
    
    // Open the file
    hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, 
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    // Read RIFF header
    if (!ReadFile(hFile, &riffHeader, sizeof(RiffHeader), &bytesRead, NULL) ||
        bytesRead != sizeof(RiffHeader)) {
        goto cleanup;
    }
    
    // Verify RIFF and WAVE signatures
    if (memcmp(riffHeader.riff, "RIFF", 4) != 0 ||
        memcmp(riffHeader.wave, "WAVE", 4) != 0) {
        goto cleanup;
    }
    
    // Read fmt chunk
    if (!ReadFile(hFile, &fmtChunk, sizeof(FmtChunk), &bytesRead, NULL) ||
        bytesRead != sizeof(FmtChunk)) {
        goto cleanup;
    }
    
    // Verify fmt signature and basic PCM format
    if (memcmp(fmtChunk.fmt, "fmt ", 4) != 0 ||
        fmtChunk.audioFormat != 1 ||  // PCM format
        fmtChunk.numChannels == 0 ||
        fmtChunk.sampleRate == 0 ||
        fmtChunk.bitsPerSample == 0) {
        goto cleanup;
    }
    
    // Skip any extra fmt chunk data beyond the standard 16 bytes
    if (fmtChunk.chunkSize > 16) {
        DWORD extraBytes = fmtChunk.chunkSize - 16;
        SetFilePointer(hFile, extraBytes, NULL, FILE_CURRENT);
    }
    
    // Look for data chunk
    char chunkId[4];
    DWORD chunkSize;
    BOOL foundData = FALSE;
    
    while (!foundData) {
        // Read chunk header (ID + size)
        if (!ReadFile(hFile, chunkId, 4, &bytesRead, NULL) || bytesRead != 4) {
            goto cleanup;
        }
        if (!ReadFile(hFile, &chunkSize, 4, &bytesRead, NULL) || bytesRead != 4) {
            goto cleanup;
        }
        
        if (memcmp(chunkId, "data", 4) == 0) {
            // Found data chunk
            dataHeader.dataSize = chunkSize;
            foundData = TRUE;
        } else {
            // Skip this chunk
            if (SetFilePointer(hFile, chunkSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER) {
                goto cleanup;
            }
        }
    }
    
    if (!foundData || dataHeader.dataSize == 0) {
        goto cleanup;
    }
    
    // Calculate duration: data_size / (sample_rate * channels * bytes_per_sample)
    DWORD bytesPerSample = (fmtChunk.bitsPerSample + 7) / 8;  // Round up to nearest byte
    DWORD bytesPerSecond = fmtChunk.sampleRate * fmtChunk.numChannels * bytesPerSample;
    
    if (bytesPerSecond == 0) {
        goto cleanup;
    }
    
    *outDuration = (float)dataHeader.dataSize / (float)bytesPerSecond;
    success = TRUE;
    
cleanup:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    
    return success;
}