/* Cabal - Legacy Game Implementations
 *
 * Filesystem Implementation using FatFS
 */

#include "cabal_fs.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// External PSRAM allocation
extern void *psram_malloc(size_t size);
extern void psram_free(void *ptr);

// External audio processing (to prevent audio freeze during file I/O)
#ifdef USE_I2S_AUDIO
extern void cabal_audio_process_frame(void);
#endif

//============================================================================
// Internal State
//============================================================================

static FATFS g_fatfs;
static bool g_initialized = false;

// File handle wrapper
struct CabalFile {
    FIL fil;
    bool valid;
};

// Directory handle wrapper
struct CabalDir {
    DIR dir;
    bool valid;
};

// Pool of file handles (avoid malloc for small allocations)
#define MAX_OPEN_FILES 8
static struct CabalFile g_files[MAX_OPEN_FILES];

#define MAX_OPEN_DIRS 4
static struct CabalDir g_dirs[MAX_OPEN_DIRS];

//============================================================================
// Initialization
//============================================================================

CabalFsResult cabal_fs_init(void) {
    if (g_initialized) {
        return CABAL_FS_OK;
    }

    printf("Cabal FS: Initializing SD card...\n");

    // Initialize the disk
    DSTATUS stat = disk_initialize(0);
    if (stat & STA_NOINIT) {
        printf("Cabal FS: Failed to initialize SD card (status=%d)\n", stat);
        return CABAL_FS_NOT_READY;
    }

    printf("Cabal FS: SD card initialized, mounting filesystem...\n");

    // Mount the filesystem
    FRESULT res = f_mount(&g_fatfs, "0:", 1);
    if (res != FR_OK) {
        printf("Cabal FS: Failed to mount filesystem (error=%d)\n", res);
        return CABAL_FS_ERROR;
    }

    // Initialize file handle pool
    memset(g_files, 0, sizeof(g_files));
    memset(g_dirs, 0, sizeof(g_dirs));

    g_initialized = true;

    // Print filesystem info
    DWORD free_clusters;
    FATFS *fs;
    res = f_getfree("0:", &free_clusters, &fs);
    if (res == FR_OK) {
        uint32_t free_kb = (free_clusters * fs->csize) / 2;
        printf("Cabal FS: Mounted successfully, %lu KB free\n", (unsigned long)free_kb);
    } else {
        printf("Cabal FS: Mounted successfully\n");
    }

    return CABAL_FS_OK;
}

bool cabal_fs_ready(void) {
    return g_initialized;
}

uint32_t cabal_fs_free_space(void) {
    if (!g_initialized) return 0;

    DWORD free_clusters;
    FATFS *fs;
    FRESULT res = f_getfree("0:", &free_clusters, &fs);
    if (res != FR_OK) return 0;

    // Return bytes
    return free_clusters * fs->csize * 512;
}

//============================================================================
// File Operations
//============================================================================

static struct CabalFile *alloc_file_handle(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!g_files[i].valid) {
            memset(&g_files[i], 0, sizeof(struct CabalFile));
            return &g_files[i];
        }
    }
    return NULL;
}

// Helper to build full path
static void build_fullpath(char *fullpath, size_t size, const char *path) {
    if (path[0] == '/' || path[0] == '0') {
        if (path[0] == '/') {
            snprintf(fullpath, size, "0:%s", path);
        } else {
            strncpy(fullpath, path, size - 1);
            fullpath[size - 1] = '\0';
        }
    } else {
        snprintf(fullpath, size, "0:/%s", path);
    }
}

CabalFile *cabal_file_open(const char *path) {
    if (!g_initialized || !path) return NULL;

    struct CabalFile *file = alloc_file_handle();
    if (!file) {
        printf("Cabal FS: No free file handles\n");
        return NULL;
    }

    char fullpath[512];
    build_fullpath(fullpath, sizeof(fullpath), path);

    FRESULT res = f_open(&file->fil, fullpath, FA_READ);
    if (res != FR_OK) {
        // Don't print error for common "file not found" case
        if (res != FR_NO_FILE && res != FR_NO_PATH) {
            printf("Cabal FS: Failed to open '%s' (error=%d)\n", fullpath, res);
        }
        return NULL;
    }

    file->valid = true;
    return file;
}

