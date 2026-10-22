#include "eplatform.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "egamedir.h"
#include "esettings.h"
#include "eengconverter.h"

void ePlatform::preInitHints() {
#ifdef __ANDROID__
    // Touch is handled natively by eTouchGestures; SDL's synthetic mouse
    // events would cause every tap to be dispatched twice.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    // Keep the GL context alive across pause/resume where possible.
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "1");
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE_PAUSEAUDIO, "1");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
#endif
}

void ePlatform::adjustSettings(eSettings& settings) {
#ifdef __ANDROID__
    SDL_DisplayMode dm;
    if(SDL_GetCurrentDisplayMode(0, &dm) == 0 && dm.w > 0 && dm.h > 0) {
        const int w = std::max(dm.w, dm.h);
        const int h = std::min(dm.w, dm.h);
        settings.fRes = eResolution(w, h);
    }
    settings.fFullscreen = true;
#else
    (void)settings;
#endif
}

bool ePlatform::prepareLanguageFiles() {
    bool ok = true;
    const auto convert = [&ok](const std::string& eng, const std::string& xml) {
        std::error_code ec;
        if(std::filesystem::exists(xml, ec)) return;
        if(!std::filesystem::exists(eng, ec)) {
            log("Language source missing: " + eng);
            return;
        }
        std::string err;
        if(eEngConverter::convertFile(eng, xml, err)) {
            log("Converted " + eng + " -> " + xml);
        } else {
            log("Language conversion failed for " + eng + ": " + err);
            ok = false;
        }
    };
    convert(eGameDir::path("Zeus_Text.eng"), eGameDir::exeDir() + "../Zeus_Text.xml");
    convert(eGameDir::path("Zeus_MM.eng"), eGameDir::exeDir() + "../Zeus_MM.xml");
    return ok;
}

void ePlatform::log(const std::string& message) {
    SDL_Log("%s", message.c_str());
}

void ePlatform::fatal(const std::string& message) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message.c_str());
    std::fprintf(stderr, "%s\n", message.c_str());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Zeus & Poseidon",
                             message.c_str(), nullptr);
}

bool ePlatform::isTouchPlatform() {
#ifdef __ANDROID__
    return true;
#else
    return false;
#endif
}
