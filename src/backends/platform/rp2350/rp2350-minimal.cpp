/* Cabal - Legacy Game Implementations
 *
 * Minimal RP2350 Backend Implementation
 */

#include "backends/platform/rp2350/rp2350-minimal.h"
#include "board_config.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pio.h"
#include <stdio.h>
#include <string.h>

#ifdef USB_HID_ENABLED
#include "usbkbd_wrapper.h"
#include "usbmouse_wrapper.h"
#else
#include "ps2.h"
#include "ps2kbd_wrapper.h"
#endif

//============================================================================
// Hard Fault Handler - catches crashes and prints diagnostic info
//============================================================================

extern "C" void isr_hardfault(void) __attribute__((naked));
extern "C" void isr_hardfault(void) {
    __asm volatile (
        "mrs r0, msp\n"           // Get main stack pointer
        "b hard_fault_handler_c\n"
    );
}

extern "C" void hard_fault_handler_c(uint32_t *stack) {
    // Stack frame: r0, r1, r2, r3, r12, lr, pc, xpsr
    uint32_t r0  = stack[0];
    uint32_t r1  = stack[1];
    uint32_t r2  = stack[2];
    uint32_t r3  = stack[3];
    uint32_t r12 = stack[4];
    uint32_t lr  = stack[5];
    uint32_t pc  = stack[6];
    uint32_t psr = stack[7];

    printf("\n\n*** HARD FAULT ***\n");
    printf("PC:  0x%08lx (crashed here)\n", pc);
    printf("LR:  0x%08lx (called from)\n", lr);
    printf("PSR: 0x%08lx\n", psr);
    printf("R0:  0x%08lx  R1: 0x%08lx\n", r0, r1);
    printf("R2:  0x%08lx  R3: 0x%08lx\n", r2, r3);
    printf("R12: 0x%08lx\n", r12);
    printf("SP:  0x%08lx\n", (uint32_t)stack);

    // Check for stack overflow
    extern char __StackLimit;
    if ((uint32_t)stack < (uint32_t)&__StackLimit) {
        printf("*** STACK OVERFLOW DETECTED ***\n");
    }

    // Print memory around crash
    printf("Memory at PC-8: ");
    for (int i = -2; i < 4; i++) {
        printf("%08lx ", ((uint32_t*)pc)[i]);
    }
    printf("\n");

    // Hang with LED blinking pattern
    while (1) {
        for (volatile int i = 0; i < 500000; i++);
        printf(".");
    }
}

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
#ifdef USB_HID_ENABLED
    printf("  Initializing USB HID keyboard...\n");
    usbkbd_init();
    printf("  Initializing USB HID mouse...\n");
    usbmouse_init();
#else
    printf("  Initializing PS/2 keyboard (pio0)...\n");
    ps2kbd_init();

    // Initialize PS/2 mouse using pio1 (keyboard uses pio0)
    printf("  Initializing PS/2 mouse (pio1)...\n");
    if (ps2_mouse_pio_init(pio1, PS2_MOUSE_CLK)) {
        if (ps2_mouse_init_device()) {
            printf("  PS/2 mouse initialized (interrupt-driven streaming)\n");
        } else {
            printf("  PS/2 mouse device init failed\n");
        }
    } else {
        printf("  PS/2 mouse PIO init failed\n");
    }
#endif

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

    // Validate cursor dimensions to prevent buffer overflows
    int cursorW = g_state.cursorW;
    int cursorH = g_state.cursorH;
    if (cursorW <= 0 || cursorW > 32 || cursorH <= 0 || cursorH > 32) return;

    // Calculate vertical offset for letterboxing (320x200 in 320x240)
    int yOffset = (240 - g_state.screenHeight) / 2;
    int drawX = g_state.cursorX - g_state.cursorHotX;
    int drawY = g_state.cursorY - g_state.cursorHotY + yOffset;

    for (int cy = 0; cy < cursorH; cy++) {
        int screenY = drawY + cy;
        if (screenY < 0 || screenY >= 240) continue;

        for (int cx = 0; cx < cursorW; cx++) {
            int screenX = drawX + cx;
            if (screenX < 0 || screenX >= 320) continue;

            uint8_t pixel = g_state.cursorData[cy * cursorW + cx];
            if (pixel != g_state.cursorKeyColor) {
                g_state.framebuffer[screenY * 320 + screenX] = pixel;
            }
        }
    }
}

