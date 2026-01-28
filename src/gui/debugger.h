/* Cabal - Legacy Game Implementations
 *
 * Minimal GUI Debugger stub for embedded platforms
 */

#ifndef GUI_DEBUGGER_H
#define GUI_DEBUGGER_H

#include "common/scummsys.h"
#include "common/str.h"

namespace GUI {

/**
 * Minimal debugger class for embedded platforms.
 * This is a stub that provides the interface without full functionality.
 */
class Debugger {
public:
	Debugger() : _isActive(false) {}
	virtual ~Debugger() {}

	// Returns true if debugger is currently active
	bool isActive() const { return _isActive; }

	// Attach debugger with optional error message
	void attach(const char *entry = nullptr) { _isActive = true; }

	// Called each frame when debugger is active
	void onFrame() {}

	// Detach the debugger
	void detach() { _isActive = false; }

	// Print debug output (public for external callers)
	void debugPrintf(const char *format, ...) {
		// Could redirect to printf on embedded if desired
	}

protected:
	// Register a command handler - simplified for embedded
	typedef bool (*CommandProc)(int argc, const char **argv);

	void registerCmd(const char *cmdName, CommandProc proc, const char *description = nullptr) {
		// Stub - commands not implemented on embedded
	}

	// Template version used by ScummVM console classes
	template<class T>
	void registerCmd(const char *cmdName, bool (T::*method)(int argc, const char **argv),
	                 const char *description = nullptr) {
		// Stub - commands not implemented on embedded
	}

private:
	bool _isActive;
};

} // End of namespace GUI

// WRAP_METHOD macro for registering debug commands
// On embedded, this is a no-op since the debugger is stubbed
#define WRAP_METHOD(cls, method) \
	(bool (cls::*)(int, const char**))(&cls::method)

#endif // GUI_DEBUGGER_H
