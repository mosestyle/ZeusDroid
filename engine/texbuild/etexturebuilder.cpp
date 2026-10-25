#include "etexturebuilder.h"

#include "esg3file.h"
#include "eqoi.h"
#include "eresample.h"
#include "spriteData/espritedata.h"

#include <SDL2/SDL_image.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <vector>

namespace {
struct eTexMapFrame {
    short fFile;  // index into kTexFiles (-1: no frame)
    int fIndex;   // record index in the .sg3
    short fX;     // crop offset in the decoded sprite
    short fY;
    char fShadows; // pure red = shadow
};

struct eTexMapAtlas {
    const char* fName;
    bool fInterface;       // goes into interface.e (all four sizes)
    const eTexMapFrame* fFrames;
    int fCount;
    const std::vector<eSpriteData>* fSd[4]; // 15, 30, 45, 60
};

struct eTexJpg {
    int fSize;          // 15, 30, 45, 60
    const char* fName;  // DATA/<name> (any letter case on disk)
};
}

#include "etexmap.inc"

namespace fs = std::filesystem;

namespace {
int gFrames = 0;
int gMissing = 0;

struct eWriter {
    std::string fPath;
    std::ofstream fOut;
    std::ofstream fIdx;
    uint64_t fPos = 0;
    bool open(const std::string& path) {
        fPath = path;
        fOut.open(path + ".tmp", std::ios::binary | std::ios::trunc);
        fIdx.open(path + ".idx.tmp", std::ios::trunc);
        return fOut.good() && fIdx.good();
    }
    void add(const std::string& name, const std::vector<uint8_t>& data) {
        fOut.write(reinterpret_cast<const char*>(data.data()), data.size());
        fIdx << name << ' ' << fPos << ' ' << data.size() << '\n';
        fPos += data.size();
    }
    bool finish() {
        fOut.close();
        fIdx.close();
        if(!fOut.good() && fOut.fail()) return false;
        std::error_code ec;
        fs::rename(fPath + ".tmp", fPath, ec);
        if(ec) return false;
        fs::rename(fPath + ".idx.tmp", fPath + ".idx", ec);
        return !ec;
    }
    void abort() {
        fOut.close();
        fIdx.close();
        std::error_code ec;
        fs::remove(fPath + ".tmp", ec);
        fs::remove(fPath + ".idx.tmp", ec);
    }
};

const int kSizes[4] = {15, 30, 45, 60};

void blit(std::vector<uint8_t>& page, const int pw, const int ph,
          const uint8_t* src, const int sw, const int sh,
          const int x0, const int y0) {
    for(int y = 0; y < sh; y++) {
        const int py = y0 + y;
        if(py < 0 || py >= ph) continue;
        for(int x = 0; x < sw; x++) {
            const int px = x0 + x;
            if(px < 0 || px >= pw) continue;
            std::memcpy(&page[(size_t(py)*pw + px)*4], &src[(size_t(y)*sw + x)*4], 4);
        }
    }
}
}

