#include "eengconverter.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>

namespace {

const int kTextMaxIndexEntries = 1000;
const size_t kMaxDataSize = 1000000;

int32_t rd32(const std::vector<unsigned char>& d, size_t o) {
    return static_cast<int32_t>(uint32_t(d[o]) | (uint32_t(d[o + 1]) << 8) |
                                (uint32_t(d[o + 2]) << 16) | (uint32_t(d[o + 3]) << 24));
}

int16_t rd16(const std::vector<unsigned char>& d, size_t o) {
    return static_cast<int16_t>(uint16_t(d[o]) | (uint16_t(d[o + 1]) << 8));
}

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for(const char c : s) {
        switch(c) {
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '&': out += "&amp;"; break;
        case '"': out += "&quot;"; break;
        default: out += c;
        }
    }
    return out;
}

std::string header16(const std::vector<unsigned char>& d) {
    std::string name;
    for(int i = 0; i < 16; i++) {
        const char c = static_cast<char>(d[i]);
        if(!c) break;
        name += (static_cast<unsigned char>(c) < 0x80) ? c : '?';
    }
    return name;
}

// Returns size after removing duplicate trailing NULs (as upstream does).
size_t trimmedSize(const unsigned char* raw, size_t size) {
    while(size > 1 && raw[size - 1] == 0 && raw[size - 2] == 0) size--;
    return size;
}

std::string cstrAt(const unsigned char* raw, size_t len, size_t start) {
    std::string s;
    for(size_t i = start; i < len && raw[i]; i++) s += static_cast<char>(raw[i]);
    return eEngConverter::cp1252ToUtf8(s);
}

enum class eType { unknown, text, message };

eType detect(const std::vector<unsigned char>& d) {
    if(d.size() < 24) return eType::unknown;
    const int32_t first = rd32(d, 16);
    const int32_t second = rd32(d, 20);
    if((first == 400 || first == 1000) && first >= second) return eType::message;
    if(first <= 400 && first <= second) return eType::text;
    const size_t off = 16 + 8 + 8000;
    if(d.size() >= off + 8) {
        const uint32_t zero = static_cast<uint32_t>(rd32(d, off));
        const uint32_t nonzero = static_cast<uint32_t>(rd32(d, off + 4));
        if(zero == 0 && nonzero != 0) return eType::text;
    }
    return eType::unknown;
}

bool convertText(const std::vector<unsigned char>& d, std::string& xml, std::string& err) {
    size_t pos = 16 + 12;
    if(d.size() < pos + kTextMaxIndexEntries*8) {
        err = "text file truncated in index table";
        return false;
    }
    struct Group { int id; int32_t offset; std::vector<std::string> strings; };
    std::vector<Group> groups;
    bool withCounts = false;
    for(int id = 0; id < kTextMaxIndexEntries; id++) {
        const int32_t offset = rd32(d, pos);
        const int32_t used = rd32(d, pos + 4);
        pos += 8;
        if(used) {
            groups.push_back({id, offset, {}});
            if(used > 1) withCounts = true;
        }
    }
    const unsigned char* raw = d.data() + pos;
    const size_t rawLen = std::min(d.size() - pos, kMaxDataSize);
    const size_t size = trimmedSize(raw, rawLen);
    for(size_t i = 0; i < groups.size(); i++) {
        auto& g = groups[i];
        long start = g.offset;
        const long end = i + 1 == groups.size() ? long(size) : long(groups[i + 1].offset);
        if(start > long(size) || end > long(size) || start > end || start < 0) {
            err = "invalid data offset for group " + std::to_string(g.id);
            return false;
        }
        while(start < end) {
            while(start < end && raw[start] == 0) start++;
            g.strings.push_back(cstrAt(raw, rawLen, start));
            while(size_t(start) < rawLen && raw[start] != 0) start++;
            start++;
        }
    }
    std::ostringstream o;
    o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    o << "<strings name=\"" << escape(header16(d)) << "\" indexWithCounts=\""
      << (withCounts ? "true" : "false") << "\">\n";
    for(const auto& g : groups) {
        o << "    <group id=\"" << g.id << "\">\n";
        for(size_t i = 0; i < g.strings.size(); i++) {
            o << "        <string id=\"" << i << "\">" << escape(g.strings[i]) << "</string>\n";
        }
        o << "    </group>\n";
    }
    o << "</strings>\n";
    xml = o.str();
    return true;
}

