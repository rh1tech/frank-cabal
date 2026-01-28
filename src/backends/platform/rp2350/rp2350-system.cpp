/* Cabal - Legacy Game Implementations
 *
 * RP2350 OSystem implementation
 */

#include "backends/platform/rp2350/rp2350-system.h"
#include "backends/platform/rp2350/rp2350-minimal.h"
#include "backends/fs/rp2350/rp2350-fs-factory.h"
#include "backends/events/default/default-events.h"
#include "backends/timer/default/default-timer.h"
#include "common/config-manager.h"
#include "audio/mixer_intern.h"

#include <stdio.h>
#include <string.h>

// Graphics mode
static const OSystem::GraphicsMode s_supportedGraphicsModes[] = {
	{"1x", "Normal", 1},
	{0, 0, 0}
};

OSystem_RP2350::OSystem_RP2350()
	: _screenWidth(320), _screenHeight(200), _shakeOffset(0),
	  _overlayVisible(false), _paletteDirty(false),
	  _mouseX(160), _mouseY(100), _mouseVisible(false),
	  _mouseButtons(0), _prevMouseButtons(0),
	  _mixer(nullptr), _startTime(0), _quitRequested(false) {

	memset(&_cursor, 0, sizeof(_cursor));
	memset(_palette, 0, sizeof(_palette));
}

OSystem_RP2350::~OSystem_RP2350() {
	if (_cursor.data) {
		delete[] _cursor.data;
	}
	if (_mixer) {
		delete _mixer;
	}
	_screen.reset();
	_overlay.reset();
}

void OSystem_RP2350::initBackend() {
	printf("OSystem_RP2350: Initializing backend...\n");

	// Initialize the minimal system
	cabal_system_init();

	// Set filesystem factory
	_fsFactory = &RP2350FilesystemFactory::instance();

	// Create event manager (OSystem_RP2350 is the EventSource)
	_eventManager = new DefaultEventManager(this);

	// Create timer manager
	_timerManager = new DefaultTimerManager();

	// Record start time
	_startTime = cabal_get_millis();

	// Create mixer (no actual audio output on embedded, but engines need it)
	_mixer = new Audio::MixerImpl(this, 22050);
	_mixer->setReady(true);

	printf("OSystem_RP2350: Backend initialized.\n");
	OSystem::initBackend();
}

bool OSystem_RP2350::hasFeature(Feature f) {
	switch (f) {
	case kFeatureCursorPalette:
		return true;
	default:
		return false;
	}
}

void OSystem_RP2350::setFeatureState(Feature f, bool enable) {
	// Not implemented
}

bool OSystem_RP2350::getFeatureState(Feature f) {
	return false;
}

// Graphics

const OSystem::GraphicsMode *OSystem_RP2350::getSupportedGraphicsModes() const {
	return s_supportedGraphicsModes;
}

int OSystem_RP2350::getDefaultGraphicsMode() const {
	return 1;
}

bool OSystem_RP2350::setGraphicsMode(int mode) {
	return true;
}

int OSystem_RP2350::getGraphicsMode() const {
	return 1;
}

void OSystem_RP2350::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	printf("OSystem_RP2350: initSize(%u, %u)\n", width, height);

	_screenWidth = width;
	_screenHeight = height;

	// Initialize the minimal backend graphics
	cabal_init_graphics(width, height);

	// Create screen surface
	_screen.create(width, height, Graphics::PixelFormat::createFormatCLUT8());

	// Create overlay surface (same size, 8bpp for simplicity)
	_overlay.create(width, height, Graphics::PixelFormat::createFormatCLUT8());
}

int16 OSystem_RP2350::getHeight() {
	return _screenHeight;
}

int16 OSystem_RP2350::getWidth() {
	return _screenWidth;
}

PaletteManager *OSystem_RP2350::getPaletteManager() {
	return this;
}

void OSystem_RP2350::setPalette(const byte *colors, uint start, uint num) {
	memcpy(_palette + start * 3, colors, num * 3);
	_paletteDirty = true;
}

void OSystem_RP2350::grabPalette(byte *colors, uint start, uint num) {
	memcpy(colors, _palette + start * 3, num * 3);
}

void OSystem_RP2350::updatePalette() {
	if (_paletteDirty) {
		cabal_set_palette(_palette, 0, 256);
		_paletteDirty = false;
	}
}

