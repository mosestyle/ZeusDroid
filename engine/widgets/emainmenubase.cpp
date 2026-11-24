#include "emainmenubase.h"

#include "textures/egametextures.h"
#include "platform/eplatform.h"
#include "epainter.h"
#include "egamedir.h"
#include "etexture.h"
#include "efonts.h"
#include "texbuild/esg3file.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <vector>

namespace {
struct eLogos {
    bool fTried = false;
    std::vector<std::shared_ptr<eTexture>> fZeus;
    std::vector<std::shared_ptr<eTexture>> fPoseidon;
    std::shared_ptr<eTexture> fPlus;
};
// Never destroyed: its textures must not be freed after the renderer at exit.
eLogos& gLogos = *new eLogos;

std::string findDataFile(const std::string& name) {
    namespace fs = std::filesystem;
    const auto dir = eGameDir::path("DATA");
    std::error_code ec;
    if(fs::exists(dir + "/" + name, ec)) return dir + "/" + name;
    std::string lname = name;
    std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
    for(const auto& e : fs::directory_iterator(dir, ec)) {
        std::string n = e.path().filename().string();
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        if(n == lname) return e.path().string();
    }
    return "";
}

// Frames of the animated logos in PoseidonImps.sg3 (bitmaps "ZeusLogo",
// 4 frames 108x64, and "Poseidonlogo", 3 frames 112x54). The record
// sizes are checked, so another file version just shows no logos.
void loadLogoFrames(SDL_Renderer* const r, eSg3File& f, const int first,
                    const int n, const int w0, const int h0,
                    std::vector<std::shared_ptr<eTexture>>& out) {
    for(int i = first; i < first + n; i++) {
        int w, h;
        if(!f.recordSize(i, w, h) || std::abs(w - w0) > 2 || h != h0) {
            out.clear();
            return;
        }
        eSg3Image im;
        if(!f.decode(i, im, true) || im.empty()) {
            out.clear();
            return;
        }
        SDL_Surface* const s = SDL_CreateRGBSurfaceWithFormatFrom(
                    im.fRGBA.data(), im.fW, im.fH, 32, im.fW*4,
                    SDL_PIXELFORMAT_RGBA32);
        if(!s) {
            out.clear();
            return;
        }
        const auto t = std::make_shared<eTexture>();
        const bool ok = t->load(r, s); // load() frees the surface
        if(!ok) {
            out.clear();
            return;
        }
        out.push_back(t);
    }
}

void loadLogos(SDL_Renderer* const r, const eResolution& res) {
    if(gLogos.fTried) return;
    gLogos.fTried = true;
    const auto path = findDataFile("PoseidonImps.sg3");
    if(path.empty()) return;
    eSg3File f;
    if(!f.open(path)) return;
    loadLogoFrames(r, f, 8531, 4, 108, 64, gLogos.fZeus);
    loadLogoFrames(r, f, 8528, 3, 112, 54, gLogos.fPoseidon);
    if(gLogos.fZeus.empty() || gLogos.fPoseidon.empty()) return;
    const auto font = eFonts::defaultFont(res.hugeFontSize());
    if(!font) return;
    gLogos.fPlus = std::make_shared<eTexture>();
    if(!gLogos.fPlus->loadText(r, "+", eFontColor::light, *font)) {
        gLogos.fPlus.reset();
    }
}
}

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
    if(!tex || !tex->tex()) return eLabel::paintEvent(p);
    const double ff = f >= 0.999 ? 1. : f;
    const int w = static_cast<int>(tex->width()/ff);
    const int h = static_cast<int>(tex->height()/ff);
    const SDL_Rect dst{x() + (width() - w)/2, y() + (height() - h)/2, w, h};
    if(f >= 0.999) {
        eLabel::paintEvent(p);
    } else {
        SDL_RenderCopy(p.renderer(), tex->tex(), nullptr, &dst);
    }
    paintLogos(p, dst);
}

void eMainMenuBase::paintLogos(ePainter& p, const SDL_Rect& bg) {
    const auto r = p.renderer();
    loadLogos(r, resolution());
    if(gLogos.fZeus.empty() || gLogos.fPoseidon.empty()) return;
    // The menu picture is 1024 x 768 in the original; the logos are drawn in
    // its sky, left of the dialogs: Zeus  +  Poseidon.
    const double s = bg.h/768.;
    const auto frame = [](const std::vector<std::shared_ptr<eTexture>>& v) {
        const int i = static_cast<int>(SDL_GetTicks()/150) % static_cast<int>(v.size());
        return v[i];
    };
    const auto zt = frame(gLogos.fZeus);
    const auto pt = frame(gLogos.fPoseidon);
    const int zw = static_cast<int>(zt->width()*s);
    const int zh = static_cast<int>(zt->height()*s);
    const int pw = static_cast<int>(pt->width()*s);
    const int ph = static_cast<int>(pt->height()*s);
    int plusW = 0, plusH = 0;
    if(gLogos.fPlus) {
        plusH = std::min(static_cast<int>(gLogos.fPlus->height()*s), zh);
        plusW = gLogos.fPlus->width()*plusH/std::max(1, gLogos.fPlus->height());
    }
    const int gap = static_cast<int>(6*s);
    const int x0 = bg.x + static_cast<int>(18*s);
    const int y0 = bg.y + static_cast<int>(135*s);
    const SDL_Rect zr{x0, y0, zw, zh};
    SDL_RenderCopy(r, zt->tex(), nullptr, &zr);
    int x = x0 + zw + gap;
    if(gLogos.fPlus) {
        const SDL_Rect pr{x, y0 + (zh - plusH)/2, plusW, plusH};
        SDL_RenderCopy(r, gLogos.fPlus->tex(), nullptr, &pr);
        x += plusW + gap;
    }
    const SDL_Rect por{x, y0 + (zh - ph)/2, pw, ph};
    SDL_RenderCopy(r, pt->tex(), nullptr, &por);
}
