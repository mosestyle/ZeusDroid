#include "eplatformpaths.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_filesystem.h>
#include <SDL2/SDL_system.h>

#include <cstdlib>
#include <filesystem>
#include <system_error>

std::string ePlatformPaths::withSlash(const std::string& path) {
    if(path.empty()) return path;
    const char c = path.back();
    if(c == '/' || c == '\\') return path;
    return path + "/";
}

bool ePlatformPaths::makeDirs(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return !ec && std::filesystem::is_directory(path, ec);
}

std::string ePlatformPaths::androidDataRoot() {
#ifdef __ANDROID__
    const char* const ext = SDL_AndroidGetExternalStoragePath();
    if(ext) return withSlash(ext) + "ZeusPoseidon/";
    const char* const internal = SDL_AndroidGetInternalStoragePath();
    if(internal) return withSlash(internal) + "ZeusPoseidon/";
#endif
    return std::string();
}

std::string ePlatformPaths::binDir() {
    const char* const env = std::getenv("EZEUS_BIN_DIR");
    if(env && *env) return withSlash(env);
#ifdef __ANDROID__
    const auto root = androidDataRoot();
    if(!root.empty()) {
        const auto bin = root + "eZeus/Bin/";
        makeDirs(bin);
        return bin;
    }
#endif
    char* const base = SDL_GetBasePath();
    if(!base) return "./";
    std::string result(base);
    SDL_free(base);
    return withSlash(result);
}
