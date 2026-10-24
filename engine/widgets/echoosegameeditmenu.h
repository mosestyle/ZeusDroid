#ifndef ECHOOSEGAMEEDITMENU_H
#define ECHOOSEGAMEEDITMENU_H

#include "emainmenubase.h"

#include "engine/ecampaign.h"

class eBitmapWidget;

class eChooseGameEditMenu : public eMainMenuBase {
public:
    using eMainMenuBase::eMainMenuBase;
    void initialize(const bool editor);

    void setGlossary(const eCampaignGlossary& g);
    // All adventures shown in the list: eZeus folders + original .pak files
    // (Zeus and Poseidon, incl. custom adventures in sub-folders).
    static std::vector<eCampaignGlossary> sListAdventures();
private:
    eCampaignGlossary mSelected;
    eBitmapWidget* mBitmap = nullptr;
    eLabel* mTitle = nullptr;
    eLabel* mDesc = nullptr;
};

#endif // ECHOOSEGAMEEDITMENU_H
