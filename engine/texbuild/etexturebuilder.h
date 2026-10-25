#ifndef ETEXTUREBUILDER_H
#define ETEXTUREBUILDER_H

// Port (texture builder): makes the texture packs (interface.e, i15.e,
// i30.e, optionally i45.e / i60.e) from the user's own original game files
// (DATA/*.sg3 + *.555), so no external download is needed. Every atlas frame
// comes from one original sprite (table in etexmap.inc: indices and crop
// offsets only); the 30 size is the original pixels, 15/45/60 are scaled.
// Output pages are QOI images; "<pack>.idx" lists name, position, size.

#include <functional>
#include <string>

namespace eTextureBuilder {
    using eProgress = std::function<void(double fraction, const std::string& what)>;

    // True when the original sprite files are present in dataDir.
    bool sourcesPresent(const std::string& dataDir);
    // True when packs are missing in outDir (interface.e, or both i15/i30).
    bool needed(const std::string& outDir);
    // Builds the missing packs. withLarge: also i45.e / i60.e (map textures
    // for higher zoom). Returns false and sets error on failure.
    bool build(const std::string& dataDir, const std::string& outDir,
               const bool withLarge, const eProgress& progress,
               std::string& error);
    // Statistics of the last build (frames written / without source).
    int lastFrames();
    int lastMissing();
}

#endif // ETEXTUREBUILDER_H
