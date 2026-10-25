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
        eResample::resize(src.data(), 8, 8, dst, 4, 4, eResample::eFilter::triangle, false);
        check(dst[3] == 255 && dst[0] > 190 && dst[0] < 210, "opaque left half keeps colour");
        check(dst[(3)*4 + 3] == 0, "transparent right edge stays transparent");
        check(dst[(1)*4 + 3] > 200, "inner pixel near-opaque");
        eResample::resize(src.data(), 8, 8, dst, 16, 16, eResample::eFilter::cubic, true);
        bool binary = true;
        for(size_t i = 3; i < dst.size(); i += 4) binary = binary && (dst[i] == 0 || dst[i] == 255);
        check(binary, "binary alpha for interface");
    }
    std::printf("qoi/resample: %d checks, %d failures\n", gC, gF);
    return gF ? 1 : 0;
}
