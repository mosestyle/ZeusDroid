#ifndef __ANDROID__
// On Android SDLActivity calls SDL_main() from libmain.so, so SDL_main.h must
// rename main(); desktop builds keep eZeus' own entry point.
#define SDL_MAIN_HANDLED
#endif
#include "texbuild/etexturebuilder.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <cstdlib>

#include <string>

#include <filesystem>

#include "emainwindow.h"
#include "textures/egametextures.h"

#include "egamedir.h"
#include "enumbers.h"

#include "audio/emusic.h"
#include "audio/esounds.h"

#include "platform/eplatform.h"

bool init() {
    ePlatform::preInitHints();
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n",
               SDL_GetError());
        return false;
    }

    if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        printf("Warning: Linear texture filtering not enabled!");
    }

    const int imgFlags = IMG_INIT_PNG;
    if(!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n",
               IMG_GetError());
        return false;
    }

    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n",
               Mix_GetError());
        return false;
    }

    if(TTF_Init()) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n",
               TTF_GetError());
        return false;
    }

    return true;
}

void close() {
    TTF_Quit();
    IMG_Quit();
    Mix_Quit();
    SDL_Quit();
}

bool getDisplayResolution(SDL_DisplayMode& mode) {
    int display_count = 0, display_index = 0, mode_index = 0;
    mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

    if((display_count = SDL_GetNumVideoDisplays()) < 1) {
        SDL_Log("SDL_GetNumVideoDisplays returned: %i", display_count);
        return false;
    }

    if(SDL_GetDisplayMode(display_index, mode_index, &mode) != 0) {
        SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
        return false;
    }
    SDL_Log("SDL_GetDisplayMode(0, 0, &mode):\t\t%i bpp\t%i x %i",
    SDL_BITSPERPIXEL(mode.format), mode.w, mode.h);

    return true;
}

bool getDisplayResolutions(std::vector<SDL_DisplayMode>& resolutions) {
    const int display_count = SDL_GetNumVideoDisplays();

    SDL_Log("Number of displays: %i", display_count);

    for(int display_index = 0; display_index <= display_count; display_index++) {
        SDL_Log("Display %i:", display_index);

        const int modes_count = SDL_GetNumDisplayModes(display_index);

        for(int mode_index = 0; mode_index <= modes_count; mode_index++) {
            SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

            if (SDL_GetDisplayMode(display_index, mode_index, &mode) == 0) {
                SDL_Log(" %i bpp\t%i x %i @ %iHz",
                    SDL_BITSPERPIXEL(mode.format), mode.w, mode.h, mode.refresh_rate);

                resolutions.push_back(mode);
            }
        }
    }
    return true;
}

// Port (texture builder): when the texture packs are missing but the
// original game's sprite files are there, build the packs from them, with a
// simple progress screen (first start after importing the game folder).
static bool buildTexturesIfNeeded() {
    const std::string out = eGameDir::exeDir() + "..";
    const std::string data = eGameDir::path("DATA");
    if(!eTextureBuilder::needed(out)) return true;
    if(!eTextureBuilder::sourcesPresent(data)) return true; // old checks report it
    ePlatform::log("Building textures from " + data);
    SDL_Window* win = SDL_CreateWindow("ZeusDroid", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, 960, 540,
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED |
                                                          SDL_RENDERER_PRESENTVSYNC) : nullptr;
    TTF_Font* font = nullptr;
    int ww = 960;
    int wh = 540;
    if(ren) {
        SDL_GetRendererOutputSize(ren, &ww, &wh);
        font = TTF_OpenFont((eGameDir::exeDir() + "../Fonts/Zeus.ttf").c_str(), std::max(16, wh/24));
    }
    const auto text = [&](const std::string& t, const int y) {
        if(!font || t.empty()) return;
        SDL_Color c{0xF2, 0xE6, 0xC8, 0xFF};
        SDL_Surface* s = TTF_RenderUTF8_Blended(font, t.c_str(), c);
        if(!s) return;
        SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s);
        SDL_Rect r{(ww - s->w)/2, y, s->w, s->h};
        SDL_RenderCopy(ren, tx, nullptr, &r);
        SDL_DestroyTexture(tx);
        SDL_FreeSurface(s);
    };
    uint32_t last = 0;
    const auto progress = [&](const double f, const std::string& what) {
        const uint32_t now = SDL_GetTicks();
        if(now - last < 50 && f < 1) return;
        last = now;
        SDL_Event e;
        while(SDL_PollEvent(&e)) {}
        if(!ren) return;
        SDL_SetRenderDrawColor(ren, 0x10, 0x30, 0x50, 0xFF);
        SDL_RenderClear(ren);
        text("Preparing the game graphics from your game files", wh/4);
        text("(only once, about a minute)", wh/4 + wh/12);
        const int bw = ww*2/3;
        const int bh = std::max(12, wh/20);
        const SDL_Rect frame{(ww - bw)/2, wh/2, bw, bh};
        SDL_SetRenderDrawColor(ren, 0x40, 0x60, 0x80, 0xFF);
        SDL_RenderFillRect(ren, &frame);
        const SDL_Rect bar{frame.x, frame.y, static_cast<int>(bw*std::clamp(f, 0., 1.)), bh};
        SDL_SetRenderDrawColor(ren, 0xF2, 0xC8, 0x50, 0xFF);
        SDL_RenderFillRect(ren, &bar);
        text(std::to_string(static_cast<int>(f*100)) + " %  " + what, wh/2 + bh + wh/24);
        SDL_RenderPresent(ren);
    };
    std::string error;
    const char* const large = std::getenv("EZEUS_TEXBUILD_LARGE");
    const bool ok = eTextureBuilder::build(data, out, large && *large == '1', progress, error);
    if(font) TTF_CloseFont(font);
    if(ren) SDL_DestroyRenderer(ren);
    if(win) SDL_DestroyWindow(win);
    ePlatform::log("Texture builder: " + std::string(ok ? "done" : "failed: " + error) +
                   ", frames " + std::to_string(eTextureBuilder::lastFrames()) +
                   ", without source " + std::to_string(eTextureBuilder::lastMissing()));
    if(!ok) ePlatform::fatal("Could not prepare the game graphics: " + error);
    return ok;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if(!init()) {
        ePlatform::fatal("Failed to initialize SDL subsystems.");
        close();
        return 1;
    }

    eGameDir::initialize();
    ePlatform::log("eZeus data root: " + eGameDir::path(""));
    ePlatform::prepareLanguageFiles();