bool convertMessages(const std::vector<unsigned char>& d, std::string& xml, std::string& err) {
    const int32_t total = rd32(d, 16);
    if(total < 0 || total > 100000) {
        err = "implausible message count";
        return false;
    }
    const size_t entrySize = 80;
    size_t pos = 24;
    if(d.size() < pos + size_t(total)*entrySize) {
        err = "message file truncated in entry table";
        return false;
    }
    struct Msg {
        int id;
        int type, subtype;
        int dlg[4];
        int img[3];
        int img2[3];
        int titleXY[2], subtitleXY[2], videoXY[2];
        int32_t urgent;
        int32_t offVideo, offTitle, offSubtitle, offContent;
    };
    std::vector<Msg> entries;
    for(int id = 0; id < total; id++) {
        const size_t b = pos;
        Msg m;
        m.id = id;
        m.type = rd16(d, b + 0);
        m.subtype = rd16(d, b + 2);
        for(int i = 0; i < 4; i++) m.dlg[i] = rd16(d, b + 6 + 2*i);
        for(int i = 0; i < 3; i++) m.img[i] = rd16(d, b + 14 + 2*i);
        for(int i = 0; i < 3; i++) m.img2[i] = rd16(d, b + 20 + 2*i);
        m.titleXY[0] = rd16(d, b + 26); m.titleXY[1] = rd16(d, b + 28);
        m.subtitleXY[0] = rd16(d, b + 30); m.subtitleXY[1] = rd16(d, b + 32);
        m.videoXY[0] = rd16(d, b + 38); m.videoXY[1] = rd16(d, b + 40);
        m.urgent = rd32(d, b + 56);
        m.offVideo = rd32(d, b + 60);
        m.offTitle = rd32(d, b + 68);
        m.offSubtitle = rd32(d, b + 72);
        m.offContent = rd32(d, b + 76);
        pos += entrySize;
        const bool empty = !m.img[0] && !m.img2[0] &&
                           !(m.dlg[0] || m.dlg[1] || m.dlg[2] || m.dlg[3]) &&
                           !m.offVideo && !m.offTitle && !m.offSubtitle && !m.offContent;
        if(!empty) entries.push_back(m);
    }
    const unsigned char* raw = d.data() + pos;
    const size_t rawLen = std::min(d.size() - pos, kMaxDataSize);
    const size_t size = trimmedSize(raw, rawLen);
    const auto text = [&](int32_t off, std::string& out) {
        out.clear();
        if(!off) return true;
        if(off < 0 || size_t(off) > size) return false;
        out = cstrAt(raw, rawLen, off);
        return true;
    };
    std::ostringstream o;
    o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    o << "<messages name=\"" << escape(header16(d)) << "\" entries=\"" << total << "\">\n";
    for(const auto& m : entries) {
        std::string video, title, subtitle, content;
        if(!text(m.offVideo, video) || !text(m.offTitle, title) ||
           !text(m.offSubtitle, subtitle) || !text(m.offContent, content)) {
            err = "invalid text offset in entry " + std::to_string(m.id);
            return false;
        }
        o << "    <message id=\"" << m.id << "\" type=\"" << m.type << "\" subtype=\""
          << m.subtype << "\"" << (m.urgent ? " urgent=\"true\"" : "") << ">\n";
        o << "        <dialog x=\"" << m.dlg[0] << "\" y=\"" << m.dlg[1] << "\" width=\""
          << m.dlg[2] << "\" height=\"" << m.dlg[3] << "\"/>\n";
        if(m.img[0]) {
            o << "        <image graphic=\"" << m.img[0] << "\" x=\"" << m.img[1]
              << "\" y=\"" << m.img[2] << "\"/>\n";
        }
        if(m.img2[0]) {
            // Upstream engconverter writes image2 using image() values.
            o << "        <image2 graphic=\"" << m.img[0] << "\" x=\"" << m.img[1]
              << "\" y=\"" << m.img[2] << "\"/>\n";
        }
        const auto tag = [&o](const char* name, int32_t off, const std::string& t, const int* xy) {
            if(off == 0 && t.empty()) return;
            o << "        <" << name << " x=\"" << xy[0] << "\" y=\"" << xy[1] << "\">"
              << escape(t) << "</" << name << ">\n";
        };
        tag("title", m.offTitle, title, m.titleXY);
        tag("subtitle", m.offSubtitle, subtitle, m.subtitleXY);
        tag("video", m.offVideo, video, m.videoXY);
        if(!content.empty()) {
            for(auto& c : content) if(c == '\x0e') c = '~';
            o << "        <content>" << escape(content) << "</content>\n";
        }
        o << "    </message>\n";
    }
    o << "</messages>\n";
    xml = o.str();
    return true;
}

} // namespace

std::string eEngConverter::cp1252ToUtf8(const std::string& in) {
    // 0x80-0x9F differ from Latin-1; undefined positions map to U+FFFD like
    // Python's errors="replace".
    static const uint16_t c1[32] = {
        0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
        0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
        0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178};
    std::string out;
    out.reserve(in.size());
    for(const char ch : in) {
        const unsigned char u = static_cast<unsigned char>(ch);
        uint32_t cp = u;
        if(u >= 0x80 && u < 0xA0) cp = c1[u - 0x80];
        if(cp < 0x80) {
            out += static_cast<char>(cp);
        } else if(cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

bool eEngConverter::convert(const std::vector<unsigned char>& data,
                            std::string& xml, std::string& err) {
    switch(detect(data)) {
    case eType::text: return convertText(data, xml, err);
    case eType::message: return convertMessages(data, xml, err);
    default: break;
    }
    err = "unknown .eng file type";
    return false;
}

bool eEngConverter::convertFile(const std::string& src, const std::string& dst,
                                std::string& err) {
    std::ifstream in(src, std::ios::binary);
    if(!in) {
        err = "cannot open " + src;
        return false;
    }
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
    std::string xml;
    if(!convert(data, xml, err)) return false;
    const std::string tmp = dst + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if(!out) {
            err = "cannot write " + tmp;
            return false;
        }
        out.write(xml.data(), static_cast<std::streamsize>(xml.size()));
        if(!out) {
            err = "write failed for " + tmp;
            return false;
        }
    }
    if(std::rename(tmp.c_str(), dst.c_str()) != 0) {
        std::remove(tmp.c_str());
        err = "cannot rename " + tmp + " to " + dst;
        return false;
    }
    return true;
}
