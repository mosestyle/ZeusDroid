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
//   pinch_deadband    "1.04"    finger-distance ratio before a two-finger
//                                drag starts zooming (below: pan only)
//   max_zoom          "3.0"     1..4, maximum pinch/wheel zoom (1 = upstream)
//   tap_slop_px       "24"
//   build_drag        "hold"    hold | direct  (hold: drag pans in build mode,
//                                hold still then drag draws roads/rows)
//   build_hold_ms     "300"
//   build_confirm     "on"      on | off  (on: taps/drags only move a preview,
//                                the confirm button builds)
//   show_stats        "off"     on: FPS / frame time / ticks / memory overlay
//   build_repeat      "on"      on | off  (on: after building, the next
//                                preview appears next to it)
struct ePortSettings {
    double fRenderFps = -1;      // <0: platform default
    int fUIScale = -1;           // -1 auto, else eUIScale value
    float fPanSensitivity = 1.f;
    int fLongPressMs = 500;
    float fPinchDeadband = 1.04f;
    double fMaxZoom = 3.0;
    float fTapSlop = 24.f;
    bool fHoldToDraw = true;
    int fBuildHoldMs = 300;
    bool fBuildConfirm = true;
    bool fBuildRepeat = true;
    bool fShowStats = false;

    static std::string path();
    // Loads from path(); writes a commented default file if none exists.
    void load();
    // Writes all keys to path() (temp file + rename).
    void save() const;
    void applyTo(eTouchConfig& c) const;
    static ePortSettings& instance();
};

#endif // EPORTSETTINGS_H
