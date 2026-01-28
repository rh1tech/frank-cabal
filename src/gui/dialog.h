/* Cabal - Legacy Game Implementations
 *
 * Minimal GUI Dialog stub for embedded platforms
 */

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include "common/scummsys.h"
#include "common/str.h"

namespace GUI {

class CommandSender;

// Dialog result codes
enum {
	kMessageOK = 1,
	kMessageCancel = 0
};

/**
 * Minimal dialog class for embedded platforms.
 * This is a stub that provides the interface without full functionality.
 */
class Dialog {
public:
	Dialog(int x, int y, int w, int h) {}
	Dialog(const Common::String &name) {}
	virtual ~Dialog() {}

	// Run the dialog modally - returns immediately on embedded
	virtual int runModal() { return kMessageOK; }

	// Handle a command from a widget
	virtual void handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {}

	// Reflow layout after resize
	virtual void reflowLayout() {}

	// Close the dialog
	void close() {}

	// Check if dialog is visible
	bool isVisible() const { return false; }
};

/**
 * Command sender interface
 */
class CommandSender {
public:
	virtual ~CommandSender() {}
};

/**
 * Widget base class stub
 */
class Widget {
public:
	Widget(Dialog *parent, int x, int y, int w, int h) {}
	virtual ~Widget() {}
};

class ButtonWidget : public Widget, public CommandSender {
public:
	ButtonWidget(Dialog *parent, int x, int y, int w, int h, const Common::String &label, uint32 cmd = 0)
		: Widget(parent, x, y, w, h) {}
	virtual ~ButtonWidget() {}
};

class GraphicsWidget : public Widget {
public:
	GraphicsWidget(Dialog *parent, int x, int y, int w, int h)
		: Widget(parent, x, y, w, h) {}
	virtual ~GraphicsWidget() {}
};

} // End of namespace GUI

#endif // GUI_DIALOG_H
