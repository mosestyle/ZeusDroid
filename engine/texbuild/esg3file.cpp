#include "esg3file.h"

#include <algorithm>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace {
const int kHeader = 680;
const int kBitmapRecord = 200;
const int kMaxBitmaps = 200;
const uint16_t kTransparent = 0xF81F;
const uint16_t kShadow = 0x7C00;

uint32_t u32(const std::vector<uint8_t>& d, const size_t o) {
    if(o + 4 > d.size()) return 0;
    return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (uint32_t(d[o + 3]) << 24);
}

int s16(const std::vector<uint8_t>& d, const size_t o) {
    if(o + 2 > d.size()) return 0;
    return static_cast<int16_t>(d[o] | (d[o + 1] << 8));
}

int u16(const std::vector<uint8_t>& d, const size_t o) {
    if(o + 2 > d.size()) return 0;
    return d[o] | (d[o + 1] << 8);
}

// 15-bit colour to 8-bit channels the way the existing texture packs were
// made: red and blue 5 -> 8 bits, green treated as 6 bits (g5 * 2).
inline void putPixel(uint8_t* const p, const uint16_t c) {
    const int r = (c >> 10) & 0x1f;
    const int g = ((c >> 5) & 0x1f)*2;
    const int b = c & 0x1f;
    p[0] = static_cast<uint8_t>((r << 3) | (r >> 2));
    p[1] = static_cast<uint8_t>((g << 2) | (g >> 4));
    p[2] = static_cast<uint8_t>((b << 3) | (b >> 2));
    p[3] = 255;
}

// RLE sprite data: 255, n = skip n transparent pixels; c < 255 = c literal
// 15-bit pixels follow. Pixels run left to right, row after row.
void decodeRle(const uint8_t* d, const size_t n, eSg3Image& img,
               const int w, const int h, const bool shadows) {
    size_t i = 0;
    int x = 0;
    int y = 0;
    while(i < n && y < h) {
        const int c = d[i];
        if(c == 255) {
            if(i + 1 >= n) break;
            x += d[i + 1];
            i += 2;
        } else {
            i++;
            for(int k = 0; k < c && i + 1 < n; k++) {
                if(y < h) {
                    const uint16_t px = d[i] | (d[i + 1] << 8);
                    uint8_t* const p = &img.fRGBA[(size_t(y)*img.fW + x)*4];
                    if(shadows && px == kShadow) {
                        // Pure red marks shadow pixels: half-transparent black.
                        p[0] = p[1] = p[2] = 0;
                        p[3] = 127;
                    } else {
                        putPixel(p, px);
                    }
                }
                i += 2;
                if(++x >= w) { x = 0; y++; }
            }
        }
        while(x >= w) { x -= w; y++; }
    }
}

// Alpha mask (images with semi-transparent parts): same run structure as
// the sprite data, one byte (0..31) per pixel.
void decodeAlpha(const uint8_t* d, const size_t n, eSg3Image& img,
                 const int w, const int h) {
    size_t i = 0;
    int x = 0;
    int y = 0;
    while(i < n && y < h) {
        const int c = d[i];
        if(c == 255) {
            if(i + 1 >= n) break;
            x += d[i + 1];
            i += 2;
        } else {
            i++;
            for(int k = 0; k < c && i < n; k++) {
                if(y < h) {
                    const int a = d[i] & 0x1f;
                    uint8_t* const p = &img.fRGBA[(size_t(y)*img.fW + x)*4];
                    p[3] = static_cast<uint8_t>((a << 3) | (a >> 2));
                }
                i++;
                if(++x >= w) { x = 0; y++; }
            }
        }
        while(x >= w) { x -= w; y++; }
    }
}