//    SDL_DisplayMode mode;
//    const bool r0 = getDisplayResolution(mode);
//    eResolution resolution;
//    if(r0) {
//        resolution = eResolution(mode.w, mode.h);
//    } else {
//        resolution = eResolution(1280, 720);
//    }

//    std::vector<SDL_DisplayMode> resolutions;
//    const bool r1 = getDisplayResolutions(resolutions);
//    if(r1 && !resolutions.empty()) {
//        eResolution::sResolutions = std::vector<eResolution>();
//        for(const auto& m : resolutions) {
//            eResolution::sResolutions.emplace_back(m.w, m.h);
//        }
//    }
    std::vector<SDL_DisplayMode> ress;
    const int displayCount = SDL_GetNumVideoDisplays();

    for(int i = 0; i < displayCount; i++) {
        SDL_DisplayMode dm;
        SDL_GetCurrentDisplayMode(i, &dm);
        bool contains = false;
        for(const auto& rr : eResolution::sResolutions) {
            contains = dm.w == rr.width() &&
                       dm.h == rr.height();
            if(contains) break;
        }
        if(!contains) {
            const auto res = eResolution(dm.w, dm.h);
            eResolution::sResolutions.push_back(res);
        }
    }

    if(!buildTexturesIfNeeded()) {
        close();
        return 1;
    }

    if(!std::filesystem::exists(eGameDir::iBinaryPath())) {
        ePlatform::fatal("Could not find interface textures (interface.e).\n"
                         "Expected at: " + eGameDir::iBinaryPath() + "\n"
                         "Import your game data again.");
        close();
        return 1;
    }

    eNumbers::sLoad();
    eSettings settings;
    settings.read();
    ePlatform::adjustSettings(settings);
    bool found = false;
    const auto checkTextureSize = [&found](const std::string& path,
                                           bool& setting) {
        if(!setting) return;
        setting = std::filesystem::exists(path);
        if(setting) found = true;
    };
    checkTextureSize(eGameDir::i15BinaryPath(), settings.fTinyTextures);
    checkTextureSize(eGameDir::i30BinaryPath(), settings.fSmallTextures);
    checkTextureSize(eGameDir::i45BinaryPath(), settings.fMediumTextures);
    checkTextureSize(eGameDir::i60BinaryPath(), settings.fLargeTextures);
    if(!found) {
        ePlatform::fatal("Could not find any textures!\n"
                         "Make sure the texture pack (i15.e, i30.e, i45.e or i60.e) is imported.");
        close();
        return 1;
    }

    int r = 0;
    {
        eMusic music;
        eSounds sounds;
        eMainWindow w;
        const bool i = w.initialize(settings);
        if(!i) {
            ePlatform::fatal("Could not create the game window.");
            close();
            return 1;
        }
        const bool e = eGameTextures::initialize(w.renderer());

        if(e) r = w.exec();
    }

    close();

    return r;
}
