#ifndef ERESAMPLE_H
#define ERESAMPLE_H

// Port (texture builder): image scaling for the 15/45/60 texture sizes.

#include <cstdint>
#include <vector>

namespace eResample {
    enum class eFilter { triangle, cubic };
    // smooth: filtered alpha; binary: hard edges (interface art);
    // cover: a pixel is as opaque as the most opaque source pixel inside it
    // (map art: terrain tiles keep closed edges, no gaps between tiles;
    // half-transparent shadows stay half-transparent).
    enum class eAlpha { smooth, binary, cover };
    void resize(const uint8_t* src, const int sw, const int sh,
                std::vector<uint8_t>& dst, const int dw, const int dh,
                const eFilter filter, const eAlpha alpha);
}

#endif // ERESAMPLE_H
