/* Cabal - Legacy Game Implementations
 *
 * Cabal is the legal property of its developers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef BACKENDS_PLATFORM_RP2350_H
#define BACKENDS_PLATFORM_RP2350_H

#include "backends/modular-backend.h"
#include "backends/graphics/graphics.h"
#include "graphics/surface.h"
#include "graphics/pixelformat.h"
#include "common/events.h"

// Forward declarations for Pico SDK types
extern "C" {
    void ps2kbd_init(void);
    void ps2kbd_tick(void);
    int ps2kbd_get_key(int* pressed, int* keycode);
    int ps2kbd_get_modifiers(void);

    void ps2mouse_wrapper_init(void);
    void ps2mouse_wrapper_tick(void);
    int ps2mouse_get_motion(int16_t* dx, int16_t* dy);
    uint8_t ps2mouse_get_buttons(void);

    void graphics_init(int mode);
    void graphics_set_buffer(uint8_t* buffer);
    void graphics_set_res(int w, int h);
    void graphics_set_palette(uint8_t i, uint32_t color888);

    uint32_t time_us_32(void);
    void sleep_ms(uint32_t ms);
}

/**
 * RP2350 Graphics Manager
 * Handles 8-bit indexed color display via HDMI
 */
class RP2350GraphicsManager : public GraphicsManager {
public:
    RP2350GraphicsManager();
    virtual ~RP2350GraphicsManager();

    // GraphicsManager interface
    virtual bool hasFeature(OSystem::Feature f);
    virtual void setFeatureState(OSystem::Feature f, bool enable);
    virtual bool getFeatureState(OSystem::Feature f);

    virtual const OSystem::GraphicsMode *getSupportedGraphicsModes() const;
    virtual int getDefaultGraphicsMode() const;
    virtual bool setGraphicsMode(int mode);
    virtual int getGraphicsMode() const;
    virtual void resetGraphicsScale() {}

    virtual Graphics::PixelFormat getScreenFormat() const;
    virtual Common::List<Graphics::PixelFormat> getSupportedFormats() const;

    virtual void initSize(uint width, uint height, const Graphics::PixelFormat *format);
    virtual int getScreenChangeID() const { return _screenChangeID; }

    virtual void beginGFXTransaction();
    virtual OSystem::TransactionError endGFXTransaction();

    virtual int16 getHeight();
    virtual int16 getWidth();

    virtual void setPalette(const byte *colors, uint start, uint num);
    virtual void grabPalette(byte *colors, uint start, uint num);

    virtual void copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h);
    virtual Graphics::Surface *lockScreen();
    virtual void unlockScreen();
    virtual void fillScreen(uint32 col);
    virtual void updateScreen();
    virtual void setShakePos(int shakeOffset);
    virtual void setFocusRectangle(const Common::Rect &rect) {}
    virtual void clearFocusRectangle() {}

    // Overlay (for GUI) - we use same format as screen
    virtual void showOverlay();
    virtual void hideOverlay();
    virtual Graphics::PixelFormat getOverlayFormat() const;
    virtual void clearOverlay();
    virtual void grabOverlay(void *buf, int pitch);
    virtual void copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h);
    virtual int16 getOverlayHeight();
    virtual int16 getOverlayWidth();

    // Mouse cursor
    virtual bool showMouse(bool visible);
    virtual void warpMouse(int x, int y);
    virtual void setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY,
                                uint32 keycolor, bool dontScale, const Graphics::PixelFormat *format);
    virtual void setCursorPalette(const byte *colors, uint start, uint num);

private:
    void updateHardwarePalette();
    void drawCursor();
    void undrawCursor();

    // Screen state
    uint16 _screenWidth;
    uint16 _screenHeight;
    Graphics::Surface _screen;
    Graphics::Surface _overlay;
    int _screenChangeID;
    int _shakeOffset;
    bool _overlayVisible;

    // Framebuffer (allocated from PSRAM)
    uint8_t *_framebuffer;

    // Palette (256 colors, RGB888)
    byte _palette[256 * 3];
    bool _paletteDirty;

    // Mouse cursor state
    bool _cursorVisible;
    int _cursorX, _cursorY;
    int _cursorHotspotX, _cursorHotspotY;
    int _cursorWidth, _cursorHeight;
    uint32 _cursorKeyColor;
    byte *_cursorData;
    byte *_cursorBackup;
    byte _cursorPalette[256 * 3];
    bool _cursorPaletteDisabled;
};

/**
 * RP2350 OSystem Backend
 * Main backend class for RP2350 with HDMI, PS/2 input, I2S audio, SD card
 */
class OSystem_RP2350 : public ModularBackend, Common::EventSource {
public:
    OSystem_RP2350();
    virtual ~OSystem_RP2350();

    virtual void initBackend();

    // Event handling (from EventSource)
    virtual bool pollEvent(Common::Event &event);

    // Time functions
    virtual uint32 getMillis();
    virtual void delayMillis(uint msecs);
    virtual void getTimeAndDate(TimeDate &t) const;

    // Logging
    virtual void logMessage(LogMessageType::Type type, const char *message);

    // Quit
    virtual void quit();

protected:
    virtual Common::EventSource *getDefaultEventSource() { return this; }

private:
    // Input state
    int16_t _mouseX, _mouseY;
    uint8_t _mouseButtons;
    uint8_t _prevMouseButtons;

    // Timing
    uint32_t _startTime;

    // Convert our keycode to Common::KeyCode
    Common::KeyCode convertKeyCode(int keycode);
};

// Factory function
OSystem *OSystem_RP2350_create();

#endif // BACKENDS_PLATFORM_RP2350_H
