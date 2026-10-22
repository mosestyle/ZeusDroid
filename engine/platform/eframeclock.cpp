#include "eframeclock.h"

namespace {
bool gTick = true;
}

bool eFrameClock::tick() { return gTick; }
void eFrameClock::setTick(const bool t) { gTick = t; }
