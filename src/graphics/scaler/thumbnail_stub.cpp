/* Cabal - Legacy Game Implementations
 *
 * Minimal thumbnail stub for embedded platforms.
 * We don't need save game thumbnails on embedded devices.
 */

#include "graphics/surface.h"

// Stub implementation - just return an empty thumbnail
bool createThumbnailFromScreen(Graphics::Surface *surf) {
    // Create a minimal 1x1 thumbnail
    if (surf) {
        surf->create(1, 1, Graphics::PixelFormat::createFormatCLUT8());
        if (surf->getPixels()) {
            *((byte *)surf->getPixels()) = 0;  // Black pixel
        }
    }
    return true;
}
