#ifndef EMESSAGELOG_H
#define EMESSAGELOG_H

// Port: the list of received messages behind the original's Messages button
// (the scroll with the number under the build buttons) and the message text
// box of the right panel. eZeus shows every message as a pop-up but kept no
// list; the port records each shown message here. Saved next to a saved game
// as "<save>.msgs" (the eZeus save format is left unchanged).

#include <string>
#include <vector>

#include "engine/edate.h"

struct eLoggedMessage {
    eDate fDate;
    std::string fTitle;   // with names etc. filled in
    std::string fText;
    int fTileX = -1;      // site of the event, -1: none
    int fTileY = -1;
    int fCity = -1;       // eCityId of a city target, -1: none
    bool fOpened = true;  // read (pop-ups count as read)
};

class eMessageLog {
public:
    static const int sMaxMessages = 200;

    // Newest first, like the original list.
    const std::vector<eLoggedMessage>& items() const { return mItems; }
    int size() const { return static_cast<int>(mItems.size()); }
    bool empty() const { return mItems.empty(); }

    void add(const eLoggedMessage& m);
    void remove(const int i);
    void removeOpened();
    void markOpened(const int i);
    void clear() { mItems.clear(); }

    bool save(const std::string& path) const;
    bool load(const std::string& path);

    // Text form used by save/load (one message per record), for tests.
    static std::string sEscape(const std::string& s);
    static std::string sUnescape(const std::string& s);
private:
    std::vector<eLoggedMessage> mItems;
};

#endif // EMESSAGELOG_H
