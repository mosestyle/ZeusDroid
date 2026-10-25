// Host test driver: builds the texture packs from DATA into an output dir.
#include "texbuild/etexturebuilder.h"
#include <chrono>
#include <cstdio>
#include <cstring>
int main(int argc, char** argv) {
    if(argc < 3) { std::fprintf(stderr, "usage: %s <DATA> <out> [large]\n", argv[0]); return 2; }
    const bool large = argc > 3 && std::strcmp(argv[3], "large") == 0;
    std::string err;
    const auto t0 = std::chrono::steady_clock::now();
    int lastPct = -1;
    const bool ok = eTextureBuilder::build(argv[1], argv[2], large,
        [&](double f, const std::string& what) {
            const int pct = int(f*100);
            if(pct != lastPct) { lastPct = pct; std::fprintf(stderr, "%3d%% %s\n", pct, what.c_str()); }
        }, err);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("%s frames=%d missing=%d time=%.1fs %s\n", ok ? "OK" : "FAIL",
                eTextureBuilder::lastFrames(), eTextureBuilder::lastMissing(), s, err.c_str());
    return ok ? 0 : 1;
}
