#ifndef ESG3FILE_H
#define ESG3FILE_H

// Port (texture builder): reader for the original game's sprite files
// (DATA/*.sg3 index + DATA/*.555 pixel data). Written for this project from
// observations of the user's own files; see docs/TEXTURE_BUILDER.md.
// No SDL dependency: decodes to 8-bit RGBA in memory.

#include <cstdint>
#include <string>
#include <vector>

struct eSg3Image {
    int fW = 0;
    int fH = 0;
    std::vector<uint8_t> fRGBA; // fW*fH*4
    bool empty() const { return fW <= 0 || fH <= 0; }
};

class eSg3File {
public:
    // path: ".../DATA/Name.sg3"; the pixel data is read from "Name.555".
    bool open(const std::string& sg3Path);
    bool isOpen() const { return mOpen; }

    int imageCount() const { return mImageCount; }
    int bitmapCount() const { return static_cast<int>(mBitmapStart.size()); }
    // First image index of a bitmap (group) and its number of images.
    int bitmapStart(const int b) const { return mBitmapStart[b]; }
    int bitmapImages(const int b) const { return mBitmapImages[b]; }

    // Decodes image i (record index as stored in the .sg3). Mirror records
    // return the referenced image flipped horizontally.
    // shadows: pure red (0x7C00) in sprite data becomes half-transparent
    // black (used by most sprite files; not where red is a real colour).
    bool decode(const int i, eSg3Image& out, const bool shadows = true);
    // Size stored in the record (before any cropping).
    bool recordSize(const int i, int& w, int& h) const;
private:
    struct eRecord {
        uint32_t fOffset = 0;
        uint32_t fLength = 0;
        uint32_t fULength = 0;
        int32_t fMirror = 0;
        int fW = 0;
        int fH = 0;
        int fType = 0;
        int fTop = 0;
        int fTiles = 0;
        bool fExternal = false; // pixels in "<bitmap name>.555", offset - 1
        int fBitmap = 0;
        uint32_t fAlphaOffset = 0;
        uint32_t fAlphaLength = 0;
    };
    bool record(const int i, eRecord& r) const;
    bool readData(const uint32_t offset, const uint32_t length,
                  std::vector<uint8_t>& out);
    bool readData(const eRecord& r, const uint32_t offset, const uint32_t length,
                  std::vector<uint8_t>& out);
    std::string externalPath(const int bitmap) const;

    bool mOpen = false;
    uint32_t mVersion = 0;
    int mRecordSize = 64;
    int mImageCount = 0;
    std::vector<uint8_t> mIndex;    // whole .sg3
    std::string m555Path;
    std::vector<int> mBitmapStart;
    std::vector<int> mBitmapImages;
    std::vector<std::string> mBitmapNames;
    std::string mDir;
};

#endif // ESG3FILE_H
