/* Cabal - Legacy Game Implementations
 *
 * Minimal RP2350 Backend Implementation
 */

#include "backends/platform/rp2350/rp2350-minimal.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include <string.h>

//============================================================================
// Internal State
//============================================================================

static struct {
    // Graphics
    CabalSurface screen;
    CabalSurface overlay;
    uint8_t *framebuffer;
    uint8_t palette[256 * 3];
    bool paletteDirty;
    bool overlayVisible;
    int screenWidth;
    int screenHeight;

    // Cursor
    bool cursorVisible;
    int cursorX, cursorY;
    int cursorHotX, cursorHotY;
    int cursorW, cursorH;
    uint8_t cursorKeyColor;
    uint8_t *cursorData;

    // Input
    int16_t mouseX, mouseY;
    uint8_t mouseButtons;
    uint8_t prevMouseButtons;

    // Timing
    uint32_t startTime;

    // Initialized flag
    bool initialized;
} g_state;

//============================================================================
// Initialization
//============================================================================

void cabal_system_init(void) {
    if (g_state.initialized) return;

    printf("Cabal Minimal System: Initializing...\n");

    memset(&g_state, 0, sizeof(g_state));

    // Record start time
    g_state.startTime = time_us_32();

    // Initialize input
    printf("  Initializing PS/2 keyboard...\n");
    ps2kbd_init();

    printf("  Initializing PS/2 mouse...\n");
    ps2mouse_wrapper_init();

    // Default mouse position
    g_state.mouseX = 160;
    g_state.mouseY = 100;

    g_state.initialized = true;
    printf("Cabal Minimal System: Ready.\n");
}

//============================================================================
// Graphics
//============================================================================

// External double-buffer functions from main.c
extern "C" {
    uint8_t *cabal_get_framebuffer(void);
    uint8_t *cabal_get_back_buffer(void);
    void cabal_swap_buffers(void);
}

void cabal_init_graphics(int width, int height) {
    printf("Cabal Graphics: initSize(%d, %d)\n", width, height);

    g_state.screenWidth = width;
    g_state.screenHeight = height;

    // Allocate screen surface from PSRAM
    if (g_state.screen.pixels) {
        // Already allocated - just clear
        memset(g_state.screen.pixels, 0, width * height);
    } else {
        g_state.screen.pixels = (uint8_t *)psram_malloc(width * height);
        if (g_state.screen.pixels) {
            memset(g_state.screen.pixels, 0, width * height);
        }
    }
    g_state.screen.width = width;
    g_state.screen.height = height;
    g_state.screen.pitch = width;
    g_state.screen.bytesPerPixel = 1;

    // Allocate overlay surface from PSRAM
    if (g_state.overlay.pixels) {
        memset(g_state.overlay.pixels, 0, width * height);
    } else {
        g_state.overlay.pixels = (uint8_t *)psram_malloc(width * height);
        if (g_state.overlay.pixels) {
            memset(g_state.overlay.pixels, 0, width * height);
        }
    }
    g_state.overlay.width = width;
    g_state.overlay.height = height;
    g_state.overlay.pitch = width;
    g_state.overlay.bytesPerPixel = 1;

    // Use double-buffered framebuffer from main.c (already allocated)
    // Get the back buffer for initial drawing
    g_state.framebuffer = cabal_get_back_buffer();
    printf("Cabal Graphics: Using double-buffered framebuffer at %p\n", g_state.framebuffer);
}

CabalSurface *cabal_lock_screen(void) {
    return &g_state.screen;
}

void cabal_unlock_screen(void) {
    // Nothing to do
}

static void update_hardware_palette(void) {
    if (!g_state.paletteDirty) return;

    for (int i = 0; i < 256; i++) {
        uint32_t color = ((uint32_t)g_state.palette[i * 3] << 16) |
                         ((uint32_t)g_state.palette[i * 3 + 1] << 8) |
                         (uint32_t)g_state.palette[i * 3 + 2];
        graphics_set_palette(i, color);
    }
    g_state.paletteDirty = false;
}

