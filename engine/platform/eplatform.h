#ifndef EPLATFORM_H
#define EPLATFORM_H

#include <string>

class eSettings;

// Thin platform integration layer. Keeps Android specifics out of the
// simulation and widget code.
namespace ePlatform {
    // SDL hints that must be set before SDL_Init().
    void preInitHints();

    // Platform overrides applied after settings are read (e.g. Android always
    // runs fullscreen at the native display size).
    void adjustSettings(eSettings& settings);

    // Converts Zeus_Text.eng / Zeus_MM.eng from the game root into the XML
    // files eZeus reads, if they are missing. Returns false only on a hard
    // conversion error (missing source files are reported, not fatal).
    bool prepareLanguageFiles();

    // Logs and shows a blocking message box (Android has no console).
    void fatal(const std::string& message);

    // Informational log routed to logcat on Android, stdout on desktop.
    void log(const std::string& message);

    bool isTouchPlatform();

    // Touch UI mode, set by eMainWindow (touch platform or forced by tests).
    void setTouchUi(bool touch, int screenHeight);
    bool touchUi();
    // List rows: at least ~1/13 of the screen height in touch mode so they
    // can be hit with a finger; returns h unchanged otherwise.
    int touchRowHeight(int h);

    // Resident memory of this process in MB (Linux/Android), -1 if unknown.
    int residentMemoryMB();

    // Frames per second for painting. 20 = upstream behaviour (every frame
    // is a simulation tick). Override with EZEUS_RENDER_FPS (20..120).
    double renderFps();
}

#endif // EPLATFORM_H
