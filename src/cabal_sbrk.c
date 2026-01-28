/* Cabal - Legacy Game Implementations
 *
 * Stub file - PSRAM heap disabled for now
 * Using standard SRAM heap with explicit psram_malloc for large allocations
 */

// Placeholder function (does nothing - heap stays in SRAM)
void cabal_enable_psram_heap(void) {
    // Do nothing - we'll use psram_malloc explicitly for large allocations
}