namespace {
// DATA/<name> with any letter case.
std::string findCaseless(const std::string& dir, const std::string& name) {
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

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if(!f) return false;
    f.seekg(0, std::ios::end);
    const auto n = f.tellg();
    if(n <= 0) return false;
    f.seekg(0);
    out.resize(size_t(n));
    f.read(reinterpret_cast<char*>(out.data()), n);
    return f.good();
}

// The pictures: 30 = the original file as it is; 15 / 45 / 60 scaled
// (width * size / 30, rounded down) and stored as JPEG again.
bool buildJpg(const std::string& src, const int size, const std::string& tmp,
              std::vector<uint8_t>& out) {
    if(size == 30) return readFile(src, out);
    SDL_Surface* const loaded = IMG_Load(src.c_str());
    if(!loaded) return false;
    SDL_Surface* const rgba = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(loaded);
    if(!rgba) return false;
    const int w = rgba->w;
    const int h = rgba->h;
    std::vector<uint8_t> px(size_t(w)*h*4);
    for(int y = 0; y < h; y++) {
        std::memcpy(&px[size_t(y)*w*4], static_cast<uint8_t*>(rgba->pixels) + size_t(y)*rgba->pitch,
                    size_t(w)*4);
    }
    SDL_FreeSurface(rgba);
    const int dw = w*size/30;
    const int dh = h*size/30;
    if(dw <= 0 || dh <= 0) return false;
    std::vector<uint8_t> scaled;
    eResample::resize(px.data(), w, h, scaled, dw, dh,
                      size < 30 ? eResample::eFilter::triangle : eResample::eFilter::cubic,
                      false);
    SDL_Surface* const surf = SDL_CreateRGBSurfaceWithFormatFrom(
                scaled.data(), dw, dh, 32, dw*4, SDL_PIXELFORMAT_RGBA32);
    if(!surf) return false;
    const int r = IMG_SaveJPG(surf, tmp.c_str(), 92);
    SDL_FreeSurface(surf);
    if(r != 0) return false;
    const bool ok = readFile(tmp, out);
    std::error_code ec;
    fs::remove(tmp, ec);
    return ok;
}
}

bool eTextureBuilder::sourcesPresent(const std::string& dataDir) {
    std::error_code ec;
    return fs::exists(dataDir + "/Zeus_Interface.sg3", ec) &&
           fs::exists(dataDir + "/Zeus_General.sg3", ec) &&
           fs::exists(dataDir + "/SprMain.sg3", ec);
}

bool eTextureBuilder::needed(const std::string& outDir) {
    std::error_code ec;
    const bool iface = fs::exists(outDir + "/interface.e", ec);
    const bool any = fs::exists(outDir + "/i15.e", ec) || fs::exists(outDir + "/i30.e", ec);
    return !iface || !any;
}

int eTextureBuilder::lastFrames() { return gFrames; }
int eTextureBuilder::lastMissing() { return gMissing; }

