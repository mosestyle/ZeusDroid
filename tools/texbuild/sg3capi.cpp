// Host-only C API around eSg3File for the Python mapping tool (ctypes).
#include "texbuild/esg3file.h"
#include <cstring>
extern "C" {
void* sg3_open(const char* p) { auto f = new eSg3File; if(!f->open(p)) { delete f; return nullptr; } return f; }
int sg3_count(void* h) { return static_cast<eSg3File*>(h)->imageCount(); }
int sg3_nbmp(void* h) { return static_cast<eSg3File*>(h)->bitmapCount(); }
int sg3_bstart(void* h, int b) { return static_cast<eSg3File*>(h)->bitmapStart(b); }
int sg3_bimages(void* h, int b) { return static_cast<eSg3File*>(h)->bitmapImages(b); }
int sg3_size(void* h, int i, int* w, int* hh) { return static_cast<eSg3File*>(h)->recordSize(i, *w, *hh) ? 1 : 0; }
int sg3_decode(void* h, int i, unsigned char* buf, int cap, int* w, int* hh, int shadows) {
    eSg3Image img;
    if(!static_cast<eSg3File*>(h)->decode(i, img, shadows != 0)) return 0;
    *w = img.fW; *hh = img.fH;
    if(int(img.fRGBA.size()) > cap) return -1;
    std::memcpy(buf, img.fRGBA.data(), img.fRGBA.size());
    return 1;
}
}
#include "texbuild/eqoi.h"
#include <vector>
extern "C" int qoi_decode(const unsigned char* d, int n, unsigned char* buf, int cap, int* w, int* h) {
    std::vector<uint8_t> rgba;
    if(!eQoi::decode(d, n, rgba, *w, *h)) return 0;
    if(int(rgba.size()) > cap) return -1;
    std::memcpy(buf, rgba.data(), rgba.size());
    return 1;
}
