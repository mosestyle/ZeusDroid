// Unit test: QOI round trip (texture builder pages) and resampler sanity.
#include "texbuild/eqoi.h"
#include "texbuild/eresample.h"
#include <cstdio>
#include <cstdlib>
static int gC = 0, gF = 0;
static void check(bool ok, const char* w) { gC++; if(!ok) { gF++; std::printf("FAIL: %s\n", w); } }
int main() {
    std::srand(7);
    for(int t = 0; t < 20; t++) {
        const int w = 1 + std::rand() % 300, h = 1 + std::rand() % 200;
        std::vector<uint8_t> img(size_t(w)*h*4);
        for(size_t i = 0; i < img.size(); i++) {
            // mix of runs, small diffs and random values
            img[i] = (t % 3 == 0) ? std::rand() % 256 : ((i / 64) % 7) * 30 + (std::rand() % 3);
        }
        std::vector<uint8_t> enc, dec;
        eQoi::encode(img.data(), w, h, enc);
        int dw = 0, dh = 0;
        const bool ok = eQoi::decode(enc.data(), enc.size(), dec, dw, dh);
        check(ok && dw == w && dh == h && dec == img, "qoi round trip");
    }
    {   // transparent stays transparent, opaque colour kept when halving
        std::vector<uint8_t> src(8*8*4, 0), dst;
        for(int y = 0; y < 8; y++) for(int x = 0; x < 4; x++) {
            uint8_t* p = &src[(y*8 + x)*4]; p[0] = 200; p[1] = 100; p[2] = 50; p[3] = 255;
        }
        eResample::resize(src.data(), 8, 8, dst, 4, 4, eResample::eFilter::triangle, eResample::eAlpha::smooth);
        check(dst[3] == 255 && dst[0] > 190 && dst[0] < 210, "opaque left half keeps colour");
        check(dst[(3)*4 + 3] == 0, "transparent right edge stays transparent");
        check(dst[(1)*4 + 3] > 200, "inner pixel near-opaque");
        eResample::resize(src.data(), 8, 8, dst, 16, 16, eResample::eFilter::cubic, eResample::eAlpha::binary);
        bool binary = true;
        for(size_t i = 3; i < dst.size(); i += 4) binary = binary && (dst[i] == 0 || dst[i] == 255);
        check(binary, "binary alpha for interface");
    }
    {   // cover: an opaque diamond tile halved has no half-transparent rim
        // inside the diamond, and a 50% shadow stays 50%.
        const int w = 58, h = 30;
        std::vector<uint8_t> src(w*h*4, 0), dst;
        for(int y = 0; y < h; y++) for(int x = 0; x < w; x++) {
            const double dx = std::abs(x + 0.5 - w/2.0)/(w/2.0);
            const double dy = std::abs(y + 0.5 - h/2.0)/(h/2.0);
            if(dx + dy <= 1.0) { uint8_t* p = &src[(y*w + x)*4]; p[0] = 180; p[1] = 150; p[2] = 60; p[3] = 255; }
        }
        eResample::resize(src.data(), w, h, dst, 29, 15, eResample::eFilter::triangle, eResample::eAlpha::cover);
        int partial = 0, opaque = 0;
        for(size_t i = 3; i < dst.size(); i += 4) { partial += dst[i] > 0 && dst[i] < 255; opaque += dst[i] == 255; }
        check(partial == 0, "cover: no half-transparent edge pixels on a terrain tile");
        eResample::resize(src.data(), w, h, dst, 29, 15, eResample::eFilter::triangle, eResample::eAlpha::smooth);
        int opaqueSmooth = 0;
        for(size_t i = 3; i < dst.size(); i += 4) opaqueSmooth += dst[i] == 255;
        check(opaque > opaqueSmooth, "cover: more opaque pixels than smooth (closed edges)");
        std::vector<uint8_t> sh(8*8*4, 0);
        for(size_t i = 0; i < 64; i++) sh[i*4 + 3] = 127;
        eResample::resize(sh.data(), 8, 8, dst, 4, 4, eResample::eFilter::triangle, eResample::eAlpha::cover);
        bool half = true;
        for(size_t i = 3; i < dst.size(); i += 4) half = half && dst[i] == 127;
        check(half, "cover: shadow alpha kept");
    }
    std::printf("qoi/resample: %d checks, %d failures\n", gC, gF);
    return gF ? 1 : 0;
}
