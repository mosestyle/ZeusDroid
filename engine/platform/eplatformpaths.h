#ifndef EPLATFORMPATHS_H
#define EPLATFORMPATHS_H

#include <string>

// Platform-specific filesystem roots. Game systems must not call SDL path
// functions directly; they go through eGameDir, which uses this.
namespace ePlatformPaths {
    // Directory that plays the role of the desktop "eZeus/Bin/" folder,
    // always ending with '/'. Order of precedence:
    //   1. EZEUS_BIN_DIR environment variable (tests, custom installs)
    //   2. Android: <external app files>/ZeusPoseidon/eZeus/Bin/
    //   3. SDL_GetBasePath()
    std::string binDir();

    // Root of user-owned data on Android (<external app files>/ZeusPoseidon/),
    // empty on desktop.
    std::string androidDataRoot();

    // Creates a directory and its parents; returns false on failure.
    bool makeDirs(const std::string& path);

    // Ensures trailing '/'.
    std::string withSlash(const std::string& path);
}

#endif // EPLATFORMPATHS_H
