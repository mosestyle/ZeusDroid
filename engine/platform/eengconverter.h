#ifndef EENGCONVERTER_H
#define EENGCONVERTER_H

#include <string>
#include <vector>

// Native port of the .eng -> XML conversion performed by the Windows-only
// engconverter tool that eZeus releases ship with. Format knowledge comes from
// the MIT-licensed citybuilding-tools engconverter (Bianca van Schaik); output
// is byte-compatible with tools/assets/eng_to_xml.py (checked by tests).
namespace eEngConverter {
    // Windows-1252 bytes -> UTF-8.
    std::string cp1252ToUtf8(const std::string& in);

    // Converts in-memory .eng data. Returns false and sets err on failure.
    bool convert(const std::vector<unsigned char>& data,
                 std::string& xml, std::string& err);

    // Reads src, writes dst atomically (temp file + rename).
    bool convertFile(const std::string& src, const std::string& dst,
                     std::string& err);
}

#endif // EENGCONVERTER_H
