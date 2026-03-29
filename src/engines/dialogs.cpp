/* Cabal - Legacy Game Implementations
 *
 * Minimal MainMenuDialog stub for embedded platforms.
 */

#include "common/system.h"
#include "common/events.h"
#include "gui/gui-manager.h"

#include "engines/dialogs.h"
#include "engines/engine.h"

MainMenuDialog::MainMenuDialog(Engine *engine)
	: GUI::Dialog("GlobalMenu"), _engine(engine),
	  _logo(nullptr), _rtlButton(nullptr), _loadButton(nullptr),
	  _saveButton(nullptr), _helpButton(nullptr),
	  _aboutDialog(nullptr), _optionsDialog(nullptr),
	  _loadDialog(nullptr), _saveDialog(nullptr) {
}

MainMenuDialog::~MainMenuDialog() {
}

void MainMenuDialog::handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kPlayCmd:
		close();
		break;
	case kRTLCmd: {
		Common::Event eventRTL;
		eventRTL.type = Common::EVENT_RTL;
		g_system->getEventManager()->pushEvent(eventRTL);
		close();
		}
		break;
	case kQuitCmd: {
		Common::Event eventQ;
		eventQ.type = Common::EVENT_QUIT;
		g_system->getEventManager()->pushEvent(eventQ);
		close();
		}
		break;
	default:
		GUI::Dialog::handleCommand(sender, cmd, data);
	}
}

void MainMenuDialog::reflowLayout() {
}

void MainMenuDialog::save() {
}

void MainMenuDialog::load() {
}