static void draw_cursor(void) {
    if (!g_state.cursorData || !g_state.framebuffer) return;
    if (!g_state.cursorVisible) return;

    // Calculate vertical offset for letterboxing (320x200 in 320x240)
    int yOffset = (240 - g_state.screenHeight) / 2;
    int drawX = g_state.cursorX - g_state.cursorHotX;
    int drawY = g_state.cursorY - g_state.cursorHotY + yOffset;

    for (int cy = 0; cy < g_state.cursorH; cy++) {
        int screenY = drawY + cy;
        if (screenY < 0 || screenY >= 240) continue;

        for (int cx = 0; cx < g_state.cursorW; cx++) {
            int screenX = drawX + cx;
            if (screenX < 0 || screenX >= 320) continue;

            uint8_t pixel = g_state.cursorData[cy * g_state.cursorW + cx];
            if (pixel != g_state.cursorKeyColor) {
                g_state.framebuffer[screenY * 320 + screenX] = pixel;
            }
        }
    }
}

void cabal_update_screen(void) {
    update_hardware_palette();

    // Get the back buffer (not being displayed) for drawing
    g_state.framebuffer = cabal_get_back_buffer();
    if (!g_state.framebuffer) return;

    // Calculate vertical centering offset (320x200 in 320x240)
    int yOffset = (240 - g_state.screenHeight) / 2;

    // Clear top border
    if (yOffset > 0) {
        memset(g_state.framebuffer, 0, 320 * yOffset);
    }

    // Select source surface
    CabalSurface *src = g_state.overlayVisible ? &g_state.overlay : &g_state.screen;
    if (!src->pixels) return;

    // Copy to framebuffer
    uint8_t *dst = g_state.framebuffer + yOffset * 320;
    const uint8_t *srcPtr = src->pixels;
    int copyHeight = g_state.screenHeight;
    if (yOffset + copyHeight > 240) {
        copyHeight = 240 - yOffset;
    }

    for (int row = 0; row < copyHeight; row++) {
        int copyWidth = (src->width < 320) ? src->width : 320;
        memcpy(dst, srcPtr, copyWidth);
        srcPtr += src->pitch;
        dst += 320;
    }

    // Clear bottom border
    int bottomY = yOffset + g_state.screenHeight;
    if (bottomY < 240) {
        memset(g_state.framebuffer + bottomY * 320, 0, 320 * (240 - bottomY));
    }

    // Draw cursor (now on back buffer, not displayed yet)
    if (g_state.cursorVisible && !g_state.overlayVisible) {
        draw_cursor();
    }

    // Swap buffers - display what we just drew, get new back buffer for next frame
    cabal_swap_buffers();
}

void cabal_set_palette(const uint8_t *colors, int start, int num) {
    memcpy(g_state.palette + start * 3, colors, num * 3);
    g_state.paletteDirty = true;
}

void cabal_get_palette(uint8_t *colors, int start, int num) {
    memcpy(colors, g_state.palette + start * 3, num * 3);
}

void cabal_fill_screen(uint8_t color) {
    if (g_state.screen.pixels) {
        memset(g_state.screen.pixels, color,
               g_state.screen.width * g_state.screen.height);
    }
}

int cabal_screen_width(void) {
    return g_state.screenWidth;
}

int cabal_screen_height(void) {
    return g_state.screenHeight;
}

//============================================================================
// Overlay
//============================================================================

void cabal_show_overlay(void) {
    g_state.overlayVisible = true;
}

void cabal_hide_overlay(void) {
    g_state.overlayVisible = false;
}

bool cabal_overlay_visible(void) {
    return g_state.overlayVisible;
}

CabalSurface *cabal_get_overlay(void) {
    return &g_state.overlay;
}

//============================================================================
// Cursor
//============================================================================

void cabal_show_mouse(bool visible) {
    g_state.cursorVisible = visible;
}

void cabal_set_mouse_pos(int x, int y) {
    g_state.cursorX = x;
    g_state.cursorY = y;
}

void cabal_set_mouse_cursor(const uint8_t *data, int w, int h,
                            int hotX, int hotY, uint8_t keycolor) {
    if (g_state.cursorData) {
        // Note: Using PSRAM bump allocator, can't free
    }

    g_state.cursorW = w;
    g_state.cursorH = h;
    g_state.cursorHotX = hotX;
    g_state.cursorHotY = hotY;
    g_state.cursorKeyColor = keycolor;

    g_state.cursorData = (uint8_t *)psram_malloc(w * h);
    if (g_state.cursorData) {
        memcpy(g_state.cursorData, data, w * h);
    }
}