CabalFile *cabal_file_open_write(const char *path) {
    if (!g_initialized || !path) return NULL;

    struct CabalFile *file = alloc_file_handle();
    if (!file) {
        printf("Cabal FS: No free file handles\n");
        return NULL;
    }

    char fullpath[512];
    build_fullpath(fullpath, sizeof(fullpath), path);

    // Create or truncate file for writing
    FRESULT res = f_open(&file->fil, fullpath, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        printf("Cabal FS: Failed to create '%s' (error=%d)\n", fullpath, res);
        return NULL;
    }

    file->valid = true;
    return file;
}

void cabal_file_close(CabalFile *file) {
    if (!file || !file->valid) return;

    f_close(&file->fil);
    file->valid = false;
}

int32_t cabal_file_read(CabalFile *file, void *buffer, uint32_t size) {
    if (!file || !file->valid || !buffer) return -1;

#ifdef USE_I2S_AUDIO
    // For large reads, chunk the operation and process audio between chunks
    // to prevent audio underruns during file I/O
    // Use 4KB chunks for more frequent audio processing
    if (size > 4096) {
        uint8_t *dst = (uint8_t *)buffer;
        uint32_t total_read = 0;
        while (size > 0) {
            uint32_t chunk = (size > 4096) ? 4096 : size;
            UINT bytes_read;
            FRESULT res = f_read(&file->fil, dst, chunk, &bytes_read);
            if (res != FR_OK) {
                return (total_read > 0) ? (int32_t)total_read : -1;
            }
            total_read += bytes_read;
            dst += bytes_read;
            size -= bytes_read;
            if (bytes_read < chunk) break;  // EOF
            cabal_audio_process_frame();
        }
        return (int32_t)total_read;
    }
#endif

    UINT bytes_read;
    FRESULT res = f_read(&file->fil, buffer, size, &bytes_read);
    if (res != FR_OK) {
        return -1;
    }

    return (int32_t)bytes_read;
}

int32_t cabal_file_write(CabalFile *file, const void *buffer, uint32_t size) {
    if (!file || !file->valid || !buffer) return -1;

#ifdef USE_I2S_AUDIO
    // For large writes, chunk the operation and process audio between chunks
    if (size > 4096) {
        const uint8_t *src = (const uint8_t *)buffer;
        uint32_t total_written = 0;
        while (size > 0) {
            uint32_t chunk = (size > 4096) ? 4096 : size;
            UINT bytes_written;
            FRESULT res = f_write(&file->fil, src, chunk, &bytes_written);
            if (res != FR_OK) {
                return (total_written > 0) ? (int32_t)total_written : -1;
            }
            total_written += bytes_written;
            src += bytes_written;
            size -= bytes_written;
            if (bytes_written < chunk) break;  // Disk full?
            cabal_audio_process_frame();
        }
        return (int32_t)total_written;
    }
#endif

    UINT bytes_written;
    FRESULT res = f_write(&file->fil, buffer, size, &bytes_written);
    if (res != FR_OK) {
        return -1;
    }

    return (int32_t)bytes_written;
}

CabalFsResult cabal_file_flush(CabalFile *file) {
    if (!file || !file->valid) return CABAL_FS_INVALID_PARAM;

    FRESULT res = f_sync(&file->fil);
    return (res == FR_OK) ? CABAL_FS_OK : CABAL_FS_ERROR;
}

void *cabal_file_read_all(const char *path, uint32_t *size_out) {
    if (size_out) *size_out = 0;

    CabalFile *file = cabal_file_open(path);
    if (!file) return NULL;

    uint32_t size = cabal_file_size(file);
    if (size == 0) {
        cabal_file_close(file);
        return NULL;
    }

    // Allocate from PSRAM
    void *buffer = psram_malloc(size);
    if (!buffer) {
        printf("Cabal FS: Failed to allocate %lu bytes for '%s'\n",
               (unsigned long)size, path);
        cabal_file_close(file);
        return NULL;
    }

    int32_t bytes_read = cabal_file_read(file, buffer, size);
    cabal_file_close(file);

    if (bytes_read < 0 || (uint32_t)bytes_read != size) {
        printf("Cabal FS: Read error for '%s'\n", path);
        // Can't free PSRAM with bump allocator, but mark as error
        return NULL;
    }

    if (size_out) *size_out = size;
    return buffer;
}