// One isometric tile, 58 x 30 pixels: rows 0..14 are 2, 6, ... 58 pixels
// wide, rows 15..29 shrink again; stored row by row, centred.
size_t decodeTile(const uint8_t* d, const size_t n, size_t k,
                  eSg3Image& img, const int x0, const int y0) {
    for(int j = 0; j < 30; j++) {
        int w;
        int xs;
        if(j < 15) {
            w = 4*j + 2;
            xs = 28 - 2*j;
        } else {
            w = 4*(29 - j) + 2;
            xs = 2*(j - 15);
        }
        for(int x = 0; x < w; x++) {
            if(k + 1 >= n) return k;
            const int px = x0 + xs + x;
            const int py = y0 + j;
            if(px >= 0 && py >= 0 && px < img.fW && py < img.fH) {
                putPixel(&img.fRGBA[(size_t(py)*img.fW + px)*4], d[k] | (d[k + 1] << 8));
            }
            k += 2;
        }
    }
    return k;
}
}

bool eSg3File::open(const std::string& sg3Path) {
    mOpen = false;
    std::ifstream f(sg3Path, std::ios::binary);
    if(!f) return false;
    mIndex.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    if(mIndex.size() < size_t(kHeader + kBitmapRecord*kMaxBitmaps)) return false;
    mVersion = u32(mIndex, 4);
    mRecordSize = mVersion >= 0xd6 ? 72 : 64;
    mImageCount = static_cast<int>(u32(mIndex, 16));
    const int nb = static_cast<int>(u32(mIndex, 20));
    mBitmapStart.clear();
    mBitmapImages.clear();
    mBitmapNames.clear();
    for(int b = 0; b < nb && b < kMaxBitmaps; b++) {
        const size_t no = kHeader + size_t(b)*kBitmapRecord;
        std::string name(reinterpret_cast<const char*>(&mIndex[no]),
                         strnlen(reinterpret_cast<const char*>(&mIndex[no]), 65));
        mBitmapNames.push_back(name);
        const size_t o = kHeader + size_t(b)*kBitmapRecord + 116;
        mBitmapImages.push_back(static_cast<int>(u32(mIndex, o + 8)));
        mBitmapStart.push_back(static_cast<int>(u32(mIndex, o + 12)));
    }
    std::string base = sg3Path;
    const auto dot = base.find_last_of('.');
    if(dot != std::string::npos) base = base.substr(0, dot);
    m555Path = base + ".555";
    const auto slash = sg3Path.find_last_of("/\\");
    mDir = slash == std::string::npos ? std::string(".") : sg3Path.substr(0, slash);
    mOpen = true;
    return true;
}

bool eSg3File::record(const int i, eRecord& r) const {
    if(!mOpen || i <= 0 || i > mImageCount + 256) return false;
    const size_t o = kHeader + size_t(kMaxBitmaps)*kBitmapRecord + size_t(i)*mRecordSize;
    if(o + mRecordSize > mIndex.size()) return false;
    r.fOffset = u32(mIndex, o);
    r.fLength = u32(mIndex, o + 4);
    r.fULength = u32(mIndex, o + 8);
    r.fMirror = static_cast<int32_t>(u32(mIndex, o + 16));
    r.fW = s16(mIndex, o + 20);
    r.fH = s16(mIndex, o + 22);
    r.fType = u16(mIndex, o + 50);
    r.fExternal = mIndex[o + 52] != 0;
    r.fTop = mIndex[o + 53];
    r.fBitmap = mIndex[o + 56];
    r.fTiles = mIndex[o + 55];
    if(mRecordSize >= 72) {
        r.fAlphaOffset = u32(mIndex, o + 64);
        r.fAlphaLength = u32(mIndex, o + 68);
    }
    return true;
}

bool eSg3File::recordSize(const int i, int& w, int& h) const {
    eRecord r;
    if(!record(i, r)) return false;
    w = r.fW;
    h = r.fH;
    return true;
}

bool eSg3File::readData(const uint32_t offset, const uint32_t length,
                        std::vector<uint8_t>& out) {
    std::ifstream f(m555Path, std::ios::binary);
    if(!f) return false;
    f.seekg(offset);
    out.resize(length);
    f.read(reinterpret_cast<char*>(out.data()), length);
    return static_cast<uint32_t>(f.gcount()) == length;
}

