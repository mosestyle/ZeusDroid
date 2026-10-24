#include "eautotest.h"

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "emainwindow.h"
#include "widgets/egamewidget.h"
#include "platform/etouchgestures.h"

namespace {

struct Cmd {
    uint32_t fDelay = 0;
    std::string fName;
    std::vector<std::string> fArgs;
    std::string fRest;
};

struct State {
    bool fActive = false;
    std::vector<Cmd> fCmds;
    size_t fNext = 0;
    uint32_t fLastMs = 0;
    bool fStarted = false;
    std::string fPendingShot;
    bool fWaiting = false;
    uint32_t fWaitUntil = 0;
};

State& st() {
    static State s;
    return s;
}

std::string rssInfo() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while(std::getline(f, line)) {
        if(line.rfind("VmRSS:", 0) == 0) {
            std::istringstream is(line.substr(6));
            long kb = 0;
            is >> kb;
            return "rss_mb=" + std::to_string(kb/1024);
        }
    }
    return "rss_mb=?";
}

void logLine(const std::string& s) {
    SDL_Log("[autotest] %s", s.c_str());
    static FILE* f = nullptr;
    static bool tried = false;
    if(!tried) {
        tried = true;
        const char* const p = std::getenv("EZEUS_AUTOTEST_LOG");
        if(p && *p) f = std::fopen(p, "w");
    }
    if(f) {
        std::fprintf(f, "%s\n", s.c_str());
        std::fflush(f);
    }
}

void pushFinger(Uint32 type, const Cmd& c, eMainWindow& w) {
    if(c.fArgs.size() < 3) return;
    SDL_Event e{};
    e.type = type;
    e.tfinger.touchId = 1;
    e.tfinger.fingerId = std::stoll(c.fArgs[0]);
    int ww = w.width(), wh = w.height();
    if(w.window()) SDL_GetWindowSize(w.window(), &ww, &wh);
    e.tfinger.x = std::stof(c.fArgs[1])/ww;
    e.tfinger.y = std::stof(c.fArgs[2])/wh;
    e.tfinger.pressure = type == SDL_FINGERUP ? 0.f : 1.f;
    e.tfinger.timestamp = SDL_GetTicks();
    SDL_PushEvent(&e);
}

void execute(const Cmd& c, eMainWindow& w, const uint32_t now) {
    auto& s = st();
    if(c.fName == "finger_down") pushFinger(SDL_FINGERDOWN, c, w);
    else if(c.fName == "finger_move") pushFinger(SDL_FINGERMOTION, c, w);
    else if(c.fName == "finger_up") pushFinger(SDL_FINGERUP, c, w);
    else if(c.fName == "mouse_move" && c.fArgs.size() >= 2) {
        if(w.window()) SDL_WarpMouseInWindow(w.window(), std::stoi(c.fArgs[0]), std::stoi(c.fArgs[1]));
    } else if(c.fName == "click" && c.fArgs.size() >= 3) {
        Uint8 b = SDL_BUTTON_LEFT;
        if(c.fArgs[0] == "right") b = SDL_BUTTON_RIGHT;
        else if(c.fArgs[0] == "middle") b = SDL_BUTTON_MIDDLE;
        const int x = std::stoi(c.fArgs[1]);
        const int y = std::stoi(c.fArgs[2]);
        if(w.window()) SDL_WarpMouseInWindow(w.window(), x, y);
        SDL_Event e{};
        e.type = SDL_MOUSEMOTION;
        e.motion.x = x;
        e.motion.y = y;
        SDL_PushEvent(&e);
        SDL_Event d{};
        d.type = SDL_MOUSEBUTTONDOWN;
        d.button.button = b;
        d.button.x = x;
        d.button.y = y;
        d.button.state = SDL_PRESSED;
        SDL_PushEvent(&d);
        SDL_Event u = d;
        u.type = SDL_MOUSEBUTTONUP;
        u.button.state = SDL_RELEASED;
        SDL_PushEvent(&u);
    } else if(c.fName == "key" && !c.fArgs.empty()) {
        std::string name = c.fArgs[0]; // names may contain spaces ("AC Back")
        for(size_t i = 1; i < c.fArgs.size(); i++) name += " " + c.fArgs[i];
        const auto sc = SDL_GetScancodeFromName(name.c_str());
        SDL_Event e{};
        e.type = SDL_KEYDOWN;
        e.key.keysym.scancode = sc;
        e.key.keysym.sym = SDL_GetKeyFromScancode(sc);
        SDL_PushEvent(&e);
        SDL_Event u = e;
        u.type = SDL_KEYUP;
        SDL_PushEvent(&u);
    } else if(c.fName == "screenshot" && !c.fArgs.empty()) {
        s.fPendingShot = c.fArgs[0];
    } else if(c.fName == "log") {
        std::ostringstream o;
        o << "t=" << now << " " << c.fRest;
        const auto gw = w.gameWidget();
        const bool gameShown = gw && w.currentWidget() == static_cast<eWidget*>(gw);
        o << " game=" << (gameShown ? 1 : 0) << " " << rssInfo();
        if(gameShown) o << " " << gw->debugCameraState();
        logLine(o.str());
    } else if(c.fName == "debug_banner" && c.fArgs.size() >= 2) {
        // Test-only: a player soldier banner (no soldiers) at tile tx ty.
        if(const auto gw = w.gameWidget()) {
            gw->debugSpawnBanner(std::stoi(c.fArgs[0]), std::stoi(c.fArgs[1]));
        }
    } else if(c.fName == "debug_build" && c.fArgs.size() >= 3) {
        // Test-only: place "house" | "park" | "gazebo" | "hedgemaze" at tx ty.
        if(const auto gw = w.gameWidget()) {
            const bool ok = gw->debugBuild(c.fArgs[0], std::stoi(c.fArgs[1]),
                                           std::stoi(c.fArgs[2]));
            logLine("debug_build " + c.fRest + (ok ? " ok" : " failed"));
        }
    } else if(c.fName == "debug_des" && c.fArgs.size() >= 2) {
        // Test-only: desirability at tile tx ty (+ building, level, missing).
        if(const auto gw = w.gameWidget()) {
            logLine("t=" + std::to_string(now) + " " + gw->debugDesirability(
                        std::stoi(c.fArgs[0]), std::stoi(c.fArgs[1])));
        }
    } else if(c.fName == "wait_game") {
        s.fWaiting = true;
        const uint32_t timeout = c.fArgs.empty() ? 60000 : std::stoul(c.fArgs[0]);
        s.fWaitUntil = now + timeout;
    } else if(c.fName == "quit") {
        logLine("quit");
        SDL_Event e{};
        e.type = SDL_QUIT;
        SDL_PushEvent(&e);
    } else {
        logLine("unknown command: " + c.fName);
    }
}

} // namespace

