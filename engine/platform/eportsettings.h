#ifndef EPORTSETTINGS_H
#define EPORTSETTINGS_H

#include <string>

struct eTouchConfig;

// Port-specific options stored next to eZeus' settings.txt in
// "<eZeus dir>/port_settings.txt" (same key "value" format), so they can be
// tuned on a device without rebuilding and without touching eSettings'
// on-disk format. Missing keys keep their defaults.
//
//   render_fps        "60"      20..120 (20 = upstream behaviour)
//   ui_scale          "auto"    auto | tiny | small | medium | large
//   pan_sensitivity   "1.0"
//   long_press_ms     "500"
//   pinch_step        "1.30"
//   tap_slop_px       "24"
struct ePortSettings {
    double fRenderFps = -1;      // <0: platform default
    int fUIScale = -1;           // -1 auto, else eUIScale value
    float fPanSensitivity = 1.f;
    int fLongPressMs = 500;
    float fPinchStep = 1.30f;
    float fTapSlop = 24.f;

    static std::string path();
    // Loads from path(); writes a commented default file if none exists.
    void load();
    void applyTo(eTouchConfig& c) const;
    static ePortSettings& instance();
};

#endif // EPORTSETTINGS_H
