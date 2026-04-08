#pragma once
#include <cocos2d.h>
#include <string>
#include <vector>
#include "../DDLIntegration.hpp"

class IDLeaderboardCell : public cocos2d::CCLayer {
public:
    static IDLeaderboardCell* create(const IDLeaderboardEntry& entry);

protected:
    IDLeaderboardEntry m_entry;

    bool init(const IDLeaderboardEntry& entry);
    void onInfo(cocos2d::CCObject*);
};