//============================================================================
// Events
//============================================================================

bool cabal_poll_event(CabalEvent *event) {
    // Poll input devices
    ps2kbd_tick();
    ps2mouse_wrapper_tick();

    // Check for keyboard events
    int pressed, keycode;
    if (ps2kbd_get_key(&pressed, &keycode)) {
        event->type = pressed ? CABAL_EVENT_KEYDOWN : CABAL_EVENT_KEYUP;
        event->kbd.keycode = keycode;

        // Get ASCII value
        if (keycode >= 32 && keycode < 127) {
            event->kbd.ascii = keycode;
        } else {
            event->kbd.ascii = 0;
        }

        // Get modifier flags
        int mods = ps2kbd_get_modifiers();
        event->kbd.flags = 0;
        if (mods & 1) event->kbd.flags |= CABAL_MOD_SHIFT;
        if (mods & 2) event->kbd.flags |= CABAL_MOD_CTRL;
        if (mods & 4) event->kbd.flags |= CABAL_MOD_ALT;

        return true;
    }

    // Check for mouse motion
    int16_t dx, dy;
    if (ps2mouse_get_motion(&dx, &dy)) {
        // Clamp large deltas to prevent jumping (likely spurious values)
        const int16_t MAX_DELTA = 20;
        if (dx > MAX_DELTA) dx = MAX_DELTA;
        if (dx < -MAX_DELTA) dx = -MAX_DELTA;
        if (dy > MAX_DELTA) dy = MAX_DELTA;
        if (dy < -MAX_DELTA) dy = -MAX_DELTA;

        // Reduce sensitivity
        g_state.mouseX += dx / 2;
        g_state.mouseY -= dy / 2;  // Invert Y for screen coordinates

        // Clamp to screen bounds
        if (g_state.mouseX < 0) g_state.mouseX = 0;
        if (g_state.mouseX >= g_state.screenWidth) g_state.mouseX = g_state.screenWidth - 1;
        if (g_state.mouseY < 0) g_state.mouseY = 0;
        if (g_state.mouseY >= g_state.screenHeight) g_state.mouseY = g_state.screenHeight - 1;

        event->type = CABAL_EVENT_MOUSEMOVE;
        event->mouse.x = g_state.mouseX;
        event->mouse.y = g_state.mouseY;

        // Update cursor position
        g_state.cursorX = g_state.mouseX;
        g_state.cursorY = g_state.mouseY;

        return true;
    }

    // Check for mouse button events
    uint8_t buttons = ps2mouse_get_buttons();
    if (buttons != g_state.prevMouseButtons) {
        // Left button
        if ((buttons & 1) != (g_state.prevMouseButtons & 1)) {
            event->type = (buttons & 1) ? CABAL_EVENT_LBUTTONDOWN : CABAL_EVENT_LBUTTONUP;
            event->mouse.x = g_state.mouseX;
            event->mouse.y = g_state.mouseY;
            g_state.prevMouseButtons = (g_state.prevMouseButtons & ~1) | (buttons & 1);
            return true;
        }
        // Right button
        if ((buttons & 2) != (g_state.prevMouseButtons & 2)) {
            event->type = (buttons & 2) ? CABAL_EVENT_RBUTTONDOWN : CABAL_EVENT_RBUTTONUP;
            event->mouse.x = g_state.mouseX;
            event->mouse.y = g_state.mouseY;
            g_state.prevMouseButtons = (g_state.prevMouseButtons & ~2) | (buttons & 2);
            return true;
        }
        // Middle button
        if ((buttons & 4) != (g_state.prevMouseButtons & 4)) {
            event->type = (buttons & 4) ? CABAL_EVENT_MBUTTONDOWN : CABAL_EVENT_MBUTTONUP;
            event->mouse.x = g_state.mouseX;
            event->mouse.y = g_state.mouseY;
            g_state.prevMouseButtons = (g_state.prevMouseButtons & ~4) | (buttons & 4);
            return true;
        }
        g_state.prevMouseButtons = buttons;
    }

    event->type = CABAL_EVENT_NONE;
    return false;
}

//============================================================================
// Timing
//============================================================================

uint32_t cabal_get_millis(void) {
    uint32_t now = time_us_32();
    return (now - g_state.startTime) / 1000;
}

void cabal_delay(uint32_t ms) {
    sleep_ms(ms);
}
