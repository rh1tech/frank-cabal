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

// PSRAM is mapped at 0x11000000 on RP2350
#define PSRAM_BASE 0x11000000
#define PSRAM_SIZE ((size_t)CABAL_PSRAM_SIZE_BYTES)

static uint8_t *psram_start = (uint8_t *)PSRAM_BASE;

// Reserve 512KB for scratch buffers at the beginning
#define SCRATCH_SIZE (512 * 1024)

// Memory layout: [Scratch 512KB][Heap ~7.5MB]
#define TEMP_SIZE 0
#define HEAP_SIZE (PSRAM_SIZE - SCRATCH_SIZE - TEMP_SIZE)

// Free-list allocator for the heap region
// Block header: [size:31 bits][free:1 bit]
typedef struct block_header {
    uint32_t size_and_free;  // Lower 31 bits = size, bit 0 = free flag
    struct block_header *next_free;  // Only used when block is free
} block_header_t;

#define BLOCK_SIZE(h) ((h)->size_and_free & ~1u)
#define BLOCK_FREE(h) ((h)->size_and_free & 1u)
#define MIN_BLOCK_SIZE 16  // Minimum allocation (header + 8 bytes data)
#define HEADER_SIZE sizeof(block_header_t)

static block_header_t *free_list = NULL;
static int heap_initialized = 0;

// Temp allocator (bump allocator, reset between operations)
static size_t psram_temp_offset = 0;
static int psram_temp_mode = 0;
static int psram_sram_mode = 0;

static void heap_init(void) {
    if (heap_initialized) return;

    // Initialize heap with one big free block
    uint8_t *heap_start = psram_start + SCRATCH_SIZE;
    block_header_t *initial = (block_header_t *)heap_start;
    initial->size_and_free = (HEAP_SIZE & ~1u) | 1;  // Size with free bit set
    initial->next_free = NULL;
    free_list = initial;
    heap_initialized = 1;
    // Note: Don't printf here - may cause recursion during early init
}

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

void *psram_malloc(size_t size) {
    // Before PSRAM is ready, use SRAM (call real libc malloc to avoid wrap recursion)
    if (!psram_ready || psram_sram_mode) {
        return __real_malloc(size);
    }

    // Align size to 8 bytes, ensure minimum
    size = (size + 7) & ~7u;
    if (size < 8) size = 8;

    size_t total_size = size + HEADER_SIZE;
    if (total_size < MIN_BLOCK_SIZE) total_size = MIN_BLOCK_SIZE;

    if (psram_temp_mode) {
        // Temp mode: bump allocator in temp region
        if (psram_temp_offset + total_size > TEMP_SIZE) {
            printf("PSRAM Temp OOM! Req %d, free %d\n",
                   (int)size, (int)(TEMP_SIZE - psram_temp_offset));
            return NULL;
        }
        uint8_t *ptr = psram_start + SCRATCH_SIZE + HEAP_SIZE + psram_temp_offset;
        psram_temp_offset += total_size;
        return ptr;
    }

    // Regular mode: free-list allocator
    heap_init();

    // First-fit search
    block_header_t *prev = NULL;
    block_header_t *curr = free_list;

    while (curr) {
        uint32_t block_size = BLOCK_SIZE(curr);
        if (block_size >= total_size) {
            // Found a fit
            // Split if remainder is large enough
            if (block_size >= total_size + MIN_BLOCK_SIZE + 32) {
                // Split block
                block_header_t *new_block = (block_header_t *)((uint8_t *)curr + total_size);
                new_block->size_and_free = ((block_size - total_size) & ~1u) | 1;
                new_block->next_free = curr->next_free;

                curr->size_and_free = (total_size & ~1u);  // Mark allocated (free bit = 0)

                // Update free list
                if (prev) {
                    prev->next_free = new_block;
                } else {
                    free_list = new_block;
                }
            } else {
                // Use whole block
                curr->size_and_free &= ~1u;  // Mark allocated

                // Remove from free list
                if (prev) {
                    prev->next_free = curr->next_free;
                } else {
                    free_list = curr->next_free;
                }
            }

            return (void *)((uint8_t *)curr + HEADER_SIZE);
        }
        prev = curr;
        curr = curr->next_free;
    }

    // No fit found
    printf("PSRAM Heap OOM! Req %d\n", (int)size);
    return NULL;
}

