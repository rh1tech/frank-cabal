/* Cabal - Legacy Game Implementations
 *
 * Minimal GUI Message Dialog stub for embedded platforms
 */

#ifndef GUI_MESSAGE_H
#define GUI_MESSAGE_H

#include "gui/dialog.h"
#include "common/str.h"

namespace GUI {

/**
 * Minimal message dialog for embedded platforms.
 * Just prints to console instead of showing a GUI dialog.
 */
class MessageDialog : public Dialog {
public:
	MessageDialog(const Common::String &message,
	              const Common::String &defaultButton = "OK",
	              const Common::String &altButton = "")
		: Dialog("MessageDialog") {
		// On embedded platforms, just print to console
		printf("MESSAGE: %s\n", message.c_str());
	}

	virtual ~MessageDialog() {}

	virtual int runModal() {
		// Return OK immediately
		return kMessageOK;
	}
};

} // End of namespace GUI

#endif // GUI_MESSAGE_H
