/* Cabal - Legacy Game Implementations
 *
 * Route ALL memory allocations (malloc, new) to PSRAM.
 * SRAM is too small for game engines with large resource files.
 */

#include <cstddef>
#include <cstring>
#include <new>

extern "C" {
#include "psram_allocator.h"
int printf(const char *, ...);

void *__wrap_malloc(size_t size) {
    return psram_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size) {
    size_t total = count * size;
    void *ptr = psram_malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void *__wrap_realloc(void *ptr, size_t size) {
    return psram_realloc(ptr, size);
}

void __wrap_free(void *ptr) {
    psram_free(ptr);
}

} // extern "C"

void* operator new(std::size_t size) {
    void* ptr = psram_malloc(size);
    if (!ptr) {
        printf("\n*** new(%u) FAILED ***\n", (unsigned)size);
        psram_print_status();
        while(1) { }
    }
    return ptr;
}

void* operator new[](std::size_t size) {
    return operator new(size);
}

void operator delete(void* ptr) noexcept { psram_free(ptr); }
void operator delete[](void* ptr) noexcept { psram_free(ptr); }
void operator delete(void* ptr, std::size_t) noexcept { psram_free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { psram_free(ptr); }
