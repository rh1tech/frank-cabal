/* Cabal - Legacy Game Implementations
 *
 * Memory allocation overrides for RP2350
 * Uses linker --wrap to redirect malloc/free to PSRAM
 */

#include <cstdlib>
#include <cstddef>

extern "C" {
#include "psram_allocator.h"

// Linker will call these when code calls malloc/free/realloc
// Original functions are available as __real_malloc, etc.

void* __real_malloc(size_t size);
void __real_free(void* ptr);
void* __real_realloc(void* ptr, size_t size);

void* __wrap_malloc(size_t size) {
    // Use PSRAM for allocations
    return psram_malloc(size);
}

void __wrap_free(void* ptr) {
    psram_free(ptr);
}

void* __wrap_realloc(void* ptr, size_t size) {
    return psram_realloc(ptr, size);
}

// Also wrap calloc
void* __real_calloc(size_t nmemb, size_t size);

void* __wrap_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = psram_malloc(total);
    if (ptr) {
        // Zero the memory (calloc requirement)
        char* p = (char*)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

} // extern "C"
