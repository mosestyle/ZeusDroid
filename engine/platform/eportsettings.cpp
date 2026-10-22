#include "eportsettings.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>

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
                   "pinch_step \"1.30\"\n"
                   "tap_slop_px \"24\"\n"
                   "build_drag \"hold\"\n"
                   "build_hold_ms \"300\"\n";
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
    if(num("pinch_step", v)) fPinchStep = static_cast<float>(std::clamp(v, 1.05, 3.));
    if(num("tap_slop_px", v)) fTapSlop = static_cast<float>(std::clamp(v, 4., 200.));
    if(num("build_hold_ms", v)) fBuildHoldMs = static_cast<int>(std::clamp(v, 100., 2000.));
    const auto bd = m.find("build_drag");
    if(bd != m.end()) fHoldToDraw = bd->second != "direct";
    const auto it = m.find("ui_scale");
    if(it != m.end()) {
        const auto& s = it->second;
        if(s == "tiny") fUIScale = 0;
        else if(s == "small") fUIScale = 1;
        else if(s == "medium") fUIScale = 2;
        else if(s == "large") fUIScale = 3;
    }
}

void ePortSettings::applyTo(eTouchConfig& c) const {
    c.fPanSensitivity = fPanSensitivity;
    c.fLongPressMs = static_cast<uint32_t>(fLongPressMs);
    c.fPinchStepRatio = fPinchStep;
    c.fTapSlop = fTapSlop;
    c.fHoldToDraw = fHoldToDraw;
    c.fBuildHoldMs = static_cast<uint32_t>(fBuildHoldMs);
}
