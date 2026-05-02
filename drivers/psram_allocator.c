#include "psram_allocator.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// When using --wrap=malloc, call the real libc functions to avoid recursion
extern void *__real_malloc(size_t size);
extern void *__real_realloc(void *ptr, size_t size);
extern void __real_free(void *ptr);

// PSRAM is mapped at 0x11000000 on RP2350 (CS1 via XIP).
#define PSRAM_BASE 0x11000000
#define PSRAM_SIZE ((size_t)CABAL_PSRAM_SIZE_BYTES)

static uint8_t *psram_start = (uint8_t *)PSRAM_BASE;

// Reserve 512KB for scratch buffers at the beginning
//   0-128KB : scratch_1
// 128-256KB : scratch_2
// 256-512KB : file buffer
#define SCRATCH_SIZE (512 * 1024)

// Heap grows linearly (bump allocator) after the scratch region.
// Optional temp sub-region at the tail is not used here (TEMP_SIZE=0);
// kept around so call sites that still reference psram_temp_* keep working.
#define TEMP_SIZE 0
#define HEAP_SIZE (PSRAM_SIZE - SCRATCH_SIZE - TEMP_SIZE)

// Per-allocation header: size (in bytes) of the payload. Needed for realloc.
// Stored in SRAM-accessible-but-physically-in-PSRAM memory — but because
// writes here are write-once and we never re-read them through the cache
// after a subsequent allocation, they aren't vulnerable to the XIP cache
// coherency issue we see when free-list metadata is updated/read repeatedly.
// Layout matches murmsnes.
static size_t psram_offset = 0;      // bump pointer (bytes) within heap
static size_t psram_temp_offset = 0; // unused: TEMP_SIZE == 0

static int psram_temp_mode = 0;
static int psram_sram_mode = 0;

void psram_set_temp_mode(int enable) {
    psram_temp_mode = enable;
}

void psram_set_sram_mode(int enable) {
    psram_sram_mode = enable;
}

void psram_reset_temp(void) {
    psram_temp_offset = 0;
}

size_t psram_get_temp_offset(void) {
    return psram_temp_offset;
}

void psram_set_temp_offset(size_t offset) {
    psram_temp_offset = offset;
}

// Flag set by main() after PSRAM is confirmed working
static int psram_ready = 0;

void psram_set_ready(int ready) {
    psram_ready = ready;
}

static inline uint8_t *heap_base(void) {
    return psram_start + SCRATCH_SIZE;
}

void *psram_malloc(size_t size) {
    // Before PSRAM is ready, use SRAM (call real libc malloc to avoid wrap recursion)
    if (!psram_ready || psram_sram_mode) {
        return __real_malloc(size);
    }

    // Align size to 8 bytes, minimum 8
    size = (size + 7) & ~7u;
    if (size < 8) size = 8;

    // Each allocation: [size_t size | payload...]. Header lets realloc work.
    size_t total = size + sizeof(size_t);
    total = (total + 7) & ~7u;

    if (psram_offset + total > HEAP_SIZE) {
        printf("PSRAM Heap OOM! Req %d, free %d\n",
               (int)size, (int)(HEAP_SIZE - psram_offset));
        return NULL;
    }

    size_t *header = (size_t *)(heap_base() + psram_offset);
    *header = size;
    void *ptr = (void *)(header + 1);
    psram_offset += total;
    return ptr;
}

void *psram_realloc(void *ptr, size_t new_size) {
    if (ptr == NULL) return psram_malloc(new_size);
    if (new_size == 0) { psram_free(ptr); return NULL; }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_lo = PSRAM_BASE + SCRATCH_SIZE;
    uintptr_t heap_hi = heap_lo + HEAP_SIZE;

    if (addr >= heap_lo && addr < heap_hi) {
        size_t *header = (size_t *)ptr - 1;
        size_t old_size = *header;

        if (new_size <= old_size) {
            return ptr;
        }

        void *new_ptr = psram_malloc(new_size);
        if (new_ptr) {
            memcpy(new_ptr, ptr, old_size);
            // psram_free is a no-op for bump allocations.
        }
        return new_ptr;
    }

    return __real_realloc(ptr, new_size);
}

void psram_free(void *ptr) {
    if (!ptr) return;

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_lo = PSRAM_BASE + SCRATCH_SIZE;
    uintptr_t heap_hi = heap_lo + HEAP_SIZE;

    if (addr >= heap_lo && addr < heap_hi) {
        // Bump allocator — no-op.
        return;
    }

    // Not in PSRAM, assume regular libc malloc (avoid wrap recursion)
    __real_free(ptr);
}

void *psram_get_scratch_1(size_t size) {
    if (size > 128 * 1024) return NULL;
    return psram_start;
}

void *psram_get_scratch_2(size_t size) {
    if (size > 128 * 1024) return NULL;
    return psram_start + (128 * 1024);
}

void *psram_get_file_buffer(size_t size) {
    if (size > 256 * 1024) {
        printf("PSRAM File Buffer too small! Req: %d\n", (int)size);
        return NULL;
    }
    return psram_start + (256 * 1024);
}

void psram_reset(void) {
    psram_offset = 0;
    psram_temp_offset = 0;
}

void psram_mark_session(void) {
    printf("PSRAM: mark_session not supported (bump allocator)\n");
}

void psram_restore_session(void) {
    printf("PSRAM: restore_session not supported (bump allocator)\n");
}

void psram_print_status(void) {
    printf("PSRAM Status:\n");
    printf("  Heap: %lu used / %lu total (%lu free)\n",
           (unsigned long)psram_offset,
           (unsigned long)HEAP_SIZE,
           (unsigned long)(HEAP_SIZE - psram_offset));
    printf("  Temp: %d/%d bytes\n",
           (int)psram_temp_offset, (int)TEMP_SIZE);
}
