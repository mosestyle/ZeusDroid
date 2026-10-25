#include "eportsettings.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <map>
#include <sstream>

#include "egamedir.h"
#include "eloadtexthelper.h"
#include "platform/etouchgestures.h"

std::string ePortSettings::path() {
    return eGameDir::exeDir() + "../port_settings.txt";
}

ePortSettings& ePortSettings::instance() {
    static ePortSettings s;
    return s;
}

void ePortSettings::load() {
    std::ifstream probe(path());
    if(!probe.good()) {
        std::ofstream out(path());
        if(out) {
            out << "render_fps \"auto\"\n"
                   "ui_scale \"auto\"\n"
                   "pan_sensitivity \"1.0\"\n"
                   "long_press_ms \"500\"\n"
                   "pinch_deadband \"1.04\"\n"
                   "max_zoom \"3.0\"\n"
                   "tap_slop_px \"24\"\n"
                   "build_drag \"hold\"\n"
                   "build_hold_ms \"300\"\n"
                   "build_confirm \"on\"\n"
                   "build_repeat \"on\"\n"
                   "show_stats \"off\"\n"
                   "vibrate \"on\"\n";
        }
        return;
    }
    probe.close();
    std::map<std::string, std::string> m;
    if(!eLoadTextHelper::load(path(), m)) return;
    const auto num = [&m](const char* k, double& out) {
        const auto it = m.find(k);
        if(it == m.end() || it->second.empty() || it->second == "auto") return false;
        char* end = nullptr;
        const double v = std::strtod(it->second.c_str(), &end);
        if(end == it->second.c_str()) return false;
        out = v;
        return true;
    };
    double v;
    if(num("render_fps", v)) fRenderFps = std::clamp(v, 20., 120.);
    if(num("pan_sensitivity", v)) fPanSensitivity = static_cast<float>(std::clamp(v, 0.25, 4.));
    if(num("long_press_ms", v)) fLongPressMs = static_cast<int>(std::clamp(v, 200., 2000.));
    // "pinch_step" (discrete zoom steps, <= 0.1.3) is obsolete and ignored.
    if(num("pinch_deadband", v)) fPinchDeadband = static_cast<float>(std::clamp(v, 1., 1.5));
    if(num("max_zoom", v)) fMaxZoom = std::clamp(v, 1., 4.);
    if(num("tap_slop_px", v)) fTapSlop = static_cast<float>(std::clamp(v, 4., 200.));
    if(num("undo_seconds", v)) fUndoSeconds = static_cast<int>(std::clamp(v, 5., 300.));
    if(num("build_hold_ms", v)) fBuildHoldMs = static_cast<int>(std::clamp(v, 100., 2000.));
    const auto bd = m.find("build_drag");
    if(bd != m.end()) fHoldToDraw = bd->second != "direct";
    const auto bc = m.find("build_confirm");
    if(bc != m.end()) fBuildConfirm = bc->second != "off";
    const auto vb = m.find("vibrate");
    if(vb != m.end()) fVibrate = vb->second != "off";
    const auto ss = m.find("show_stats");
    if(ss != m.end()) fShowStats = ss->second == "on";
    const auto ds = m.find("draw_style");
    if(ds != m.end()) fDrawSwitch = ds->second == "switch";
    if(const char* const e = std::getenv("EZEUS_DRAW_STYLE")) {
        if(*e) fDrawSwitch = std::string(e) == "switch";
    }
    const auto br = m.find("build_repeat");
    if(br != m.end()) fBuildRepeat = br->second != "off";
    // Test override (autotests of the instant placement mode).
    if(const char* const e = std::getenv("EZEUS_BUILD_CONFIRM")) {
        if(*e) fBuildConfirm = *e != '0';
    }
    const auto it = m.find("ui_scale");
    if(it != m.end()) {
        const auto& s = it->second;
        if(s == "tiny") fUIScale = 0;
        else if(s == "small") fUIScale = 1;
        else if(s == "medium") fUIScale = 2;
        else if(s == "large") fUIScale = 3;
        else if(s == "fit") fUIScale = 4;
    }
}

void ePortSettings::applyTo(eTouchConfig& c) const {
    c.fPanSensitivity = fPanSensitivity;
    c.fLongPressMs = static_cast<uint32_t>(fLongPressMs);
    c.fPinchStepRatio = fPinchDeadband;
    c.fTapSlop = fTapSlop;
    c.fHoldToDraw = fHoldToDraw;
    c.fBuildHoldMs = static_cast<uint32_t>(fBuildHoldMs);
    c.fBuildConfirm = fBuildConfirm;
}

void ePortSettings::save() const {
    std::ostringstream o;
    const auto q = [](const std::string& v) { return "\"" + v + "\""; };
    const auto num = [](const double v) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%g", v);
        return std::string(buf);
    };
    static const char* const scales[] = {"tiny", "small", "medium", "large", "fit"};
    o << "render_fps " << q(fRenderFps < 20 ? "auto" : num(fRenderFps)) << "\n";
    o << "ui_scale " << q(fUIScale < 0 || fUIScale > 4 ? "auto" : scales[fUIScale]) << "\n";
    o << "pan_sensitivity " << q(num(fPanSensitivity)) << "\n";
    o << "long_press_ms " << q(num(fLongPressMs)) << "\n";
    o << "pinch_deadband " << q(num(fPinchDeadband)) << "\n";
    o << "max_zoom " << q(num(fMaxZoom)) << "\n";
    o << "tap_slop_px " << q(num(fTapSlop)) << "\n";
    o << "build_drag " << q(fHoldToDraw ? "hold" : "direct") << "\n";
    o << "build_hold_ms " << q(num(fBuildHoldMs)) << "\n";
    o << "build_confirm " << q(fBuildConfirm ? "on" : "off") << "\n";
    o << "build_repeat " << q(fBuildRepeat ? "on" : "off") << "\n";
    o << "show_stats " << q(fShowStats ? "on" : "off") << "\n";
    o << "vibrate " << q(fVibrate ? "on" : "off") << "\n";
    o << "draw_style " << q(fDrawSwitch ? "switch" : "button") << "\n";
    o << "undo_seconds " << q(num(fUndoSeconds)) << "\n";
    const auto tmp = path() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if(!out) return;
        out << o.str();
        if(!out) return;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path(), ec);
}
