/* Cabal - Legacy Game Implementations
 *
 * Minimal RP2350 Backend - Self-contained, no ScummVM dependencies
 *
 * This is a simplified backend for embedded systems that provides
 * just the essential functionality without the full ScummVM infrastructure.
 */

#ifndef BACKENDS_PLATFORM_RP2350_MINIMAL_H
#define BACKENDS_PLATFORM_RP2350_MINIMAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations for driver functions (implemented in C)
#ifdef __cplusplus
extern "C" {
#endif

// PS/2 Keyboard
void ps2kbd_init(void);
void ps2kbd_tick(void);
int ps2kbd_get_key(int* pressed, int* keycode);
int ps2kbd_get_modifiers(void);

// PS/2 Mouse
void ps2mouse_wrapper_init(void);
void ps2mouse_wrapper_tick(void);
int ps2mouse_get_motion(int16_t* dx, int16_t* dy);
uint8_t ps2mouse_get_buttons(void);

// HDMI Graphics
void graphics_init(int mode);
void graphics_set_buffer(uint8_t* buffer);
void graphics_set_res(int w, int h);
void graphics_set_palette(uint8_t i, uint32_t color888);

// PSRAM Allocator
void *psram_malloc(size_t size);
void psram_free(void *ptr);

#ifdef __cplusplus
}
#endif

//============================================================================
// Key codes (matching Common::KeyCode for easy future migration)
//============================================================================

#define CABAL_KEY_BACKSPACE  8
#define CABAL_KEY_TAB        9
#define CABAL_KEY_RETURN     13
#define CABAL_KEY_ESCAPE     27
#define CABAL_KEY_SPACE      32
#define CABAL_KEY_DELETE     127

#define CABAL_KEY_UP         273
#define CABAL_KEY_DOWN       274
#define CABAL_KEY_RIGHT      275
#define CABAL_KEY_LEFT       276

#define CABAL_KEY_F1         282
#define CABAL_KEY_F2         283
#define CABAL_KEY_F3         284
#define CABAL_KEY_F4         285
#define CABAL_KEY_F5         286
#define CABAL_KEY_F6         287
#define CABAL_KEY_F7         288
#define CABAL_KEY_F8         289
#define CABAL_KEY_F9         290
#define CABAL_KEY_F10        291
#define CABAL_KEY_F11        292
#define CABAL_KEY_F12        293

//============================================================================
// Event types
//============================================================================

typedef enum {
    CABAL_EVENT_NONE = 0,
    CABAL_EVENT_KEYDOWN,
    CABAL_EVENT_KEYUP,
    CABAL_EVENT_MOUSEMOVE,
    CABAL_EVENT_LBUTTONDOWN,
    CABAL_EVENT_LBUTTONUP,
    CABAL_EVENT_RBUTTONDOWN,
    CABAL_EVENT_RBUTTONUP,
    CABAL_EVENT_MBUTTONDOWN,
    CABAL_EVENT_MBUTTONUP,
    CABAL_EVENT_QUIT
} CabalEventType;

typedef struct {
    CabalEventType type;
    union {
        struct {
            int keycode;
            int ascii;
            int flags;
        } kbd;
        struct {
            int16_t x;
            int16_t y;
        } mouse;
    };
} CabalEvent;

// Modifier flags
#define CABAL_MOD_SHIFT  0x01
#define CABAL_MOD_CTRL   0x02
#define CABAL_MOD_ALT    0x04

//============================================================================
// Graphics Surface
//============================================================================

typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
} CabalSurface;

//============================================================================
// System API
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Initialization
void cabal_system_init(void);

// Graphics
void cabal_init_graphics(int width, int height);
CabalSurface *cabal_lock_screen(void);
void cabal_unlock_screen(void);
void cabal_update_screen(void);
void cabal_set_palette(const uint8_t *colors, int start, int num);
void cabal_get_palette(uint8_t *colors, int start, int num);
void cabal_fill_screen(uint8_t color);

// Overlay
void cabal_show_overlay(void);
void cabal_hide_overlay(void);
bool cabal_overlay_visible(void);
CabalSurface *cabal_get_overlay(void);

// Cursor
void cabal_show_mouse(bool visible);
void cabal_set_mouse_pos(int x, int y);
void cabal_set_mouse_cursor(const uint8_t *data, int w, int h,
                            int hotX, int hotY, uint8_t keycolor);

// Events
bool cabal_poll_event(CabalEvent *event);

// Timing
uint32_t cabal_get_millis(void);
void cabal_delay(uint32_t ms);

// Screen dimensions
int cabal_screen_width(void);
int cabal_screen_height(void);

#ifdef __cplusplus
}
#endif

#endif // BACKENDS_PLATFORM_RP2350_MINIMAL_H
