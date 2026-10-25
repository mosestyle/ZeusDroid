#ifndef EQOI_H
#define EQOI_H

// Port (texture builder): small lossless image codec ("QOI") used for the
// texture packs built on the device. Much faster to write and read than PNG
// on a phone, files of similar size.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace eQoi {
    bool isQoi(const uint8_t* data, const size_t size);
    void encode(const uint8_t* rgba, const int w, const int h,
                std::vector<uint8_t>& out);
    bool decode(const uint8_t* data, const size_t size,
                std::vector<uint8_t>& rgba, int& w, int& h);
}

#endif // EQOI_H
