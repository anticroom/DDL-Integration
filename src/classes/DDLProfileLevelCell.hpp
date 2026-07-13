#pragma once
#include <cocos2d.h>
#include "../DDLIntegration.hpp"

class DDLProfileLevelCell : public cocos2d::CCLayer {
public:
    static DDLProfileLevelCell* create(const DDLLevelRecord& record, bool isVerification, bool isDCL, int index);

protected:
    bool init(const DDLLevelRecord& record, bool isVerification, bool isDCL, int index);
};