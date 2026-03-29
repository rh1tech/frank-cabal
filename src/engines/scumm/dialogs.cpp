/* Cabal - Legacy Game Implementations
 *
 * Minimal SCUMM dialogs stub for embedded platforms.
 * The full GUI dialog system is not available on embedded,
 * so we provide stub implementations that print to console.
 */

#include "common/config-manager.h"
#include "common/system.h"
#include "common/events.h"
#include "common/localization.h"
#include "common/translation.h"

#include "gui/gui-manager.h"

#include "scumm/dialogs.h"
#include "scumm/sound.h"
#include "scumm/scumm.h"
#include "scumm/imuse/imuse.h"
#include "scumm/imuse_digi/dimuse.h"
#include "scumm/verbs.h"

#ifndef DISABLE_HELP
#include "scumm/help.h"
#endif

namespace Scumm {

struct ResString {
	int num;
	char string[80];
};

static const ResString string_map_table_v8[] = {
	{0, "/BT_100/Please insert disk %d. Press ENTER"},
	{0, "/BT__003/Unable to Find %s, (%s %d) Press Button."},
	{0, "/BT__004/Error reading disk %c, (%c%d) Press Button."},
	{0, "/BT__002/Game Paused.  Press SPACE to Continue."},
	{0, "/BT__005/Are you sure you want to restart?  (Y/N)"},
	{0, "/BT__006/Are you sure you want to quit?  (Y/N)"},
	{0, "/BT__008/Save"},
	{0, "/BT__009/Load"},
	{0, "/BT__010/Play"},
	{0, "/BT__011/Cancel"},
	{0, "/BT__012/Quit"},
	{0, "/BT__013/OK"},
	{0, ""},
	{0, "/BT__014/You must enter a name"},
	{0, "/BT__015/The game was NOT saved (disk full?)"},
	{0, "/BT__016/The game was NOT loaded"},
	{0, "/BT__017/Saving '%s'"},
	{0, "/BT__018/Loading '%s'"},
	{0, "/BT__019/Name your SAVE game"},
	{0, "/BT__020/Select a game to LOAD"}
};

static const ResString string_map_table_v7[] = {
	{96, "game name and version"},
	{83, "Unable to Find %s, (%c%d) Press Button."},
	{84, "Error reading disk %c, (%c%d) Press Button."},
	{85, "/BOOT.003/Game Paused.  Press SPACE to Continue."},
	{86, "/BOOT.004/Are you sure you want to restart?  (Y/N)"},
	{87, "/BOOT.005/Are you sure you want to quit?  (Y/N)"},
	{70, "/BOOT.008/Save"},
	{71, "/BOOT.009/Load"},
	{72, "/BOOT.010/Play"},
	{73, "/BOOT.011/Cancel"},
	{74, "/BOOT.012/Quit"},
	{75, "/BOOT.013/OK"},
	{0, ""},
	{78, "/BOOT.014/You must enter a name"},
	{81, "/BOOT.015/The game was NOT saved (disk full?)"},
	{82, "/BOOT.016/The game was NOT loaded"},
	{79, "/BOOT.017/Saving '%s'"},
	{80, "/BOOT.018/Loading '%s'"},
	{76, "/BOOT.019/Name your SAVE game"},
	{77, "/BOOT.020/Select a game to LOAD"}
};

static const ResString string_map_table_v6[] = {
	{90, "Insert Disk %c and Press Button to Continue."},
	{91, "Unable to Find %s, (%c%d) Press Button."},
	{92, "Error reading disk %c, (%c%d) Press Button."},
	{93, "Game Paused.  Press SPACE to Continue."},
	{94, "Are you sure you want to restart?  (Y/N)"},
	{95, "Are you sure you want to quit?  (Y/N)"},
	{96, "Save"},
	{97, "Load"},
	{98, "Play"},
	{99, "Cancel"},
	{100, "Quit"},
	{101, "OK"},
	{102, "Insert save/load game disk"},
	{103, "You must enter a name"},
	{104, "The game was NOT saved (disk full?)"},
	{105, "The game was NOT loaded"},
	{106, "Saving '%s'"},
	{107, "Loading '%s'"},
	{108, "Name your SAVE game"},
	{109, "Select a game to LOAD"},
	{117, "How may I serve you?"}
};

static const ResString string_map_table_v345[] = {
	{1, "Insert Disk %c and Press Button to Continue."},
	{2, "Unable to Find %s, (%c%d) Press Button."},
	{3, "Error reading disk %c, (%c%d) Press Button."},
	{4, "Game Paused.  Press SPACE to Continue."},
	{5, "Are you sure you want to restart?  (Y/N)Y"},
	{6, "Are you sure you want to quit?  (Y/N)Y"},
	{7, "Save"},
	{8, "Load"},
	{9, "Play"},
	{10, "Cancel"},
	{11, "Quit"},
	{12, "OK"},
	{13, "Insert save/load game disk"},
	{14, "You must enter a name"},
	{15, "The game was NOT saved (disk full?)"},
	{16, "The game was NOT loaded"},
	{17, "Saving '%s'"},
	{18, "Loading '%s'"},
	{19, "Name your SAVE game"},
	{20, "Select a game to LOAD"},
	{28, "Game title)"}
};

#pragma mark -

ScummDialog::ScummDialog(int x, int y, int w, int h) : GUI::Dialog(x, y, w, h) {
	_backgroundType = GUI::ThemeEngine::kDialogBackgroundSpecial;
}

ScummDialog::ScummDialog(String name) : GUI::Dialog(name) {
	_backgroundType = GUI::ThemeEngine::kDialogBackgroundSpecial;
}

#ifndef DISABLE_HELP

ScummMenuDialog::ScummMenuDialog(ScummEngine *scumm)
	: MainMenuDialog(scumm) {
	_helpDialog = nullptr;
}

ScummMenuDialog::~ScummMenuDialog() {
	delete _helpDialog;
}

void ScummMenuDialog::handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) {
	MainMenuDialog::handleCommand(sender, cmd, data);
}

