#ifndef ERESAMPLE_H
#define ERESAMPLE_H

// Port (texture builder): image scaling for the 15/45/60 texture sizes.

#include <cstdint>
#include <vector>

namespace eResample {
    enum class eFilter { triangle, cubic };
    void resize(const uint8_t* src, const int sw, const int sh,
                std::vector<uint8_t>& dst, const int dw, const int dh,
                const eFilter filter, const bool binaryAlpha);
}

#endif // ERESAMPLE_H
