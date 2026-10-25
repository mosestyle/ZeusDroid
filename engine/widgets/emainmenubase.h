#ifndef EMAINMENUBASE_H
#define EMAINMENUBASE_H

#include "elabel.h"

class eMainMenuBase : public eLabel {
public:
    using eLabel::eLabel;

    void initialize();
protected:
    // Port: with the fitted ("Medium") interface size the menu picture is
    // drawn at the same screen size as with "Large".
    void paintEvent(ePainter& p) override;
};

#endif // EMAINMENUBASE_H
