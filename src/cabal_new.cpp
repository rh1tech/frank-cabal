/* Cabal - Legacy Game Implementations
 *
 * Override C++ new/delete to use PSRAM
 * These override the Pico SDK's weak symbols in pico_cxx_options
 */

#include <cstddef>
#include <new>

extern "C" {
#include "psram_allocator.h"

// For debugging
int printf(const char *, ...);
}

// Strong symbols to override SDK's weak new/delete

void* operator new(std::size_t size) {
    void* ptr = psram_malloc(size);
    if (!ptr) {
        printf("new(%u) FAILED - OOM\n", (unsigned)size);
        while(1) { } // Hang on OOM
    }
    return ptr;
}

void* operator new[](std::size_t size) {
    return operator new(size);
}

void operator delete(void* ptr) noexcept {
    psram_free(ptr);
}

void operator delete[](void* ptr) noexcept {
    psram_free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    psram_free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    psram_free(ptr);
}
