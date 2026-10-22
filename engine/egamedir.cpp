#include "egamedir.h"

#include <SDL2/SDL_filesystem.h>
#include <SDL2/SDL_system.h>
#include <fstream>
#include <cstdlib>

#include "platform/eplatformpaths.h"

std::string eGameDir::sPath;
std::string eGameDir::sExeDir;

void eGameDir::initialize() {
    // The engine keeps the desktop layout <game root>/<eZeus dir>/Bin/ and
    // resolves everything relative to the "binary" directory. The platform
    // layer decides where that directory is (SDL base path on desktop,
    // app-owned external storage on Android, EZEUS_BIN_DIR override for tests).
    sExeDir = ePlatformPaths::binDir();
    sPath = exeDir() + "../../";
    const auto zp = exeDir() + "../zeus_path.txt";
    std::ifstream file(zp);
    if(!file.good()) return;
    std::string str;
    const bool g = !!std::getline(file, str);
    if(!g) return;
    sPath = exeDir() + str;
}

std::string eGameDir::path(const std::string& path) {
    return sPath + path;
}

std::string eGameDir::settingsPath() {
    return exeDir() + "../settings.txt";
}

std::string eGameDir::numbersPath() {
    return exeDir() + "../numbers.txt";
}

std::string eGameDir::iBinaryPath() {
    return exeDir() + "../interface.e";
}

std::string eGameDir::i15BinaryPath() {
    return exeDir() + "../i15.e";
}

std::string eGameDir::i30BinaryPath() {
    return exeDir() + "../i30.e";
}

std::string eGameDir::i45BinaryPath() {
    return exeDir() + "../i45.e";
}

std::string eGameDir::i60BinaryPath() {
    return exeDir() + "../i60.e";
}

std::string eGameDir::exeDir() {
    if(sExeDir.empty()) sExeDir = ePlatformPaths::binDir();
    return sExeDir;
}

std::string eGameDir::adventuresDir() {
    return exeDir() + "../Adventures/";
}

std::string eGameDir::pakAdventuresDir() {
    // return "/home/ailuropoda/.eZeus/Zeus/Adventures/"; // !!!
    return eGameDir::path("Adventures/");
}

std::string eGameDir::saveDir() {
    return exeDir() + "../Save/";
}

std::string eGameDir::texturesDir() {
    return exeDir() + "../Textures/";
}