bool eTextureBuilder::build(const std::string& dataDir, const std::string& outDir,
                            const bool withLarge, const eProgress& progress,
                            std::string& error) {
    gFrames = 0;
    gMissing = 0;
    std::error_code ec;
    fs::create_directories(outDir, ec);
    eWriter iface;
    eWriter maps[4];
    bool ok = iface.open(outDir + "/interface.e");
    for(int s = 0; s < 4 && ok; s++) {
        if(!withLarge && s >= 2) continue;
        ok = maps[s].open(outDir + "/i" + std::to_string(kSizes[s]) + ".e");
    }
    if(!ok) {
        error = "cannot write the texture files (storage full?)";
        iface.abort();
        for(auto& m : maps) m.abort();
        return false;
    }
    std::map<std::string, std::unique_ptr<eSg3File>> files;
    std::vector<uint8_t> scaled;
    std::vector<uint8_t> qoi;
    for(int a = 0; a < kTexMapCount; a++) {
        const auto& atlas = kTexMap[a];
        if(progress) progress(0.95*a/kTexMapCount, atlas.fName);
        const auto openFile = [&](const int id) -> eSg3File* {
            if(id < 0 || id >= kTexFileCount) return nullptr;
            auto& file = files[kTexFiles[id]];
            if(!file) {
                file = std::make_unique<eSg3File>();
                if(!file->open(dataDir + "/" + kTexFiles[id] + ".sg3")) {
                    file->open(dataDir + "/" + kTexFiles[id] + ".SG3");
                }
            }
            return file->isOpen() ? file.get() : nullptr;
        };
        // Crop every frame once (30 = original size).
        const auto& sd30 = *atlas.fSd[1];
        std::vector<eSg3Image> crops(atlas.fCount);
        for(int k = 0; k < atlas.fCount && k < int(sd30.size()); k++) {
            const auto& fr = atlas.fFrames[k];
            const auto& sd = sd30[k];
            if(sd.fTexId < 0 || sd.fW <= 0 || sd.fH <= 0) continue;
            eSg3File* const file = fr.fFile < 0 ? nullptr : openFile(fr.fFile);
            if(!file) { gMissing++; continue; }
            eSg3Image img;
            if(!file->decode(fr.fIndex, img, fr.fShadows != 0)) { gMissing++; continue; }
            auto& c = crops[k];
            c.fW = sd.fW;
            c.fH = sd.fH;
            c.fRGBA.assign(size_t(c.fW)*c.fH*4, 0);
            for(int y = 0; y < c.fH; y++) {
                const int sy = fr.fY + y;
                if(sy < 0 || sy >= img.fH) continue;
                for(int x = 0; x < c.fW; x++) {
                    const int sx = fr.fX + x;
                    if(sx < 0 || sx >= img.fW) continue;
                    std::memcpy(&c.fRGBA[(size_t(y)*c.fW + x)*4],
                                &img.fRGBA[(size_t(sy)*img.fW + sx)*4], 4);
                }
            }
            gFrames++;
        }
        for(int s = 0; s < 4; s++) {
            const bool large = s >= 2;
            if(!atlas.fInterface && large && !withLarge) continue;
            const auto* sdp = atlas.fSd[s];
            if(!sdp || int(sdp->size()) != atlas.fCount) continue;
            const auto& sds = *sdp;
            int maxTex = -1;
            for(const auto& sd : sds) maxTex = std::max(maxTex, sd.fTexId);
            for(int t = 0; t <= maxTex; t++) {
                int pw = 0;
                int ph = 0;
                for(const auto& sd : sds) {
                    if(sd.fTexId != t) continue;
                    pw = std::max(pw, sd.fX + sd.fW);
                    ph = std::max(ph, sd.fY + sd.fH);
                }
                if(pw <= 0 || ph <= 0) continue;
                std::vector<uint8_t> page(size_t(pw)*ph*4, 0);
                for(int k = 0; k < atlas.fCount; k++) {
                    const auto& sd = sds[k];
                    if(sd.fTexId != t || sd.fW <= 0 || sd.fH <= 0) continue;
                    const auto& c = crops[k];
                    if(c.empty()) continue;
                    if(c.fW == sd.fW && c.fH == sd.fH) {
                        blit(page, pw, ph, c.fRGBA.data(), c.fW, c.fH, sd.fX, sd.fY);
                    } else {
                        const auto f = sd.fW < c.fW ? eResample::eFilter::triangle :
                                                      eResample::eFilter::cubic;
                        eResample::resize(c.fRGBA.data(), c.fW, c.fH, scaled,
                                          sd.fW, sd.fH, f, atlas.fInterface);
                        blit(page, pw, ph, scaled.data(), sd.fW, sd.fH, sd.fX, sd.fY);
                    }
                }
                eQoi::encode(page.data(), pw, ph, qoi);
                const std::string name = std::to_string(kSizes[s]) + "/" + atlas.fName +
                                         "_" + std::to_string(t) + ".png";
                auto& w = atlas.fInterface ? iface : maps[s];
                w.add(name, qoi);
            }
        }
    }
    {
        std::vector<uint8_t> jpg;
        const std::string tmp = outDir + "/picture.tmp.jpg";
        for(int j = 0; j < kTexJpgCount; j++) {
            const auto& e = kTexJpgs[j];
            if(progress) progress(0.95 + 0.05*j/kTexJpgCount, e.fName);
            const auto src = findCaseless(dataDir, e.fName);
            if(src.empty() || !buildJpg(src, e.fSize, tmp, jpg)) {
                gMissing++;
                continue;
            }
            iface.add(std::to_string(e.fSize) + "/Zeus_Data_Images/" + e.fName, jpg);
        }
    }
    ok = iface.finish();
    for(int s = 0; s < 4 && ok; s++) {
        if(!withLarge && s >= 2) continue;
        ok = maps[s].finish();
    }
    if(!ok) {
        error = "could not finish writing the texture files";
        return false;
    }
    if(progress) progress(1.0, "done");
    return true;
}
