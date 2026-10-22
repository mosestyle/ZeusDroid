#include "eplatform.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "egamedir.h"
#include "esettings.h"
#include "eengconverter.h"

#ifdef __ANDROID__
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>

namespace {
// eZeus reports most problems with printf(); Android discards stdout/stderr,
// so forward them to logcat (tag "eZeus").
int gLogPipe[2] = {-1, -1};

void* logPump(void*) {
    char buf[1024];
    std::string line;
    ssize_t n;
    while((n = read(gLogPipe[0], buf, sizeof(buf))) > 0) {
        for(ssize_t i = 0; i < n; i++) {
            if(buf[i] == '\n' || line.size() > 900) {
                __android_log_write(ANDROID_LOG_INFO, "eZeus", line.c_str());
                line.clear();
                if(buf[i] == '\n') continue;
            }
            line += buf[i];
        }
    }
    return nullptr;
}

void redirectStdioToLogcat() {
    if(gLogPipe[0] != -1) return;
    if(pipe(gLogPipe) != 0) return;
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    dup2(gLogPipe[1], STDOUT_FILENO);
    dup2(gLogPipe[1], STDERR_FILENO);
    pthread_t t;
    if(pthread_create(&t, nullptr, logPump, nullptr) == 0) pthread_detach(t);
}
}
#endif

void ePlatform::preInitHints() {
#ifdef __ANDROID__
    redirectStdioToLogcat();
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

namespace {
int uiScaleFromEnv() {
    const char* const e = std::getenv("EZEUS_UI_SCALE");
    if(!e) return -1;
    const std::string s(e);
    if(s == "tiny") return 0;
    if(s == "small") return 1;
    if(s == "medium") return 2;
    if(s == "large") return 3;
    return -1;
}
}

void ePlatform::adjustSettings(eSettings& settings) {
    eResolution::sUIScaleOverride = uiScaleFromEnv();
#ifdef __ANDROID__
    if(eResolution::sUIScaleOverride < 0) {
        // Phones: eZeus' interface art exists for 4 scales (0.5x .. 2x).
        // Pick the largest that keeps >= ~540 logical rows, so buttons are
        // finger-sized: 1080p -> 2x, 720p -> 1x.
        SDL_DisplayMode m;
        if(SDL_GetCurrentDisplayMode(0, &m) == 0) {
            const int h = std::min(m.w, m.h);
            if(h >= 1080) eResolution::sUIScaleOverride = 3;
            else if(h >= 810) eResolution::sUIScaleOverride = 2;
            else eResolution::sUIScaleOverride = 1;
        }
    }
    SDL_DisplayMode dm;
    if(SDL_GetCurrentDisplayMode(0, &dm) == 0 && dm.w > 0 && dm.h > 0) {
        const int w = std::max(dm.w, dm.h);
        const int h = std::min(dm.w, dm.h);
        settings.fRes = eResolution(w, h);
    }
    settings.fFullscreen = true;
#else
    if(eResolution::sUIScaleOverride >= 0) {
        settings.fRes = eResolution(settings.fRes.width(), settings.fRes.height());
    }
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

double ePlatform::renderFps() {
    const char* const env = std::getenv("EZEUS_RENDER_FPS");
    if(env && *env) {
        const double v = std::atof(env);
        if(v >= 20. && v <= 120.) return v;
    }
    return isTouchPlatform() ? 60. : 20.;
}

bool ePlatform::isTouchPlatform() {
#ifdef __ANDROID__
    return true;
#else
    return false;
#endif
}
