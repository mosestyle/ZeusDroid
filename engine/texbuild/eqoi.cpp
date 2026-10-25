#include "eqoi.h"

#include <cstring>

// Implementation of the "Quite OK Image" format (public specification,
// qoiformat.org), written for this project.

namespace {
const uint8_t kIndex = 0x00;
const uint8_t kDiff = 0x40;
const uint8_t kLuma = 0x80;
const uint8_t kRun = 0xc0;
const uint8_t kRGB = 0xfe;
const uint8_t kRGBA = 0xff;
const uint8_t kMask = 0xc0;

struct ePx { uint8_t r, g, b, a; };

inline int hashPx(const ePx& p) {
    return (p.r*3 + p.g*5 + p.b*7 + p.a*11) % 64;
}

inline void put32(std::vector<uint8_t>& o, const uint32_t v) {
    o.push_back(v >> 24); o.push_back(v >> 16); o.push_back(v >> 8); o.push_back(v);
}
}

bool eQoi::isQoi(const uint8_t* data, const size_t size) {
    return size >= 14 && std::memcmp(data, "qoif", 4) == 0;
}

void eQoi::encode(const uint8_t* rgba, const int w, const int h,
                  std::vector<uint8_t>& out) {
    out.clear();
    out.reserve(size_t(w)*h + 64);
    out.insert(out.end(), {'q', 'o', 'i', 'f'});
    put32(out, w);
    put32(out, h);
    out.push_back(4);
    out.push_back(0);
    ePx index[64];
    std::memset(index, 0, sizeof(index));
    ePx prev{0, 0, 0, 255};
    int run = 0;
    const size_t n = size_t(w)*h;
    for(size_t i = 0; i < n; i++) {
        const ePx px{rgba[4*i], rgba[4*i + 1], rgba[4*i + 2], rgba[4*i + 3]};
        if(std::memcmp(&px, &prev, 4) == 0) {
            if(++run == 62 || i == n - 1) {
                out.push_back(kRun | (run - 1));
                run = 0;
            }
            continue;
        }
        if(run > 0) {
            out.push_back(kRun | (run - 1));
            run = 0;
        }
        const int hidx = hashPx(px);
        if(std::memcmp(&index[hidx], &px, 4) == 0) {
            out.push_back(kIndex | hidx);
        } else {
            index[hidx] = px;
            if(px.a == prev.a) {
                const int dr = int(px.r) - prev.r;
                const int dg = int(px.g) - prev.g;
                const int db = int(px.b) - prev.b;
                const int8_t vr = static_cast<int8_t>(dr);
                const int8_t vg = static_cast<int8_t>(dg);
                const int8_t vb = static_cast<int8_t>(db);
                const int8_t vgr = vr - vg;
                const int8_t vgb = vb - vg;
                if(vr > -3 && vr < 2 && vg > -3 && vg < 2 && vb > -3 && vb < 2) {
                    out.push_back(kDiff | ((vr + 2) << 4) | ((vg + 2) << 2) | (vb + 2));
                } else if(vgr > -9 && vgr < 8 && vg > -33 && vg < 32 &&
                          vgb > -9 && vgb < 8) {
                    out.push_back(kLuma | (vg + 32));
                    out.push_back(((vgr + 8) << 4) | (vgb + 8));
                } else {
                    out.push_back(kRGB);
                    out.push_back(px.r); out.push_back(px.g); out.push_back(px.b);
                }
            } else {
                out.push_back(kRGBA);
                out.push_back(px.r); out.push_back(px.g); out.push_back(px.b); out.push_back(px.a);
            }
        }
        prev = px;
    }
    for(int i = 0; i < 7; i++) out.push_back(0);
    out.push_back(1);
}

bool eQoi::decode(const uint8_t* data, const size_t size,
                  std::vector<uint8_t>& rgba, int& w, int& h) {
    if(!isQoi(data, size)) return false;
    const auto rd32 = [&](const size_t o) {
        return (uint32_t(data[o]) << 24) | (data[o + 1] << 16) | (data[o + 2] << 8) | data[o + 3];
    };
    w = static_cast<int>(rd32(4));
    h = static_cast<int>(rd32(8));
    if(w <= 0 || h <= 0 || size_t(w)*h > (size_t(1) << 28)) return false;
    const size_t n = size_t(w)*h;
    rgba.resize(n*4);
    ePx index[64];
    std::memset(index, 0, sizeof(index));
    ePx px{0, 0, 0, 255};
    size_t p = 14;
    const size_t end = size >= 8 ? size - 8 : 0;
    int run = 0;
    for(size_t i = 0; i < n; i++) {
        if(run > 0) {
            run--;
        } else if(p < end) {
            const uint8_t b1 = data[p++];
            if(b1 == kRGB) {
                px.r = data[p++]; px.g = data[p++]; px.b = data[p++];
            } else if(b1 == kRGBA) {
                px.r = data[p++]; px.g = data[p++]; px.b = data[p++]; px.a = data[p++];
            } else if((b1 & kMask) == kIndex) {
                px = index[b1];
            } else if((b1 & kMask) == kDiff) {
                px.r += ((b1 >> 4) & 3) - 2;
                px.g += ((b1 >> 2) & 3) - 2;
                px.b += (b1 & 3) - 2;
            } else if((b1 & kMask) == kLuma) {
                const uint8_t b2 = data[p++];
                const int vg = (b1 & 0x3f) - 32;
                px.r += vg - 8 + ((b2 >> 4) & 0x0f);
                px.g += vg;
                px.b += vg - 8 + (b2 & 0x0f);
            } else if((b1 & kMask) == kRun) {
                run = (b1 & 0x3f);
            }
            index[hashPx(px)] = px;
        }
        std::memcpy(&rgba[i*4], &px, 4);
    }
    return true;
}
