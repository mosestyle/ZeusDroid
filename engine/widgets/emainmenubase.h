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
    // Also draws the game logos "Zeus + Poseidon" in the sky at the top left
    // (the original shows its Zeus or Poseidon logo there); the logo sprites
    // are read at runtime from the player's own DATA/PoseidonImps.sg3.
    void paintEvent(ePainter& p) override;
private:
    void paintLogos(ePainter& p, const SDL_Rect& bg);
};

#endif // EMAINMENUBASE_H