std::string eSg3File::externalPath(const int bitmap) const {
    if(bitmap < 0 || bitmap >= int(mBitmapNames.size())) return "";
    std::string base = mBitmapNames[bitmap];
    const auto dot = base.find_last_of('.');
    if(dot != std::string::npos) base = base.substr(0, dot);
    std::string want = base + ".555";
    for(auto& c : want) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    // File names differ in case between bitmap records and DATA/.
    std::error_code ec;
    for(const auto& e : std::filesystem::directory_iterator(mDir, ec)) {
        std::string n = e.path().filename().string();
        std::string l = n;
        for(auto& c : l) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if(l == want) return e.path().string();
    }
    return "";
}

bool eSg3File::readData(const eRecord& r, const uint32_t offset, const uint32_t length,
                        std::vector<uint8_t>& out) {
    if(!r.fExternal) return readData(offset, length, out);
    const auto path = externalPath(r.fBitmap);
    if(path.empty() || offset == 0) return false;
    std::ifstream f(path, std::ios::binary);
    if(!f) return false;
    f.seekg(offset - 1);
    out.resize(length);
    f.read(reinterpret_cast<char*>(out.data()), length);
    return static_cast<uint32_t>(f.gcount()) == length;
}

bool eSg3File::decode(const int i, eSg3Image& out, const bool shadows) {
    out = eSg3Image();
    eRecord r;
    if(!record(i, r)) return false;
    if(r.fLength == 0 && r.fMirror != 0) {
        // Mirror record: the referenced image, flipped horizontally.
        eSg3Image src;
        if(!decode(i + r.fMirror, src, shadows)) return false;
        out.fW = src.fW;
        out.fH = src.fH;
        out.fRGBA.resize(src.fRGBA.size());
        for(int y = 0; y < src.fH; y++) {
            for(int x = 0; x < src.fW; x++) {
                std::memcpy(&out.fRGBA[(size_t(y)*src.fW + x)*4],
                            &src.fRGBA[(size_t(y)*src.fW + (src.fW - 1 - x))*4], 4);
            }
        }
        return true;
    }
    if(r.fW <= 0 || r.fH <= 0 || r.fLength == 0) return false;
    std::vector<uint8_t> d;
    if(!readData(r, r.fOffset, r.fLength, d)) return false;
    out.fW = r.fW;
    out.fH = r.fH;
    out.fRGBA.assign(size_t(r.fW)*r.fH*4, 0);
    const int w = r.fW;
    const int h = r.fH;
    if(r.fType == 30) {
        // Isometric: the footprint (tiles in diamond rows, bottom of the
        // image) followed by an optional RLE top drawn from the top edge.
        int n = r.fTiles;
        if(n <= 0) n = (w + 2)/60;
        const size_t fp = std::min<size_t>(r.fULength ? r.fULength : r.fLength, d.size());
        const int base = h - 30*n;
        size_t k = 0;
        for(int row = 0; row < 2*n - 1; row++) {
            const int cnt = row < n ? row + 1 : 2*n - 1 - row;
            const int y = base + 15*row;
            const int xs = row < n ? 30*(n - 1) - 30*row : 30*(row - n + 1);
            for(int t = 0; t < cnt; t++) {
                k = decodeTile(d.data(), fp, k, out, xs + 60*t, y);
            }
        }
        if(d.size() > fp) {
            decodeRle(d.data() + fp, d.size() - fp, out, w, h - 15*n, shadows);
        }
    } else if(r.fLength == uint32_t(w*h*2) &&
              r.fType != 256 && r.fType != 257 && r.fType != 276) {
        for(int k = 0; k < w*h; k++) {
            const uint16_t px = d[2*k] | (d[2*k + 1] << 8);
            if(px == kTransparent) continue;
            putPixel(&out.fRGBA[size_t(k)*4], px);
        }
    } else {
        decodeRle(d.data(), d.size(), out, w, h, shadows);
    }
    if(r.fAlphaLength) {
        std::vector<uint8_t> a;
        // The alpha mask follows the colour data directly (the record's
        // own alpha offset field is not a file position).
        if(readData(r, r.fOffset + r.fLength, r.fAlphaLength, a)) {
            decodeAlpha(a.data(), a.size(), out, w, h);
        }
    }
    return true;
}
