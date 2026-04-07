#pragma once
#include <cocos2d.h>
#include <span>
#include <vector>
#include <string>
#include <string_view>

class IDPackCell : public cocos2d::CCLayer {
public:
    static IDPackCell* create(std::string_view, double, std::span<const int>, std::string_view, std::string_view);

protected:
    std::vector<int> m_levels;
    std::string m_packName;

    bool init(std::string_view, double, std::span<const int>, std::string_view, std::string_view);
    void onClick(cocos2d::CCObject*);
};