uint32_t cabal_file_size(CabalFile *file) {
    if (!file || !file->valid) return 0;
    return f_size(&file->fil);
}

uint32_t cabal_file_tell(CabalFile *file) {
    if (!file || !file->valid) return 0;
    return f_tell(&file->fil);
}

CabalFsResult cabal_file_seek(CabalFile *file, uint32_t offset) {
    if (!file || !file->valid) return CABAL_FS_INVALID_PARAM;

    FRESULT res = f_lseek(&file->fil, offset);
    return (res == FR_OK) ? CABAL_FS_OK : CABAL_FS_ERROR;
}

bool cabal_file_eof(CabalFile *file) {
    if (!file || !file->valid) return true;
    return f_eof(&file->fil) != 0;
}

//============================================================================
// Directory Operations
//============================================================================

static struct CabalDir *alloc_dir_handle(void) {
    for (int i = 0; i < MAX_OPEN_DIRS; i++) {
        if (!g_dirs[i].valid) {
            memset(&g_dirs[i], 0, sizeof(struct CabalDir));
            return &g_dirs[i];
        }
    }
    return NULL;
}

CabalDir *cabal_dir_open(const char *path) {
    if (!g_initialized || !path) return NULL;

    struct CabalDir *dir = alloc_dir_handle();
    if (!dir) {
        printf("Cabal FS: No free directory handles\n");
        return NULL;
    }

    // Build full path
    char fullpath[512];
    if (path[0] == '/' || path[0] == '0') {
        if (path[0] == '/') {
            snprintf(fullpath, sizeof(fullpath), "0:%s", path);
        } else {
            strncpy(fullpath, path, sizeof(fullpath) - 1);
        }
    } else {
        snprintf(fullpath, sizeof(fullpath), "0:/%s", path);
    }

    FRESULT res = f_opendir(&dir->dir, fullpath);
    if (res != FR_OK) {
        return NULL;
    }

    dir->valid = true;
    return dir;
}

void cabal_dir_close(CabalDir *dir) {
    if (!dir || !dir->valid) return;

    f_closedir(&dir->dir);
    dir->valid = false;
}

bool cabal_dir_read(CabalDir *dir, CabalDirEntry *entry) {
    if (!dir || !dir->valid || !entry) return false;

    FILINFO fno;
    FRESULT res = f_readdir(&dir->dir, &fno);
    if (res != FR_OK || fno.fname[0] == 0) {
        return false;
    }

    strncpy(entry->name, fno.fname, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->size = fno.fsize;
    entry->isDirectory = (fno.fattrib & AM_DIR) != 0;

    return true;
}

bool cabal_path_exists(const char *path) {
    if (!g_initialized || !path) return false;

    char fullpath[512];
    build_fullpath(fullpath, sizeof(fullpath), path);

    FILINFO fno;
    return f_stat(fullpath, &fno) == FR_OK;
}

bool cabal_path_is_dir(const char *path) {
    if (!g_initialized || !path) return false;

    char fullpath[512];
    build_fullpath(fullpath, sizeof(fullpath), path);

    FILINFO fno;
    if (f_stat(fullpath, &fno) != FR_OK) return false;
    return (fno.fattrib & AM_DIR) != 0;
}

CabalFsResult cabal_mkdir(const char *path) {
    if (!g_initialized || !path) return CABAL_FS_INVALID_PARAM;

    char fullpath[512];
    build_fullpath(fullpath, sizeof(fullpath), path);

    FRESULT res = f_mkdir(fullpath);
    if (res == FR_OK || res == FR_EXIST) {
        return CABAL_FS_OK;
    }
    return CABAL_FS_ERROR;
}

CabalFsResult cabal_remove(const char *path) {
    if (!g_initialized || !path) return CABAL_FS_INVALID_PARAM;

    char fullpath[512];
    build_fullpath(fullpath, sizeof(fullpath), path);

    FRESULT res = f_unlink(fullpath);
    return (res == FR_OK) ? CABAL_FS_OK : CABAL_FS_ERROR;
}

//============================================================================
// Memory Management
//============================================================================

void cabal_fs_free(void *ptr) {
    // Note: PSRAM uses a bump allocator, can't actually free
    // This is just for API completeness
    (void)ptr;
}
