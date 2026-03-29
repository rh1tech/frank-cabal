/* Cabal - Legacy Game Implementations
 *
 * Minimal About Dialog stub for embedded platforms
 */

#ifndef GUI_ABOUT_H
#define GUI_ABOUT_H

#include "gui/dialog.h"

namespace GUI {

class AboutDialog : public Dialog {
public:
    AboutDialog() : Dialog(0, 0, 0, 0) {}
    virtual ~AboutDialog() {}
};

} // End of namespace GUI

#endif // GUI_ABOUT_H