void *psram_realloc(void *ptr, size_t new_size) {
    if (ptr == NULL) return psram_malloc(new_size);
    if (new_size == 0) { psram_free(ptr); return NULL; }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_start = PSRAM_BASE + SCRATCH_SIZE;
    uintptr_t heap_end = heap_start + HEAP_SIZE;

    if (addr >= heap_start && addr < heap_end) {
        block_header_t *header = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);
        size_t old_size = BLOCK_SIZE(header) - HEADER_SIZE;

        if (new_size <= old_size) {
            return ptr;
        }

        void *new_ptr = psram_malloc(new_size);
        if (new_ptr) {
            memcpy(new_ptr, ptr, old_size);
            psram_free(ptr);
        }
        return new_ptr;
    }

    return __real_realloc(ptr, new_size);
}

void psram_free(void *ptr) {
    if (!ptr) return;

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_start = PSRAM_BASE + SCRATCH_SIZE;
    uintptr_t heap_end = heap_start + HEAP_SIZE;

    if (addr >= heap_start && addr < heap_end) {
        // It's in our heap
        block_header_t *header = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);

        if (BLOCK_FREE(header)) {
            // Double free - ignore
            return;
        }

        // Mark as free
        header->size_and_free |= 1;

        // Insert into free list sorted by address (for coalescing)
        block_header_t *prev = NULL;
        block_header_t *curr = free_list;
        while (curr && curr < header) {
            prev = curr;
            curr = curr->next_free;
        }

        // Insert header between prev and curr
        header->next_free = curr;
        if (prev) {
            prev->next_free = header;
        } else {
            free_list = header;
        }

        // Coalesce with next block if adjacent
        if (curr) {
            uint8_t *header_end = (uint8_t *)header + BLOCK_SIZE(header);
            if (header_end == (uint8_t *)curr) {
                // Merge header and curr
                header->size_and_free = ((BLOCK_SIZE(header) + BLOCK_SIZE(curr)) & ~1u) | 1;
                header->next_free = curr->next_free;
            }
        }

        // Coalesce with previous block if adjacent
        if (prev) {
            uint8_t *prev_end = (uint8_t *)prev + BLOCK_SIZE(prev);
            if (prev_end == (uint8_t *)header) {
                // Merge prev and header
                prev->size_and_free = ((BLOCK_SIZE(prev) + BLOCK_SIZE(header)) & ~1u) | 1;
                prev->next_free = header->next_free;
            }
        }
        return;
    }

    // Check if in temp region - do nothing (bump allocator)
    uintptr_t temp_start = heap_end;
    uintptr_t temp_end = PSRAM_BASE + PSRAM_SIZE;
    if (addr >= temp_start && addr < temp_end) {
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
    // Reset heap to initial state
    heap_initialized = 0;
    free_list = NULL;
    heap_init();
    psram_temp_offset = 0;
}

void psram_mark_session(void) {
    // Not supported with free-list allocator
    printf("PSRAM: mark_session not supported\n");
}

void psram_restore_session(void) {
    // Not supported with free-list allocator
    printf("PSRAM: restore_session not supported\n");
}

void psram_print_status(void) {
    heap_init();

    // Count free memory
    size_t free_bytes = 0;
    size_t free_blocks = 0;
    block_header_t *curr = free_list;
    while (curr) {
        free_bytes += BLOCK_SIZE(curr);
        free_blocks++;
        curr = curr->next_free;
    }

    printf("PSRAM Status:\n");
    printf("  Heap: %lu free in %lu blocks (of %lu total)\n",
           (unsigned long)free_bytes, (unsigned long)free_blocks,
           (unsigned long)HEAP_SIZE);
    printf("  Temp: %d/%d bytes\n",
           (int)psram_temp_offset, (int)TEMP_SIZE);
}