void cabal_update_screen(void) {
    static uint32_t total_update_time = 0;
    static int update_count = 0;
    uint32_t start = time_us_32();

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

    // Timing stats
    uint32_t elapsed = time_us_32() - start;
    total_update_time += elapsed;
    update_count++;
    if (update_count == 600) {
        total_update_time = 0;
        update_count = 0;
    }
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

// Static cursor buffer to avoid repeated allocations
// Max cursor size is typically 32x32 = 1024 bytes
static uint8_t s_cursorBuffer[32 * 32];

void cabal_set_mouse_cursor(const uint8_t *data, int w, int h,
                            int hotX, int hotY, uint8_t keycolor) {
    // Clamp to max supported cursor size
    if (w > 32) w = 32;
    if (h > 32) h = 32;

    g_state.cursorW = w;
    g_state.cursorH = h;
    g_state.cursorHotX = hotX;
    g_state.cursorHotY = hotY;
    g_state.cursorKeyColor = keycolor;

    // Use static buffer instead of allocating each time
    g_state.cursorData = s_cursorBuffer;
    memcpy(g_state.cursorData, data, w * h);
}

//============================================================================
// Events
//============================================================================

#ifdef USB_HID_ENABLED
// USB HID event polling
bool cabal_poll_event(CabalEvent *event) {
    // Poll USB HID
    usbkbd_tick();

    // Check for keyboard events
    int is_down, keycode;
    if (usbkbd_get_key(&is_down, &keycode)) {
        event->type = is_down ? CABAL_EVENT_KEYDOWN : CABAL_EVENT_KEYUP;
        event->kbd.keycode = keycode;

        // Get ASCII value (basic mapping)
        if (keycode >= 32 && keycode < 127) {
            event->kbd.ascii = keycode;
        } else {
            event->kbd.ascii = 0;
        }

        // Modifier flags handled via modifier keycodes
        event->kbd.flags = 0;
        return true;
    }

    // Check for mouse events (USB HID returns all in one call)
    int16_t dx, dy;
    int8_t dz;
    uint8_t buttons;
    if (usbmouse_get_event(&dx, &dy, &dz, &buttons)) {
        // Check for button changes first (higher priority)
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

        // Apply mouse motion if there is any (reduced sensitivity for USB)
        if (dx != 0 || dy != 0) {
            g_state.mouseX += dx / 2;
            g_state.mouseY += dy / 2;  // USB HID Y is already in screen direction

            // Clamp to screen bounds
            if (g_state.mouseX < 0) g_state.mouseX = 0;
            if (g_state.mouseX >= g_state.screenWidth) g_state.mouseX = g_state.screenWidth - 1;
            if (g_state.mouseY < 0) g_state.mouseY = 0;
            if (g_state.mouseY >= g_state.screenHeight) g_state.mouseY = g_state.screenHeight - 1;

            event->type = CABAL_EVENT_MOUSEMOVE;
            event->mouse.x = g_state.mouseX;
            event->mouse.y = g_state.mouseY;

            g_state.cursorX = g_state.mouseX;
            g_state.cursorY = g_state.mouseY;
            return true;
        }
    }

    event->type = CABAL_EVENT_NONE;
    return false;
}

#else
// Profiling globals - accessible from engine
static uint32_t g_last_click_time = 0;
static uint32_t g_click_count = 0;
static uint32_t g_motion_event_count = 0;
static uint32_t g_last_motion_time = 0;

extern "C" {
    uint32_t cabal_profile_get_last_click_time(void) { return g_last_click_time; }
    uint32_t cabal_profile_get_click_count(void) { return g_click_count; }
}

// PS/2 event polling
bool cabal_poll_event(CabalEvent *event) {
    static uint32_t total_poll_time = 0;
    static uint32_t total_mouse_time = 0;
    static int poll_count = 0;
    static uint32_t motion_events_this_second = 0;
    static uint32_t last_fps_report = 0;
    uint32_t start = time_us_32();

    // Poll keyboard
    ps2kbd_tick();

    // Check for keyboard events
    int pressed;
    unsigned char keycode;
    if (ps2kbd_get_key(&pressed, &keycode)) {
        event->type = pressed ? CABAL_EVENT_KEYDOWN : CABAL_EVENT_KEYUP;
        event->kbd.keycode = keycode;

        // Get ASCII value - control keys have their own ascii codes
        if (keycode >= 32 && keycode < 127) {
            event->kbd.ascii = keycode;
        } else if (keycode == 8 || keycode == 9 || keycode == 13 || keycode == 27) {
            event->kbd.ascii = keycode;  // BS, TAB, Enter, ESC
        } else {
            event->kbd.ascii = 0;
        }

        // Modifier flags are tracked separately by keyboard driver
        event->kbd.flags = 0;

        return true;
    }

    // Get mouse state (Core 1 handles polling in background - this is non-blocking)
    int16_t dx = 0, dy = 0;
    int8_t wheel = 0;
    uint8_t buttons = g_state.prevMouseButtons;

    uint32_t mouse_start = time_us_32();
    ps2_mouse_get_state(&dx, &dy, &wheel, &buttons);
    uint32_t mouse_elapsed = time_us_32() - mouse_start;
    total_mouse_time += mouse_elapsed;

    if (buttons != g_state.prevMouseButtons || dx != 0 || dy != 0) {
        // ALWAYS update cursor position FIRST before any event
        // This prevents losing motion data when button events occur
        if (dx != 0 || dy != 0) {
            // Clamp extreme values (spurious data protection)
            const int16_t MAX_DELTA = 127;
            if (dx > MAX_DELTA) dx = MAX_DELTA;
            if (dx < -MAX_DELTA) dx = -MAX_DELTA;
            if (dy > MAX_DELTA) dy = MAX_DELTA;
            if (dy < -MAX_DELTA) dy = -MAX_DELTA;

            // Apply mouse motion (1:1, invert Y for screen coords)
            g_state.mouseX += dx;
            g_state.mouseY -= dy;

            // Clamp to screen bounds
            if (g_state.mouseX < 0) g_state.mouseX = 0;
            if (g_state.mouseX >= g_state.screenWidth) g_state.mouseX = g_state.screenWidth - 1;
            if (g_state.mouseY < 0) g_state.mouseY = 0;
            if (g_state.mouseY >= g_state.screenHeight) g_state.mouseY = g_state.screenHeight - 1;

            // Update cursor position
            g_state.cursorX = g_state.mouseX;
            g_state.cursorY = g_state.mouseY;
        }

        // Check for button changes (return button events with updated position)
        if (buttons != g_state.prevMouseButtons) {
            // Left button
            if ((buttons & 1) != (g_state.prevMouseButtons & 1)) {
                event->type = (buttons & 1) ? CABAL_EVENT_LBUTTONDOWN : CABAL_EVENT_LBUTTONUP;
                event->mouse.x = g_state.mouseX;
                event->mouse.y = g_state.mouseY;
                g_state.prevMouseButtons = (g_state.prevMouseButtons & ~1) | (buttons & 1);
                // PROFILING: Record click time
                if (buttons & 1) {
                    g_last_click_time = time_us_32();
                    g_click_count++;
                }
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

        // Return motion event if there was movement
        if (dx != 0 || dy != 0) {
            event->type = CABAL_EVENT_MOUSEMOVE;
            event->mouse.x = g_state.mouseX;
            event->mouse.y = g_state.mouseY;
            // PROFILING: Count motion events for FPS
            motion_events_this_second++;
            uint32_t now = time_us_32();
            if (now - last_fps_report >= 1000000) {
                motion_events_this_second = 0;
                last_fps_report = now;
            }
            return true;
        }
    }

    event->type = CABAL_EVENT_NONE;

    // Timing stats
    uint32_t elapsed = time_us_32() - start;
    total_poll_time += elapsed;
    poll_count++;
    if (poll_count == 1000) {
        total_poll_time = 0;
        total_mouse_time = 0;
        poll_count = 0;
    }

    return false;
}
#endif

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
