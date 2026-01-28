/* Cabal - Legacy Game Implementations
 *
 * Minimal Predictive Dialog stub for embedded platforms
 */

#ifndef GUI_PREDICTIVEDIALOG_H
#define GUI_PREDICTIVEDIALOG_H

#include "gui/dialog.h"
#include "common/str.h"

namespace GUI {

/**
 * Minimal predictive text dialog for embedded platforms.
 * This is a stub that doesn't provide predictive text functionality.
 */
class PredictiveDialog : public Dialog {
public:
	PredictiveDialog() : Dialog("PredictiveDialog") {}
	virtual ~PredictiveDialog() {}

	const char *getResult() const { return _result.c_str(); }
	void setResult(const Common::String &result) { _result = result; }

	virtual int runModal() {
		// No predictive text on embedded - return empty
		return kMessageOK;
	}

private:
	Common::String _result;
};

} // End of namespace GUI

#endif // GUI_PREDICTIVEDIALOG_H
