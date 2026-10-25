#ifndef EWORLDMENU_H
#define EWORLDMENU_H

#include "elabel.h"

#include "ebutton.h"
#include "engine/eworldcity.h"
#include "pointers/estdselfref.h"

class eWorldGoodsWidget;
class eWorldTributeWidget;

class eWorldBoard;

class eWorldMenu : public eLabel {
public:
    using eLabel::eLabel;

    void initialize(const eAction& openRequest,
                    const eAction& openFulfill,
                    const eAction& openGift,
                    const eAction& openRaid,
                    const eAction& openConquer,
                    const bool showText = true);

    void setCity(const stdsptr<eWorldCity>& c);
    void setWorldBoard(eWorldBoard* const b);
    void setText(const std::string& text);
    void updateLabels() const;
    void updateButtonsEnabled() const;
    void updateButtonsEnabled0() const;

    // Port: previous / next city arrows, the goods Orders button.
    void setPrevNextAction(const std::function<void(bool)>& a) { mPrevNext = a; }
    void setOrdersAction(const eAction& a);
    // Port: the lowest rows (raid / conquer, help / back to city) did not fit
    // the screen (phones); the world screen shows them beside the menu.
    bool compact() const { return mCompact; }
    // Copies of the raid / conquer buttons for beside the menu (compact).
    eButton* raidButton() const { return mRaidButton2; }
    eButton* conquerButton() const { return mConquerButton2; }
private:
    eButton* mRaidButton2 = nullptr;
    eButton* mConquerButton2 = nullptr;
    std::function<void(bool)> mPrevNext;
    bool mCompact = false;
    eWorldBoard* mBoard = nullptr;

    eLabel* mTextLabel = nullptr;

    eLabel* mRelationshipLabel = nullptr;
    eLabel* mNameLabel = nullptr;
    eLabel* mLeaderLabel = nullptr;

    eLabel* mAttitudeLabel = nullptr;

    eWorldGoodsWidget* mGoodsWidget = nullptr;
    eWorldTributeWidget* mTributeWidget = nullptr;

    eButton* mRequestButton = nullptr;
    eButton* mFulfillButton = nullptr;
    eButton* mGiftButton = nullptr;
    eButton* mRaidButton = nullptr;
    eButton* mConquerButton = nullptr;

    stdsptr<eWorldCity> mCity;

    bool mShowText = true;
};

#endif // EWORLDMENU_H