#endif

#pragma mark -

InfoDialog::InfoDialog(ScummEngine *scumm, int res)
: ScummDialog(0, 0, 0, 0), _vm(scumm), _text(nullptr) {
	_message = queryResString(res);
	printf("InfoDialog: %s\n", _message.c_str());
}

InfoDialog::InfoDialog(ScummEngine *scumm, const String& message)
: ScummDialog(0, 0, 0, 0), _vm(scumm), _text(nullptr) {
	_message = message;
	printf("InfoDialog: %s\n", _message.c_str());
}

void InfoDialog::setInfoText(const String& message) {
	_message = message;
}

void InfoDialog::reflowLayout() {
}

const Common::String InfoDialog::queryResString(int stringno) {
	byte buf[256];
	const byte *result;

	if (stringno == 0)
		return String();

	if (_vm->_game.heversion >= 80)
		return string_map_table_v6[stringno - 1].string;
	else if (_vm->_game.version == 8)
		result = (const byte *)string_map_table_v8[stringno - 1].string;
	else if (_vm->_game.version == 7)
		result = _vm->getStringAddressVar(string_map_table_v7[stringno - 1].num);
	else if (_vm->_game.version == 6)
		result = _vm->getStringAddressVar(string_map_table_v6[stringno - 1].num);
	else if (_vm->_game.version >= 3)
		result = _vm->getStringAddress(string_map_table_v345[stringno - 1].num);
	else
		return string_map_table_v345[stringno - 1].string;

	if (result && *result == '/') {
		_vm->translateText(result, buf);
		result = buf;
	}

	if (!result || *result == '\0') {
		return string_map_table_v345[stringno - 1].string;
	}

	byte chr;
	String tmp;
	while ((chr = *result++)) {
		if (chr == 0xFF) {
			result += 3;
		} else if (chr != '@') {
			tmp += chr;
		}
	}
	return tmp;
}

#pragma mark -

PauseDialog::PauseDialog(ScummEngine *scumm, int res)
	: InfoDialog(scumm, res) {
}

void PauseDialog::handleKeyDown(Common::KeyState state) {
	if (state.ascii == ' ')
		close();
}

ConfirmDialog::ConfirmDialog(ScummEngine *scumm, int res)
	: InfoDialog(scumm, res), _yesKey('y'), _noKey('n') {

	if (_message.lastChar() != ')') {
		_yesKey = _message.lastChar();
		_message.deleteLastChar();

		if (_yesKey >= 'A' && _yesKey <= 'Z')
			_yesKey += 'a' - 'A';
	}
}

