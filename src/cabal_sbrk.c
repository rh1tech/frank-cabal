/* Cabal - Legacy Game Implementations
 *
 * Enable PSRAM heap after hardware initialization
 */

#include "psram_allocator.h"

// Called after PSRAM hardware is initialized
void cabal_enable_psram_heap(void) {
    psram_set_ready(1);  // Enable PSRAM allocations in new/malloc
}
