#include "emainmenubase.h"

#include "textures/egametextures.h"
#include "platform/eplatform.h"
#include "epainter.h"

void eMainMenuBase::initialize() {
    const auto& intrfc = eGameTextures::interface();
    const auto res = resolution();
    const int iRes = static_cast<int>(res.uiScale());
    const auto& texs = intrfc[iRes];
    setTexture(texs.fMainMenuImage);
}

void eMainMenuBase::paintEvent(ePainter& p) {
    const double f = ePlatform::logicalFactor();
    const auto& tex = texture();
    if(f >= 0.999 || !tex || !tex->tex()) return eLabel::paintEvent(p);
    const int w = static_cast<int>(tex->width()/f);
    const int h = static_cast<int>(tex->height()/f);
    const SDL_Rect dst{x() + (width() - w)/2, y() + (height() - h)/2, w, h};
    SDL_RenderCopy(p.renderer(), tex->tex(), nullptr, &dst);
}
