/* Cabal - Legacy Game Implementations
 *
 * RP2350 OSystem implementation
 */

#ifndef BACKENDS_PLATFORM_RP2350_SYSTEM_H
#define BACKENDS_PLATFORM_RP2350_SYSTEM_H

#include "common/system.h"
#include "common/events.h"
#include "graphics/surface.h"
#include "graphics/palette.h"

namespace Audio {
class MixerImpl;
}

/**
 * OSystem implementation for RP2350 (Raspberry Pi Pico 2)
 */
class OSystem_RP2350 : public OSystem, public Common::EventSource, public PaletteManager {
protected:
	// Screen
	Graphics::Surface _screen;
	Graphics::Surface _overlay;
	int _screenWidth;
	int _screenHeight;
	int _shakeOffset;
	bool _overlayVisible;

	// Palette
	byte _palette[256 * 3];
	bool _paletteDirty;

	// Mouse
	struct MouseCursor {
		byte *data;
		int w, h;
		int hotX, hotY;
		uint32 keycolor;
	} _cursor;
	int _mouseX, _mouseY;
	bool _mouseVisible;
	uint8 _mouseButtons;
	uint8 _prevMouseButtons;

	// Audio
	Audio::MixerImpl *_mixer;

	// Timing
	uint32 _startTime;

	// State
	bool _quitRequested;

public:
	OSystem_RP2350();
	virtual ~OSystem_RP2350();

	// OSystem interface
	virtual void initBackend();

	// Feature support
	virtual bool hasFeature(Feature f);
	virtual void setFeatureState(Feature f, bool enable);
	virtual bool getFeatureState(Feature f);

	// Graphics
	virtual const GraphicsMode *getSupportedGraphicsModes() const;
	virtual int getDefaultGraphicsMode() const;
	virtual bool setGraphicsMode(int mode);
	virtual int getGraphicsMode() const;
	virtual void initSize(uint width, uint height, const Graphics::PixelFormat *format = NULL);
	virtual int16 getHeight();
	virtual int16 getWidth();
	virtual PaletteManager *getPaletteManager();
	virtual void copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h);
	virtual Graphics::Surface *lockScreen();
	virtual void unlockScreen();
	virtual void fillScreen(uint32 col);
	virtual void updateScreen();
	virtual void setShakePos(int shakeOffset);

	// PaletteManager interface
	virtual void setPalette(const byte *colors, uint start, uint num);
	virtual void grabPalette(byte *colors, uint start, uint num);

	// Overlay
	virtual void showOverlay();
	virtual void hideOverlay();
	virtual Graphics::PixelFormat getOverlayFormat() const;
	virtual void clearOverlay();
	virtual void grabOverlay(void *buf, int pitch);
	virtual void copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h);
	virtual int16 getOverlayHeight();
	virtual int16 getOverlayWidth();

	// Mouse
	virtual bool showMouse(bool visible);
	virtual void warpMouse(int x, int y);
	virtual void setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY,
	                            uint32 keycolor, bool dontScale = false,
	                            const Graphics::PixelFormat *format = NULL);

	// Events
	virtual uint32 getMillis();
	virtual void delayMillis(uint msecs);
	virtual void getTimeAndDate(TimeDate &t) const;

	// Mutex
	virtual MutexRef createMutex();
	virtual void lockMutex(MutexRef mutex);
	virtual void unlockMutex(MutexRef mutex);
	virtual void deleteMutex(MutexRef mutex);

	// Audio
	virtual Audio::Mixer *getMixer();

	// Misc
	virtual void quit();
	virtual void displayMessageOnOSD(const char *msg);
	virtual void logMessage(LogMessageType::Type type, const char *message);

	// EventSource interface
	virtual bool pollEvent(Common::Event &event);

protected:
	Common::EventSource *getDefaultEventSource() { return this; }
	void updatePalette();
};

#endif // BACKENDS_PLATFORM_RP2350_SYSTEM_H
