/* Cabal - Legacy Game Implementations
 *
 * Filesystem API for SD Card access
 */

#ifndef CABAL_FS_H
#define CABAL_FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
typedef enum {
    CABAL_FS_OK = 0,
    CABAL_FS_ERROR,
    CABAL_FS_NOT_FOUND,
    CABAL_FS_NOT_READY,
    CABAL_FS_NO_MEMORY,
    CABAL_FS_INVALID_PARAM
} CabalFsResult;

// File handle (opaque)
typedef struct CabalFile CabalFile;

// Directory entry
typedef struct {
    char name[256];
    uint32_t size;
    bool isDirectory;
} CabalDirEntry;

// Directory handle (opaque)
typedef struct CabalDir CabalDir;

//============================================================================
// Initialization
//============================================================================

// Initialize filesystem (mounts SD card)
CabalFsResult cabal_fs_init(void);

// Check if filesystem is ready
bool cabal_fs_ready(void);

// Get free space in bytes
uint32_t cabal_fs_free_space(void);

//============================================================================
// File Operations
//============================================================================

// Open file for reading
CabalFile *cabal_file_open(const char *path);

// Open file for writing (creates file if it doesn't exist, truncates if it does)
CabalFile *cabal_file_open_write(const char *path);

// Close file
void cabal_file_close(CabalFile *file);

// Read from file
// Returns number of bytes read, or -1 on error
int32_t cabal_file_read(CabalFile *file, void *buffer, uint32_t size);

// Write to file
// Returns number of bytes written, or -1 on error
int32_t cabal_file_write(CabalFile *file, const void *buffer, uint32_t size);

// Flush file buffers to disk
CabalFsResult cabal_file_flush(CabalFile *file);

// Read entire file into newly allocated buffer
// Caller must free the returned buffer with cabal_fs_free()
void *cabal_file_read_all(const char *path, uint32_t *size_out);

// Get file size
uint32_t cabal_file_size(CabalFile *file);

// Get current position
uint32_t cabal_file_tell(CabalFile *file);

// Seek to position
CabalFsResult cabal_file_seek(CabalFile *file, uint32_t offset);

// Check if at end of file
bool cabal_file_eof(CabalFile *file);

//============================================================================
// Directory Operations
//============================================================================

// Open directory for listing
CabalDir *cabal_dir_open(const char *path);

// Close directory
void cabal_dir_close(CabalDir *dir);

// Read next directory entry
// Returns false when no more entries
bool cabal_dir_read(CabalDir *dir, CabalDirEntry *entry);

// Check if path exists
bool cabal_path_exists(const char *path);

// Check if path is a directory
bool cabal_path_is_dir(const char *path);

// Create directory (and parent directories if needed)
CabalFsResult cabal_mkdir(const char *path);

// Remove file
CabalFsResult cabal_remove(const char *path);

//============================================================================
// Memory Management
//============================================================================

// Free memory allocated by cabal_file_read_all
void cabal_fs_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // CABAL_FS_H