bool eAutotest::active() { return st().fActive; }

bool eAutotest::forceTouch() {
    const char* const t = std::getenv("EZEUS_TOUCH");
    return t && std::string(t) == "1";
}

void eAutotest::start(eMainWindow& w) {
    (void)w;
    auto& s = st();
    const char* const path = std::getenv("EZEUS_AUTOTEST");
    if(!path || !*path) return;
    std::ifstream f(path);
    if(!f) {
        logLine(std::string("cannot open script ") + path);
        return;
    }
    std::string line;
    while(std::getline(f, line)) {
        const auto hash = line.find('#');
        if(hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream is(line);
        Cmd c;
        if(!(is >> c.fDelay >> c.fName)) continue;
        std::string a;
        std::getline(is, c.fRest);
        std::istringstream as(c.fRest);
        while(as >> a) c.fArgs.push_back(a);
        s.fCmds.push_back(c);
    }
    s.fActive = true;
    s.fLastMs = SDL_GetTicks();
    logLine("loaded " + std::to_string(s.fCmds.size()) + " commands from " + path);
}

void eAutotest::frame(eMainWindow& w, const uint32_t nowMs) {
    auto& s = st();
    if(!s.fActive) return;
    if(s.fWaiting) {
        const auto gw = w.gameWidget();
        const bool shown = gw && w.currentWidget() == static_cast<eWidget*>(gw);
        if(shown || nowMs >= s.fWaitUntil) {
            logLine(std::string("wait_game ") + (shown ? "ok" : "TIMEOUT") +
                    " t=" + std::to_string(nowMs));
            s.fWaiting = false;
            s.fLastMs = nowMs;
        } else {
            return;
        }
    }
    while(s.fNext < s.fCmds.size()) {
        const auto& c = s.fCmds[s.fNext];
        if(nowMs - s.fLastMs < c.fDelay) break;
        s.fLastMs = nowMs;
        s.fNext++;
        execute(c, w, nowMs);
        if(s.fWaiting) break;
    }
}

void eAutotest::beforePresent(eMainWindow& w) {
    auto& s = st();
    if(s.fPendingShot.empty()) return;
    const auto r = w.renderer();
    int ow = 0, oh = 0;
    SDL_GetRendererOutputSize(r, &ow, &oh);
    SDL_Surface* const surf = SDL_CreateRGBSurfaceWithFormat(0, ow, oh, 32, SDL_PIXELFORMAT_ARGB8888);
    if(surf) {
        if(SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, surf->pixels, surf->pitch) == 0) {
            SDL_SaveBMP(surf, s.fPendingShot.c_str());
            logLine("screenshot " + s.fPendingShot);
        } else {
            logLine(std::string("screenshot failed: ") + SDL_GetError());
        }
        SDL_FreeSurface(surf);
    }
    s.fPendingShot.clear();
}
