/* Cabal - Legacy Game Implementations
 *
 * Minimal GUI SaveLoad stub for embedded platforms
 */

#ifndef GUI_SAVELOAD_H
#define GUI_SAVELOAD_H

#include "common/scummsys.h"
#include "common/str.h"

namespace GUI {

/**
 * Minimal SaveLoadChooser for embedded platforms.
 * Opens a simple slot-based save/load dialog.
 */
class SaveLoadChooser {
public:
	SaveLoadChooser(const Common::String &title, const Common::String &buttonLabel, bool saveMode = true)
		: _title(title), _buttonLabel(buttonLabel), _saveMode(saveMode), _resultSlot(-1) {}
	virtual ~SaveLoadChooser() {}

	// Run the dialog and return the selected slot (-1 = cancelled)
	int runModalWithCurrentTarget() {
		// On embedded, just return slot 0 for save, or first available for load
		// A real implementation would show a menu
		return _saveMode ? 0 : 0;
	}

	// Get the description/string result
	const Common::String &getResultString() const {
		return _resultString;
	}

	// Create a default save description
	Common::String createDefaultSaveDescription(int slot) const {
		char buf[32];
		snprintf(buf, sizeof(buf), "Save %d", slot);
		return Common::String(buf);
	}

private:
	Common::String _title;
	Common::String _buttonLabel;
	Common::String _resultString;
	bool _saveMode;
	int _resultSlot;
};

} // End of namespace GUI

#endif // GUI_SAVELOAD_H