void ConfirmDialog::handleKeyDown(Common::KeyState state) {
	Common::KeyCode keyYes, keyNo;
	Common::getLanguageYesNo(keyYes, keyNo);

	if (state.keycode == Common::KEYCODE_n || state.ascii == _noKey || state.ascii == keyNo) {
		setResult(0);
		close();
	} else if (state.keycode == Common::KEYCODE_y || state.ascii == _yesKey || state.ascii == keyYes) {
		setResult(1);
		close();
	}
}

#pragma mark -

ValueDisplayDialog::ValueDisplayDialog(const Common::String& label, int minVal, int maxVal,
		int val, uint16 incKey, uint16 decKey)
	: GUI::Dialog(0, 0, 0, 0),
	_label(label), _min(minVal), _max(maxVal),
	_value(val), _incKey(incKey), _decKey(decKey) {
	assert(_min <= _value && _value <= _max);
}

void ValueDisplayDialog::drawDialog() {
	// Stub - no GUI on embedded
}

void ValueDisplayDialog::handleTickle() {
	if (g_system->getMillis() > _timer) {
		close();
	}
}

void ValueDisplayDialog::reflowLayout() {
}

void ValueDisplayDialog::handleKeyDown(Common::KeyState state) {
	if (state.ascii == _incKey || state.ascii == _decKey) {
		if (state.ascii == _incKey && _value < _max)
			_value++;
		else if (state.ascii == _decKey && _value > _min)
			_value--;
		setResult(_value);
		_timer = g_system->getMillis() + kDisplayDelay;
	} else {
		close();
	}
}

void ValueDisplayDialog::open() {
	setResult(_value);
	_timer = g_system->getMillis() + kDisplayDelay;
}

SubtitleSettingsDialog::SubtitleSettingsDialog(ScummEngine *scumm, int value)
	: InfoDialog(scumm, ""), _value(value) {
}

void SubtitleSettingsDialog::handleTickle() {
	if (g_system->getMillis() > _timer)
		close();
}

void SubtitleSettingsDialog::handleKeyDown(Common::KeyState state) {
	if (state.keycode == Common::KEYCODE_t && state.hasFlags(Common::KBD_CTRL)) {
		cycleValue();
	} else {
		close();
	}
}

void SubtitleSettingsDialog::open() {
	cycleValue();
	setResult(_value);
}

void SubtitleSettingsDialog::cycleValue() {
	static const char *const subtitleDesc[] = {
		"Speech Only",
		"Speech and Subtitles",
		"Subtitles Only"
	};

	_value += 1;
	if (_value > 2)
		_value = 0;

	setInfoText(subtitleDesc[_value]);
	_timer = g_system->getMillis() + 1500;
}

Indy3IQPointsDialog::Indy3IQPointsDialog(ScummEngine *scumm, char* text)
	: InfoDialog(scumm, text) {
}

void Indy3IQPointsDialog::handleKeyDown(Common::KeyState state) {
	if (state.ascii == 'i')
		close();
}

DebugInputDialog::DebugInputDialog(ScummEngine *scumm, char* text)
	: InfoDialog(scumm, text) {
	mainText = text;
	done = 0;
}

void DebugInputDialog::handleKeyDown(Common::KeyState state) {
	if (state.keycode == Common::KEYCODE_BACKSPACE && buffer.size() > 0) {
		buffer.deleteLastChar();
		Common::String total = mainText + ' ' + buffer;
		setInfoText(total);
	} else if (state.keycode == Common::KEYCODE_RETURN) {
		done = 1;
		close();
		return;
	} else if ((state.ascii >= '0' && state.ascii <= '9') || (state.ascii >= 'A' && state.ascii <= 'Z') || (state.ascii >= 'a' && state.ascii <= 'z') || state.ascii == '.' || state.ascii == ' ') {
		buffer += state.ascii;
		Common::String total = mainText + ' ' + buffer;
		setInfoText(total);
	}
}

LoomTownsDifficultyDialog::LoomTownsDifficultyDialog()
    : Dialog("LoomTownsDifficultyDialog"), _difficulty(-1) {
}

void LoomTownsDifficultyDialog::handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) {
	// Stub - just close with standard difficulty
	_difficulty = 1;
	close();
}

} // End of namespace Scumm
