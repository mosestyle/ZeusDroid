#ifndef ECHOOSEGAMEEDITMENU_H
#define ECHOOSEGAMEEDITMENU_H

#include "emainmenubase.h"

#include "engine/ecampaign.h"

class eBitmapWidget;

class eChooseGameEditMenu : public eMainMenuBase {
public:
    using eMainMenuBase::eMainMenuBase;
    // Port: which adventures the list shows.
    enum class eList { all, original, custom };
    void initialize(const bool editor, const eList list = eList::all);

    void setGlossary(const eCampaignGlossary& g);
    // All adventures shown in the list: eZeus folders + original .pak files
    // (Zeus and Poseidon, incl. custom adventures in sub-folders).
    static std::vector<eCampaignGlossary> sListAdventures(const eList list = eList::all);
private:
    eCampaignGlossary mSelected;
    eBitmapWidget* mBitmap = nullptr;
    eLabel* mTitle = nullptr;
    eLabel* mDesc = nullptr;
};

#endif // ECHOOSEGAMEEDITMENU_H