void OSystem_RP2350::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	if (!_screen.getPixels()) return;

	const byte *src = (const byte *)buf;
	byte *dst = (byte *)_screen.getBasePtr(x, y);
	int dstPitch = _screen.getPitch();

	for (int row = 0; row < h; row++) {
		memcpy(dst, src, w);
		src += pitch;
		dst += dstPitch;
	}
}

Graphics::Surface *OSystem_RP2350::lockScreen() {
	return &_screen;
}

void OSystem_RP2350::unlockScreen() {
	// Nothing to do
}

void OSystem_RP2350::fillScreen(uint32 col) {
	if (_screen.getPixels()) {
		memset(_screen.getPixels(), col, _screen.getWidth() * _screen.getHeight());
	}
}

void OSystem_RP2350::updateScreen() {
	// Update palette if needed
	updatePalette();

	// Copy to minimal backend
	CabalSurface *cabalScreen = cabal_lock_screen();
	if (cabalScreen && cabalScreen->pixels && _screen.getPixels()) {
		const byte *src;
		if (_overlayVisible && _overlay.getPixels()) {
			src = (const byte *)_overlay.getPixels();
		} else {
			src = (const byte *)_screen.getPixels();
		}
		memcpy(cabalScreen->pixels, src, _screenWidth * _screenHeight);
	}
	cabal_unlock_screen();

	// Update cursor position
	cabal_set_mouse_pos(_mouseX, _mouseY);

	// Push to display
	cabal_update_screen();
}

void OSystem_RP2350::setShakePos(int shakeOffset) {
	_shakeOffset = shakeOffset;
}

// Overlay

void OSystem_RP2350::showOverlay() {
	_overlayVisible = true;
	cabal_show_overlay();
}

void OSystem_RP2350::hideOverlay() {
	_overlayVisible = false;
	cabal_hide_overlay();
}

Graphics::PixelFormat OSystem_RP2350::getOverlayFormat() const {
	return Graphics::PixelFormat::createFormatCLUT8();
}

void OSystem_RP2350::clearOverlay() {
	if (_overlay.getPixels()) {
		// Copy screen to overlay
		memcpy(_overlay.getPixels(), _screen.getPixels(), _screenWidth * _screenHeight);
	}
}

void OSystem_RP2350::grabOverlay(void *buf, int pitch) {
	if (!_overlay.getPixels()) return;

	byte *dst = (byte *)buf;
	const byte *src = (const byte *)_overlay.getPixels();

	for (int y = 0; y < _screenHeight; y++) {
		memcpy(dst, src, _screenWidth);
		dst += pitch;
		src += _overlay.getPitch();
	}
}

void OSystem_RP2350::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	if (!_overlay.getPixels()) return;

	const byte *src = (const byte *)buf;
	byte *dst = (byte *)_overlay.getBasePtr(x, y);
	int dstPitch = _overlay.getPitch();

	for (int row = 0; row < h; row++) {
		memcpy(dst, src, w);
		src += pitch;
		dst += dstPitch;
	}
}

int16 OSystem_RP2350::getOverlayHeight() {
	return _screenHeight;
}

int16 OSystem_RP2350::getOverlayWidth() {
	return _screenWidth;
}

// Mouse

bool OSystem_RP2350::showMouse(bool visible) {
	bool prev = _mouseVisible;
	_mouseVisible = visible;
	cabal_show_mouse(visible);
	return prev;
}

void OSystem_RP2350::warpMouse(int x, int y) {
	_mouseX = x;
	_mouseY = y;
	cabal_set_mouse_pos(x, y);
}

void OSystem_RP2350::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY,
                                    uint32 keycolor, bool dontScale,
                                    const Graphics::PixelFormat *format) {
	if (_cursor.data) {
		delete[] _cursor.data;
	}

	_cursor.w = w;
	_cursor.h = h;
	_cursor.hotX = hotspotX;
	_cursor.hotY = hotspotY;
	_cursor.keycolor = keycolor;
	_cursor.data = new byte[w * h];
	memcpy(_cursor.data, buf, w * h);

	cabal_set_mouse_cursor(_cursor.data, w, h, hotspotX, hotspotY, (uint8)keycolor);
}

// Events

uint32 OSystem_RP2350::getMillis() {
	return cabal_get_millis();
}

void OSystem_RP2350::delayMillis(uint msecs) {
	cabal_delay(msecs);
}

