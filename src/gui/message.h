/* Cabal - Legacy Game Implementations
 *
 * Minimal Message Dialog stub for embedded platforms
 */

#ifndef GUI_MESSAGE_H
#define GUI_MESSAGE_H

#include "common/scummsys.h"
#include "common/str.h"
#include "gui/dialog.h"

namespace GUI {

/**
 * Message dialog stub - just prints to console on embedded
 */
class MessageDialog : public Dialog {
public:
    MessageDialog(const Common::String &message, const char *defaultButton = "OK", const char *altButton = nullptr)
        : Dialog(0, 0, 0, 0) {
        // On embedded, just print the message
        printf("MessageDialog: %s\n", message.c_str());
    }
    virtual ~MessageDialog() {}
};

} // End of namespace GUI

#endif // GUI_MESSAGE_H
