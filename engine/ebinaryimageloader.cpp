#include "ebinaryimageloader.h"

#include <cstring>
#include <fstream>

#include "esplitbinary.h"
#include "egamedir.h"
#include "texbuild/eqoi.h"

#include <mutex>
#include <vector>

namespace {
// Port (texture builder): packs built on the device come with an index file
// "<pack>.idx" (name position size per line) and QOI pages; they replace the
// compiled-in table of the eZeus packs.
std::once_flag gIdxOnce;
std::map<std::string, eBinaryData> gRuntime;

void loadRuntimeIndex() {
    const std::pair<eFileId, std::string> packs[] = {
        {eFileId::i, eGameDir::iBinaryPath()},
        {eFileId::i15, eGameDir::i15BinaryPath()},
        {eFileId::i30, eGameDir::i30BinaryPath()},
        {eFileId::i45, eGameDir::i45BinaryPath()},
        {eFileId::i60, eGameDir::i60BinaryPath()},
    };
    for(const auto& p : packs) {
        std::ifstream idx(p.second + ".idx");
        if(!idx) continue;
        std::string name;
        long long pos;
        long long size;
        while(idx >> name >> pos >> size) {
            gRuntime[name] = eBinaryData{p.first, static_cast<int>(pos), static_cast<int>(size)};
        }
    }
}
}

std::shared_ptr<eTexture> eBinaryImageLoader::load(SDL_Renderer* const r,
                                                   const std::string& path) {
    std::call_once(gIdxOnce, loadRuntimeIndex);
    const eBinaryData* bdp = nullptr;
    const auto rt = gRuntime.find(path);
    if(rt != gRuntime.end()) {
        bdp = &rt->second;
    } else {
        // A pack with an index never uses the compiled table (offsets differ).
        const auto it = eBinaryDataMap.find(path);
        if(it == eBinaryDataMap.end()) {
            printf("Could not find '%s' image\n", path.c_str());
            return nullptr;
        }
        bdp = &it->second;
        for(const auto& r : gRuntime) {
            if(r.second.fFileId == bdp->fFileId) {
                printf("Image '%s' missing in the built pack\n", path.c_str());
                return nullptr;
            }
        }
    }
    const auto& bd = *bdp;
    std::string epath;
    switch(bd.fFileId) {
    case eFileId::i:
        epath = eGameDir::iBinaryPath();
        break;
    case eFileId::i15:
        epath = eGameDir::i15BinaryPath();
        break;
    case eFileId::i30:
        epath = eGameDir::i30BinaryPath();
        break;
    case eFileId::i45:
        epath = eGameDir::i45BinaryPath();
        break;
    case eFileId::i60:
        epath = eGameDir::i60BinaryPath();
        break;
    }

    std::ifstream file(epath, std::ios::in | std::ios::binary);
    if(!file) {
        printf("Could not open '%s'\n", epath.c_str());
        return nullptr;
    }

    const auto data = new char[bd.fSize];
    file.seekg(bd.fPos);
    file.read(data, bd.fSize);
    file.close();
    if(eQoi::isQoi(reinterpret_cast<const uint8_t*>(data), bd.fSize)) {
        std::vector<uint8_t> rgba;
        int w = 0;
        int h = 0;
        const bool ok = eQoi::decode(reinterpret_cast<const uint8_t*>(data), bd.fSize, rgba, w, h);
        delete[] data;
        if(!ok) {
            printf("Unable to decode image %s\n", path.c_str());
            return nullptr;
        }
        const auto surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
        if(!surf) return nullptr;
        for(int y = 0; y < h; y++) {
            std::memcpy(static_cast<uint8_t*>(surf->pixels) + size_t(y)*surf->pitch,
                        &rgba[size_t(y)*w*4], size_t(w)*4);
        }
        const auto tex = std::make_shared<eTexture>();
        tex->load(r, surf);
        return tex;
    }
    const auto rw = SDL_RWFromMem(data, bd.fSize);
    const auto surf = IMG_Load_RW(rw, SDL_FALSE);
    if(!surf) {
        printf("Unable to load image %s! SDL_image Error: %s\n",
               path.c_str(), IMG_GetError());
        return nullptr;
    }
    delete[] data;

    const auto tex = std::make_shared<eTexture>();
    tex->load(r, surf);

    return tex;
}
