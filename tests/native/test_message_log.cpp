// Port: message list (Messages button) – save/load round trip of "<save>.msgs".
#include "widgets/emessagelog.h"

#include <cstdio>
#include <string>

// eDate's constructor lives in edate.cpp (which pulls in the language files);
// the test only needs the plain value constructor.
eDate::eDate(const int day, const eMonth month, const int year) :
    mDay(day), mMonth(month), mYear(year) {}

static int gChecks = 0;
static int gFails = 0;
#define CHECK(c) do { gChecks++; if(!(c)) { gFails++; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while(0)

int main() {
    eMessageLog log;
    for(int i = 0; i < 3; i++) {
        eLoggedMessage m;
        m.fDate = eDate(1 + i, eMonth::june, -1750);
        m.fTitle = "Title " + std::to_string(i);
        m.fText = "Line one\nLine \\two\\ " + std::to_string(i);
        m.fTileX = i == 1 ? 12 : -1;
        m.fTileY = i == 1 ? 34 : -1;
        m.fCity = i;
        m.fOpened = i != 2;
        log.add(m);
    }
    CHECK(log.size() == 3);
    CHECK(log.items().front().fTitle == "Title 2"); // newest first
    const std::string path = "/tmp/zeusdroid_test.msgs";
    CHECK(log.save(path));
    eMessageLog r;
    CHECK(r.load(path));
    CHECK(r.size() == 3);
    for(int i = 0; i < 3 && i < r.size(); i++) {
        const auto& a = log.items()[i];
        const auto& b = r.items()[i];
        CHECK(a.fTitle == b.fTitle);
        CHECK(a.fText == b.fText);
        CHECK(a.fDate.day() == b.fDate.day());
        CHECK(a.fDate.month() == b.fDate.month());
        CHECK(a.fDate.year() == b.fDate.year());
        CHECK(a.fTileX == b.fTileX && a.fTileY == b.fTileY);
        CHECK(a.fCity == b.fCity && a.fOpened == b.fOpened);
    }
    r.removeOpened();
    CHECK(r.size() == 1 && r.items()[0].fTitle == "Title 2");
    r.remove(0);
    CHECK(r.empty());
    eMessageLog big;
    for(int i = 0; i < eMessageLog::sMaxMessages + 20; i++) big.add(eLoggedMessage());
    CHECK(big.size() == eMessageLog::sMaxMessages);
    eMessageLog bad;
    CHECK(!bad.load("/nonexistent/x.msgs"));
    std::remove(path.c_str());
    std::printf("message log: %d checks, %d failures\n", gChecks, gFails);
    return gFails ? 1 : 0;
}
