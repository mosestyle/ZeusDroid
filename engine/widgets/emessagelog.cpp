#include "emessagelog.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

void eMessageLog::add(const eLoggedMessage& m) {
    mItems.insert(mItems.begin(), m);
    if(size() > sMaxMessages) mItems.resize(sMaxMessages);
}

void eMessageLog::remove(const int i) {
    if(i < 0 || i >= size()) return;
    mItems.erase(mItems.begin() + i);
}

void eMessageLog::removeOpened() {
    std::vector<eLoggedMessage> keep;
    for(const auto& m : mItems) {
        if(!m.fOpened) keep.push_back(m);
    }
    mItems.swap(keep);
}

void eMessageLog::markOpened(const int i) {
    if(i < 0 || i >= size()) return;
    mItems[i].fOpened = true;
}

std::string eMessageLog::sEscape(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for(const char c : s) {
        if(c == '\\') r += "\\\\";
        else if(c == '\n') r += "\\n";
        else if(c == '\r') continue;
        else r += c;
    }
    return r;
}

std::string eMessageLog::sUnescape(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for(size_t i = 0; i < s.size(); i++) {
        const char c = s[i];
        if(c == '\\' && i + 1 < s.size()) {
            const char n = s[++i];
            r += n == 'n' ? '\n' : n;
        } else {
            r += c;
        }
    }
    return r;
}

bool eMessageLog::save(const std::string& path) const {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream o(tmp, std::ios::out | std::ios::trunc | std::ios::binary);
        if(!o) return false;
        o << "zeusdroid-messages 1\n" << mItems.size() << "\n";
        for(const auto& m : mItems) {
            o << m.fDate.day() << ' ' << static_cast<int>(m.fDate.month()) << ' '
              << m.fDate.year() << ' ' << m.fTileX << ' ' << m.fTileY << ' '
              << m.fCity << ' ' << (m.fOpened ? 1 : 0) << "\n"
              << sEscape(m.fTitle) << "\n" << sEscape(m.fText) << "\n";
        }
        if(!o.good()) return false;
    }
    std::remove(path.c_str());
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}

bool eMessageLog::load(const std::string& path) {
    mItems.clear();
    std::ifstream in(path, std::ios::binary);
    if(!in) return false;
    std::string line;
    if(!std::getline(in, line) || line != "zeusdroid-messages 1") return false;
    if(!std::getline(in, line)) return false;
    const int n = std::atoi(line.c_str());
    for(int i = 0; i < n && i < sMaxMessages; i++) {
        if(!std::getline(in, line)) break;
        std::istringstream is(line);
        int d, mo, y, tx, ty, city, opened;
        if(!(is >> d >> mo >> y >> tx >> ty >> city >> opened)) break;
        if(mo < 0 || mo > 11) break;
        eLoggedMessage m;
        m.fDate = eDate(d, static_cast<eMonth>(mo), y);
        m.fTileX = tx;
        m.fTileY = ty;
        m.fCity = city;
        m.fOpened = opened != 0;
        std::string title;
        std::string text;
        if(!std::getline(in, title) || !std::getline(in, text)) break;
        m.fTitle = sUnescape(title);
        m.fText = sUnescape(text);
        mItems.push_back(m);
    }
    return true;
}
