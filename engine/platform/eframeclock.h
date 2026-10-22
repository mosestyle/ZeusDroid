#ifndef EFRAMECLOCK_H
#define EFRAMECLOCK_H

// Separates rendering rate from simulation rate.
//
// Upstream eZeus advances the city simulation, sprite animation frames and
// edge-scrolling once per painted frame and paints at a fixed 20 FPS. To allow
// smoother rendering (60 FPS camera/UI) without changing game speed, the main
// loop marks which painted frames are "simulation ticks" (exactly the 20 Hz
// cadence of the original loop). Code that advances game state checks tick().
// With the render rate set to 20 every frame is a tick, which is bit-for-bit
// the upstream behaviour.
namespace eFrameClock {
    bool tick();
    void setTick(bool t);
    // Simulation ticks per second (upstream: 20).
    constexpr double kTickRate = 20.0;
}

#endif // EFRAMECLOCK_H
