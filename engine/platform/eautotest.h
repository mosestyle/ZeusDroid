#ifndef EAUTOTEST_H
#define EAUTOTEST_H

#include <cstdint>

class eMainWindow;

// Scripted input driver for reproducible integration tests of the real engine
// (desktop under Xvfb, or on device via adb-pushed script). Activated only when
// the EZEUS_AUTOTEST environment variable names a script file; otherwise every
// function is a no-op. Script format (one command per line, '#' comments):
//
//   <delay_ms> finger_down <id> <x> <y>      window pixels
//   <delay_ms> finger_move <id> <x> <y>
//   <delay_ms> finger_up <id> <x> <y>
//   <delay_ms> mouse_move <x> <y>
//   <delay_ms> click <left|right|middle> <x> <y>
//   <delay_ms> key <SDL scancode name, e.g. Escape>
//   <delay_ms> screenshot <path.bmp>
//   <delay_ms> log <free text>                logs camera/gesture state
//   <delay_ms> wait_game <timeout_ms>         block until game view shown
//   <delay_ms> quit
//
// delay_ms is relative to the previous command. Events are injected with
// SDL_PushEvent so they travel the same path as real input.
namespace eAutotest {
    bool active();
    bool forceTouch();   // EZEUS_TOUCH=1 enables the touch layer on desktop
    void start(eMainWindow& w);
    void frame(eMainWindow& w, uint32_t nowMs);
    void beforePresent(eMainWindow& w);
}

#endif // EAUTOTEST_H
