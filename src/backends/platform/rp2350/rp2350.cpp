/* Cabal - Legacy Game Implementations
 *
 * Cabal is the legal property of its developers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "backends/platform/rp2350/rp2350.h"
#include "backends/saves/default/default-saves.h"
#include "backends/timer/default/default-timer.h"
#include "backends/events/default/default-events.h"
#include "backends/mutex/null/null-mutex.h"
#include "audio/mixer_intern.h"
#include "common/scummsys.h"
#include "common/config-manager.h"

#include <stdio.h>
#include <string.h>

extern "C" {
#include "cabal_fs.h"
}

// External PSRAM allocation
extern "C" {
    void *psram_malloc(size_t size);
    void psram_free(void *ptr);
}

//============================================================================
// RP2350 Graphics Manager Implementation
//============================================================================

static const OSystem::GraphicsMode s_supportedGraphicsModes[] = {
    {"1x", "Normal (320x200)", 1},
    {0, 0, 0}
};

RP2350GraphicsManager::RP2350GraphicsManager() :
    _screenWidth(320),
    _screenHeight(200),
    _screenChangeID(0),
    _shakeOffset(0),
    _overlayVisible(false),
    _framebuffer(nullptr),
    _paletteDirty(true),
    _cursorVisible(false),
    _cursorX(0), _cursorY(0),
    _cursorHotspotX(0), _cursorHotspotY(0),
    _cursorWidth(0), _cursorHeight(0),
    _cursorKeyColor(0),
    _cursorData(nullptr),
    _cursorBackup(nullptr),
    _cursorPaletteDisabled(true) {

    memset(_palette, 0, sizeof(_palette));
    memset(_cursorPalette, 0, sizeof(_cursorPalette));

    // Allocate framebuffer from PSRAM
    _framebuffer = (uint8_t *)psram_malloc(320 * 240);
    if (_framebuffer) {
        memset(_framebuffer, 0, 320 * 240);
        graphics_set_buffer(_framebuffer);
    }
}

RP2350GraphicsManager::~RP2350GraphicsManager() {
    _screen.reset();
    _overlay.reset();
    if (_cursorData) delete[] _cursorData;
    if (_cursorBackup) delete[] _cursorBackup;
    // Don't free _framebuffer as it's PSRAM (bump allocator)
}

bool RP2350GraphicsManager::hasFeature(OSystem::Feature f) {
    switch (f) {
    case OSystem::kFeatureCursorPalette:
        return true;
    default:
        return false;
    }
}

void RP2350GraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
    switch (f) {
    case OSystem::kFeatureCursorPalette:
        _cursorPaletteDisabled = !enable;
        break;
    default:
        break;
    }
}

bool RP2350GraphicsManager::getFeatureState(OSystem::Feature f) {
    switch (f) {
    case OSystem::kFeatureCursorPalette:
        return !_cursorPaletteDisabled;
    default:
        return false;
    }
}

const OSystem::GraphicsMode *RP2350GraphicsManager::getSupportedGraphicsModes() const {
    return s_supportedGraphicsModes;
}

int RP2350GraphicsManager::getDefaultGraphicsMode() const {
    return 1;
}

bool RP2350GraphicsManager::setGraphicsMode(int mode) {
    return mode == 1;
}

int RP2350GraphicsManager::getGraphicsMode() const {
    return 1;
}

Graphics::PixelFormat RP2350GraphicsManager::getScreenFormat() const {
    return Graphics::PixelFormat::createFormatCLUT8();
}

Common::List<Graphics::PixelFormat> RP2350GraphicsManager::getSupportedFormats() const {
    Common::List<Graphics::PixelFormat> formats;
    formats.push_back(Graphics::PixelFormat::createFormatCLUT8());
    return formats;
}

void RP2350GraphicsManager::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
    _screenWidth = width;
    _screenHeight = height;

    _screen.reset();
    _screen.create(width, height, Graphics::PixelFormat::createFormatCLUT8());

    _overlay.reset();
    _overlay.create(width, height, Graphics::PixelFormat::createFormatCLUT8());

    _screenChangeID++;

    printf("RP2350 Graphics: initSize(%d, %d)\n", width, height);
}

void RP2350GraphicsManager::beginGFXTransaction() {
    // Nothing to do
}

OSystem::TransactionError RP2350GraphicsManager::endGFXTransaction() {
    return OSystem::kTransactionSuccess;
}

int16 RP2350GraphicsManager::getHeight() {
    return _screenHeight;
}

int16 RP2350GraphicsManager::getWidth() {
    return _screenWidth;
}

void RP2350GraphicsManager::setPalette(const byte *colors, uint start, uint num) {
    memcpy(_palette + start * 3, colors, num * 3);
    _paletteDirty = true;
}

void RP2350GraphicsManager::grabPalette(byte *colors, uint start, uint num) {
    memcpy(colors, _palette + start * 3, num * 3);
}

void RP2350GraphicsManager::updateHardwarePalette() {
    if (!_paletteDirty) return;

    for (int i = 0; i < 256; i++) {
        uint32_t color = (_palette[i * 3] << 16) |
                         (_palette[i * 3 + 1] << 8) |
                         _palette[i * 3 + 2];
        graphics_set_palette(i, color);
    }
    _paletteDirty = false;
}

void RP2350GraphicsManager::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
    const byte *src = (const byte *)buf;
    byte *dst = (byte *)_screen.getBasePtr(x, y);

    for (int row = 0; row < h; row++) {
        memcpy(dst, src, w);
        src += pitch;
        dst += _screen.getPitch();
    }
}

Graphics::Surface *RP2350GraphicsManager::lockScreen() {
    return &_screen;
}

void RP2350GraphicsManager::unlockScreen() {
    // Nothing to do
}

void RP2350GraphicsManager::fillScreen(uint32 col) {
    memset(_screen.getPixels(), col, _screen.getWidth() * _screen.getHeight());
}

void RP2350GraphicsManager::updateScreen() {
    // Update palette if needed
    updateHardwarePalette();

    if (!_framebuffer) return;

    // Calculate vertical centering offset (320x200 in 320x240)
    int yOffset = (240 - _screenHeight) / 2;

    // Clear top border
    if (yOffset > 0) {
        memset(_framebuffer, 0, 320 * yOffset);
    }

    // Copy screen to framebuffer
    const byte *src;
    if (_overlayVisible) {
        src = (const byte *)_overlay.getPixels();
    } else {
        src = (const byte *)_screen.getPixels();
    }

    byte *dst = _framebuffer + yOffset * 320;
    int copyHeight = _screenHeight;
    if (yOffset + copyHeight > 240) {
        copyHeight = 240 - yOffset;
    }

    // Apply shake offset
    if (_shakeOffset != 0) {
        if (_shakeOffset > 0) {
            memset(dst, 0, 320 * _shakeOffset);
            dst += 320 * _shakeOffset;
            copyHeight -= _shakeOffset;
        } else {
            src += _screen.getPitch() * (-_shakeOffset);
            copyHeight += _shakeOffset;
        }
    }

    for (int row = 0; row < copyHeight; row++) {
        memcpy(dst, src, _screenWidth < 320 ? _screenWidth : 320);
        src += _overlayVisible ? _overlay.getPitch() : _screen.getPitch();
        dst += 320;
    }

    // Clear bottom border
    int bottomY = yOffset + _screenHeight;
    if (bottomY < 240) {
        memset(_framebuffer + bottomY * 320, 0, 320 * (240 - bottomY));
    }

    // Draw cursor on top
    if (_cursorVisible && !_overlayVisible) {
        drawCursor();
    }
}

void RP2350GraphicsManager::setShakePos(int shakeOffset) {
    _shakeOffset = shakeOffset;
}

void RP2350GraphicsManager::showOverlay() {
    _overlayVisible = true;
}

void RP2350GraphicsManager::hideOverlay() {
    _overlayVisible = false;
}

Graphics::PixelFormat RP2350GraphicsManager::getOverlayFormat() const {
    return Graphics::PixelFormat::createFormatCLUT8();
}

void RP2350GraphicsManager::clearOverlay() {
    memset(_overlay.getPixels(), 0, _overlay.getWidth() * _overlay.getHeight());
}

void RP2350GraphicsManager::grabOverlay(void *buf, int pitch) {
    const byte *src = (const byte *)_overlay.getPixels();
    byte *dst = (byte *)buf;

    for (int row = 0; row < _overlay.getHeight(); row++) {
        memcpy(dst, src, _overlay.getWidth());
        src += _overlay.getPitch();
        dst += pitch;
    }
}

void RP2350GraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
    const byte *src = (const byte *)buf;
    byte *dst = (byte *)_overlay.getBasePtr(x, y);

    for (int row = 0; row < h; row++) {
        memcpy(dst, src, w);
        src += pitch;
        dst += _overlay.getPitch();
    }
}

int16 RP2350GraphicsManager::getOverlayHeight() {
    return _screenHeight;
}

int16 RP2350GraphicsManager::getOverlayWidth() {
    return _screenWidth;
}

bool RP2350GraphicsManager::showMouse(bool visible) {
    bool oldVisible = _cursorVisible;
    _cursorVisible = visible;
    return oldVisible;
}

void RP2350GraphicsManager::warpMouse(int x, int y) {
    _cursorX = x;
    _cursorY = y;
}

void RP2350GraphicsManager::setMouseCursor(const void *buf, uint w, uint h,
                                            int hotspotX, int hotspotY,
                                            uint32 keycolor, bool dontScale,
                                            const Graphics::PixelFormat *format) {
    if (_cursorData) delete[] _cursorData;
    if (_cursorBackup) delete[] _cursorBackup;

    _cursorWidth = w;
    _cursorHeight = h;
    _cursorHotspotX = hotspotX;
    _cursorHotspotY = hotspotY;
    _cursorKeyColor = keycolor;

    _cursorData = new byte[w * h];
    _cursorBackup = new byte[w * h];
    memcpy(_cursorData, buf, w * h);
}

void RP2350GraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
    memcpy(_cursorPalette + start * 3, colors, num * 3);
}

void RP2350GraphicsManager::drawCursor() {
    if (!_cursorData || !_framebuffer) return;

    int yOffset = (240 - _screenHeight) / 2;
    int drawX = _cursorX - _cursorHotspotX;
    int drawY = _cursorY - _cursorHotspotY + yOffset;

    for (int cy = 0; cy < _cursorHeight; cy++) {
        int screenY = drawY + cy;
        if (screenY < 0 || screenY >= 240) continue;

        for (int cx = 0; cx < _cursorWidth; cx++) {
            int screenX = drawX + cx;
            if (screenX < 0 || screenX >= 320) continue;

            byte pixel = _cursorData[cy * _cursorWidth + cx];
            if (pixel != _cursorKeyColor) {
                _framebuffer[screenY * 320 + screenX] = pixel;
            }
        }
    }
}

void RP2350GraphicsManager::undrawCursor() {
    // For double buffering, we don't need to undraw
}

//============================================================================
// RP2350 OSystem Implementation
//============================================================================

OSystem_RP2350::OSystem_RP2350() :
    _mouseX(160), _mouseY(100),
    _mouseButtons(0), _prevMouseButtons(0),
    _startTime(0) {
}

OSystem_RP2350::~OSystem_RP2350() {
}

void OSystem_RP2350::initBackend() {
    printf("RP2350 Backend: Initializing...\n");

    // Record start time
    _startTime = time_us_32();

    // Initialize input
    printf("  Initializing PS/2 keyboard...\n");
    ps2kbd_init();

    printf("  Initializing PS/2 mouse...\n");
    ps2mouse_wrapper_init();

    // Create managers
    _mutexManager = new NullMutexManager();
    _timerManager = new DefaultTimerManager();
    _eventManager = new DefaultEventManager(this);
    _savefileManager = new DefaultSaveFileManager("/cabal/saves");

    // Create saves directory if it doesn't exist
    cabal_mkdir("/cabal/saves");

    // Create graphics manager
    _graphicsManager = new RP2350GraphicsManager();

    // Create audio mixer (22050 Hz to save CPU)
    _mixer = new Audio::MixerImpl(this, 22050);
    // Mark mixer as not ready for now (audio not yet implemented)
    static_cast<Audio::MixerImpl *>(_mixer)->setReady(false);

    printf("RP2350 Backend: Initialization complete\n");

    ModularBackend::initBackend();
}

Common::KeyCode OSystem_RP2350::convertKeyCode(int keycode) {
    // Our keycode values match Common::KeyCode for most keys
    if (keycode >= 'a' && keycode <= 'z') {
        return (Common::KeyCode)keycode;
    }
    if (keycode >= '0' && keycode <= '9') {
        return (Common::KeyCode)keycode;
    }

    // Special keys
    switch (keycode) {
    case 8: return Common::KEYCODE_BACKSPACE;
    case 9: return Common::KEYCODE_TAB;
    case 13: return Common::KEYCODE_RETURN;
    case 27: return Common::KEYCODE_ESCAPE;
    case 32: return Common::KEYCODE_SPACE;
    case 127: return Common::KEYCODE_DELETE;

    // Punctuation
    case '-': return Common::KEYCODE_MINUS;
    case '=': return Common::KEYCODE_EQUALS;
    case '[': return Common::KEYCODE_LEFTBRACKET;
    case ']': return Common::KEYCODE_RIGHTBRACKET;
    case '\\': return Common::KEYCODE_BACKSLASH;
    case ';': return Common::KEYCODE_SEMICOLON;
    case '\'': return Common::KEYCODE_QUOTE;
    case '`': return Common::KEYCODE_BACKQUOTE;
    case ',': return Common::KEYCODE_COMMA;
    case '.': return Common::KEYCODE_PERIOD;
    case '/': return Common::KEYCODE_SLASH;

    // Arrow keys (our codes match Common::KeyCode)
    case 273: return Common::KEYCODE_UP;
    case 274: return Common::KEYCODE_DOWN;
    case 275: return Common::KEYCODE_RIGHT;
    case 276: return Common::KEYCODE_LEFT;

    // Navigation
    case 277: return Common::KEYCODE_INSERT;
    case 278: return Common::KEYCODE_HOME;
    case 279: return Common::KEYCODE_END;
    case 280: return Common::KEYCODE_PAGEUP;
    case 281: return Common::KEYCODE_PAGEDOWN;

    // Function keys (F1-F12: 282-293)
    case 282: return Common::KEYCODE_F1;
    case 283: return Common::KEYCODE_F2;
    case 284: return Common::KEYCODE_F3;
    case 285: return Common::KEYCODE_F4;
    case 286: return Common::KEYCODE_F5;
    case 287: return Common::KEYCODE_F6;
    case 288: return Common::KEYCODE_F7;
    case 289: return Common::KEYCODE_F8;
    case 290: return Common::KEYCODE_F9;
    case 291: return Common::KEYCODE_F10;
    case 292: return Common::KEYCODE_F11;
    case 293: return Common::KEYCODE_F12;

    // Modifiers
    case 303: return Common::KEYCODE_RSHIFT;
    case 304: return Common::KEYCODE_LSHIFT;
    case 305: return Common::KEYCODE_RCTRL;
    case 306: return Common::KEYCODE_LCTRL;
    case 307: return Common::KEYCODE_RALT;
    case 308: return Common::KEYCODE_LALT;

    default:
        return Common::KEYCODE_INVALID;
    }
}

bool OSystem_RP2350::pollEvent(Common::Event &event) {
    // Poll input devices
    ps2kbd_tick();
    ps2mouse_wrapper_tick();

    // Check for keyboard events
    int pressed, keycode;
    if (ps2kbd_get_key(&pressed, &keycode)) {
        event.type = pressed ? Common::EVENT_KEYDOWN : Common::EVENT_KEYUP;
        event.kbd.keycode = convertKeyCode(keycode);

        // Get ASCII value
        if (keycode >= 32 && keycode < 127) {
            event.kbd.ascii = keycode;
        } else {
            event.kbd.ascii = 0;
        }

        // Get modifier flags
        int mods = ps2kbd_get_modifiers();
        event.kbd.flags = 0;
        if (mods & 1) event.kbd.flags |= Common::KBD_SHIFT;
        if (mods & 2) event.kbd.flags |= Common::KBD_CTRL;
        if (mods & 4) event.kbd.flags |= Common::KBD_ALT;

        return true;
    }

    // Check for mouse motion
    int16_t dx, dy;
    if (ps2mouse_get_motion(&dx, &dy)) {
        _mouseX += dx;
        _mouseY -= dy;  // Invert Y for screen coordinates

        // Clamp to screen bounds
        if (_mouseX < 0) _mouseX = 0;
        if (_mouseX >= 320) _mouseX = 319;
        if (_mouseY < 0) _mouseY = 0;
        if (_mouseY >= 200) _mouseY = 199;

        event.type = Common::EVENT_MOUSEMOVE;
        event.mouse.x = _mouseX;
        event.mouse.y = _mouseY;

        // Update cursor position in graphics manager
        if (_graphicsManager) {
            static_cast<RP2350GraphicsManager *>(_graphicsManager)->warpMouse(_mouseX, _mouseY);
        }

        return true;
    }

    // Check for mouse button events
    uint8_t buttons = ps2mouse_get_buttons();
    if (buttons != _prevMouseButtons) {
        // Left button
        if ((buttons & 1) != (_prevMouseButtons & 1)) {
            event.type = (buttons & 1) ? Common::EVENT_LBUTTONDOWN : Common::EVENT_LBUTTONUP;
            event.mouse.x = _mouseX;
            event.mouse.y = _mouseY;
            _prevMouseButtons = (_prevMouseButtons & ~1) | (buttons & 1);
            return true;
        }
        // Right button
        if ((buttons & 2) != (_prevMouseButtons & 2)) {
            event.type = (buttons & 2) ? Common::EVENT_RBUTTONDOWN : Common::EVENT_RBUTTONUP;
            event.mouse.x = _mouseX;
            event.mouse.y = _mouseY;
            _prevMouseButtons = (_prevMouseButtons & ~2) | (buttons & 2);
            return true;
        }
        // Middle button
        if ((buttons & 4) != (_prevMouseButtons & 4)) {
            event.type = (buttons & 4) ? Common::EVENT_MBUTTONDOWN : Common::EVENT_MBUTTONUP;
            event.mouse.x = _mouseX;
            event.mouse.y = _mouseY;
            _prevMouseButtons = (_prevMouseButtons & ~4) | (buttons & 4);
            return true;
        }
        _prevMouseButtons = buttons;
    }

    return false;
}

uint32 OSystem_RP2350::getMillis() {
    uint32_t now = time_us_32();
    return (now - _startTime) / 1000;
}

void OSystem_RP2350::delayMillis(uint msecs) {
    sleep_ms(msecs);
}

void OSystem_RP2350::getTimeAndDate(TimeDate &t) const {
    // Return a fixed time for now (RTC not implemented)
    t.tm_sec = 0;
    t.tm_min = 0;
    t.tm_hour = 12;
    t.tm_mday = 1;
    t.tm_mon = 0;
    t.tm_year = 125;  // 2025
    t.tm_wday = 0;
}

void OSystem_RP2350::logMessage(LogMessageType::Type type, const char *message) {
    printf("%s", message);
}

void OSystem_RP2350::quit() {
    printf("RP2350: Quit requested\n");
    // On embedded, we just loop forever or reset
    while (1) {
        // Could trigger a watchdog reset here
    }
}

//============================================================================
// Factory function
//============================================================================

OSystem *OSystem_RP2350_create() {
    return new OSystem_RP2350();
}
