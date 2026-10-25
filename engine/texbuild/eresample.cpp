#include "eresample.h"

#include <algorithm>
#include <cmath>

// Separable resampling with premultiplied alpha. Downscaling widens the
// kernel by the scale factor (area-correct), upscaling uses it as is.

namespace {
double triangle(const double x) {
    const double a = std::fabs(x);
    return a < 1 ? 1 - a : 0;
}

double cubic(const double x) {
    // Keys cubic, a = -0.5 (Catmull-Rom).
    const double a = -0.5;
    const double t = std::fabs(x);
    if(t < 1) return ((a + 2)*t - (a + 3))*t*t + 1;
    if(t < 2) return (((t - 5)*t + 8)*t - 4)*a;
    return 0;
}

struct eTap { int fFirst; std::vector<float> fW; };

std::vector<eTap> taps(const int src, const int dst, const eResample::eFilter f) {
    std::vector<eTap> out(dst);
    const double scale = double(dst)/src;
    const double support = f == eResample::eFilter::triangle ? 1.0 : 2.0;
    const double fs = scale < 1 ? 1/scale : 1;
    for(int i = 0; i < dst; i++) {
        const double c = (i + 0.5)/scale;
        const int a = static_cast<int>(std::floor(c - support*fs));
        const int b = static_cast<int>(std::ceil(c + support*fs));
        auto& t = out[i];
        t.fFirst = std::max(0, a);
        double sum = 0;
        for(int j = t.fFirst; j < std::min(src, b + 1); j++) {
            const double x = (j + 0.5 - c)/fs;
            const double w = f == eResample::eFilter::triangle ? triangle(x) : cubic(x);
            t.fW.push_back(static_cast<float>(w));
            sum += w;
        }
        if(sum != 0) for(auto& w : t.fW) w = static_cast<float>(w/sum);
    }
    return out;
}
}

void eResample::resize(const uint8_t* src, const int sw, const int sh,
                       std::vector<uint8_t>& dst, const int dw, const int dh,
                       const eFilter filter, const bool binaryAlpha) {
    dst.assign(size_t(dw)*dh*4, 0);
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
    // Premultiplied float source.
    std::vector<float> pm(size_t(sw)*sh*4);
    for(size_t i = 0; i < size_t(sw)*sh; i++) {
        const float a = src[4*i + 3]/255.f;
        pm[4*i] = src[4*i]*a;
        pm[4*i + 1] = src[4*i + 1]*a;
        pm[4*i + 2] = src[4*i + 2]*a;
        pm[4*i + 3] = src[4*i + 3];
    }
    const auto tx = taps(sw, dw, filter);
    const auto ty = taps(sh, dh, filter);
    std::vector<float> tmp(size_t(dw)*sh*4, 0.f);
    for(int y = 0; y < sh; y++) {
        for(int x = 0; x < dw; x++) {
            const auto& t = tx[x];
            float acc[4] = {0, 0, 0, 0};
            for(size_t k = 0; k < t.fW.size(); k++) {
                const float* p = &pm[(size_t(y)*sw + t.fFirst + k)*4];
                const float w = t.fW[k];
                acc[0] += p[0]*w; acc[1] += p[1]*w; acc[2] += p[2]*w; acc[3] += p[3]*w;
            }
            float* o = &tmp[(size_t(y)*dw + x)*4];
            o[0] = acc[0]; o[1] = acc[1]; o[2] = acc[2]; o[3] = acc[3];
        }
    }
    for(int y = 0; y < dh; y++) {
        const auto& t = ty[y];
        for(int x = 0; x < dw; x++) {
            float acc[4] = {0, 0, 0, 0};
            for(size_t k = 0; k < t.fW.size(); k++) {
                const float* p = &tmp[(size_t(t.fFirst + k)*dw + x)*4];
                const float w = t.fW[k];
                acc[0] += p[0]*w; acc[1] += p[1]*w; acc[2] += p[2]*w; acc[3] += p[3]*w;
            }
            uint8_t* o = &dst[(size_t(y)*dw + x)*4];
            float a = std::clamp(acc[3], 0.f, 255.f);
            if(binaryAlpha) a = a >= 127.5f ? 255.f : 0.f;
            if(a <= 0.f) continue;
            const float inv = 255.f/std::clamp(acc[3], 1e-3f, 255.f);
            o[0] = static_cast<uint8_t>(std::clamp(acc[0]*inv + 0.5f, 0.f, 255.f));
            o[1] = static_cast<uint8_t>(std::clamp(acc[1]*inv + 0.5f, 0.f, 255.f));
            o[2] = static_cast<uint8_t>(std::clamp(acc[2]*inv + 0.5f, 0.f, 255.f));
            o[3] = static_cast<uint8_t>(a + 0.5f);
        }
    }
}