void OSystem_RP2350::getTimeAndDate(TimeDate &t) const {
	// Return a default time (no RTC on board)
	t.tm_sec = 0;
	t.tm_min = 0;
	t.tm_hour = 12;
	t.tm_mday = 1;
	t.tm_mon = 0;
	t.tm_year = 125;  // 2025
	t.tm_wday = 0;
}

bool OSystem_RP2350::pollEvent(Common::Event &event) {
	CabalEvent cabalEvent;
	if (!cabal_poll_event(&cabalEvent)) {
		return false;
	}

	switch (cabalEvent.type) {
	case CABAL_EVENT_KEYDOWN:
		event.type = Common::EVENT_KEYDOWN;
		event.kbd.keycode = (Common::KeyCode)cabalEvent.kbd.keycode;
		event.kbd.ascii = cabalEvent.kbd.ascii;
		event.kbd.flags = 0;
		if (cabalEvent.kbd.flags & CABAL_MOD_SHIFT) event.kbd.flags |= Common::KBD_SHIFT;
		if (cabalEvent.kbd.flags & CABAL_MOD_CTRL) event.kbd.flags |= Common::KBD_CTRL;
		if (cabalEvent.kbd.flags & CABAL_MOD_ALT) event.kbd.flags |= Common::KBD_ALT;
		return true;

	case CABAL_EVENT_KEYUP:
		event.type = Common::EVENT_KEYUP;
		event.kbd.keycode = (Common::KeyCode)cabalEvent.kbd.keycode;
		event.kbd.ascii = cabalEvent.kbd.ascii;
		event.kbd.flags = 0;
		if (cabalEvent.kbd.flags & CABAL_MOD_SHIFT) event.kbd.flags |= Common::KBD_SHIFT;
		if (cabalEvent.kbd.flags & CABAL_MOD_CTRL) event.kbd.flags |= Common::KBD_CTRL;
		if (cabalEvent.kbd.flags & CABAL_MOD_ALT) event.kbd.flags |= Common::KBD_ALT;
		return true;

	case CABAL_EVENT_MOUSEMOVE:
		event.type = Common::EVENT_MOUSEMOVE;
		event.mouse.x = _mouseX = cabalEvent.mouse.x;
		event.mouse.y = _mouseY = cabalEvent.mouse.y;
		return true;

	case CABAL_EVENT_LBUTTONDOWN:
		event.type = Common::EVENT_LBUTTONDOWN;
		event.mouse.x = _mouseX = cabalEvent.mouse.x;
		event.mouse.y = _mouseY = cabalEvent.mouse.y;
		return true;

	case CABAL_EVENT_LBUTTONUP:
		event.type = Common::EVENT_LBUTTONUP;
		event.mouse.x = _mouseX = cabalEvent.mouse.x;
		event.mouse.y = _mouseY = cabalEvent.mouse.y;
		return true;

	case CABAL_EVENT_RBUTTONDOWN:
		event.type = Common::EVENT_RBUTTONDOWN;
		event.mouse.x = _mouseX = cabalEvent.mouse.x;
		event.mouse.y = _mouseY = cabalEvent.mouse.y;
		return true;

	case CABAL_EVENT_RBUTTONUP:
		event.type = Common::EVENT_RBUTTONUP;
		event.mouse.x = _mouseX = cabalEvent.mouse.x;
		event.mouse.y = _mouseY = cabalEvent.mouse.y;
		return true;

	case CABAL_EVENT_QUIT:
		event.type = Common::EVENT_QUIT;
		_quitRequested = true;
		return true;

	default:
		return false;
	}
}

// Mutex - Dummy implementation (single-threaded)

OSystem::MutexRef OSystem_RP2350::createMutex() {
	return (MutexRef)1;  // Dummy non-null pointer
}

void OSystem_RP2350::lockMutex(MutexRef mutex) {
	// Single-threaded, no-op
}

void OSystem_RP2350::unlockMutex(MutexRef mutex) {
	// Single-threaded, no-op
}

void OSystem_RP2350::deleteMutex(MutexRef mutex) {
	// Single-threaded, no-op
}

// Audio

Audio::Mixer *OSystem_RP2350::getMixer() {
	return _mixer;
}

// Misc

void OSystem_RP2350::quit() {
	_quitRequested = true;
}

void OSystem_RP2350::displayMessageOnOSD(const char *msg) {
	printf("OSD: %s\n", msg);
}

void OSystem_RP2350::logMessage(LogMessageType::Type type, const char *message) {
	printf("%s\n", message);
}
