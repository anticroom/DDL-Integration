#include <Geode/binding/LevelManagerDelegate.hpp>
#include <Geode/binding/SetIDPopupDelegate.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/web.hpp>
#include <set>
#include "IDPackCell.hpp"
#include "../DDLIntegration.hpp"

class IDListLayer : public cocos2d::CCLayer, SetIDPopupDelegate, LevelManagerDelegate {
public:
    static IDListLayer* create();
    static cocos2d::CCScene* scene();

    void page(int);
    void keyDown(cocos2d::enumKeyCodes, double) override;
    void keyBackClicked() override;

    ~IDListLayer() override;
protected:
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_ddlListener;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_dclListener;
    GJListLayer* m_list;
    cocos2d::CCLabelBMFont* m_listLabel;
    LoadingCircle* m_loadingCircle;
    cocos2d::CCMenu* m_searchBarMenu;
    geode::TextInput* m_searchBar;
    cocos2d::CCLabelBMFont* m_countLabel;
    cocos2d::CCLabelBMFont* m_pageLabel;
    InfoAlertButton* m_infoButton;
    
    CCMenuItemSpriteExtra* m_leftButton;
    CCMenuItemSpriteExtra* m_rightButton;
    CCMenuItemSpriteExtra* m_pageButton;
    CCMenuItemSpriteExtra* m_randomButton;
    CCMenuItemSpriteExtra* m_firstButton;
    CCMenuItemSpriteExtra* m_lastButton;
    CCMenuItemSpriteExtra* m_starToggle;
    CCMenuItemSpriteExtra* m_moonToggle;
    CCMenuItemSpriteExtra* m_searchButton;
    CCMenuItemSpriteExtra* m_modeToggleBtn;

    cocos2d::CCArray* m_pageCache = nullptr;
    std::set<int> m_attemptedFetches;
    int m_page = 0;
    std::string m_query;
    
    bool m_showingPacks = false;
    std::vector<std::string> m_fullSearchResults;
    std::vector<IDDemonPack> m_fullPackResults;
    
    geode::CopyableFunction<void(int)> m_ddlFailure;
    geode::CopyableFunction<void(int)> m_dclFailure;

    bool init() override;
    void updateHeaders();
    void onModeToggle(cocos2d::CCObject*);
    void onSearch(cocos2d::CCObject*);
    void onBack(cocos2d::CCObject*);
    void onPrevPage(cocos2d::CCObject*);
    void onNextPage(cocos2d::CCObject*);
    void onRefresh(cocos2d::CCObject*);
    void onStar(cocos2d::CCObject*);
    void onMoon(cocos2d::CCObject*);
    void onPage(cocos2d::CCObject*);
    void onRandom(cocos2d::CCObject*);
    void onFirst(cocos2d::CCObject*);
    void onLast(cocos2d::CCObject*);
    void showLoading();
    void populateList(const std::string& query);
    void loadLevelsFinished(cocos2d::CCArray* levels, const char* key, int) override;
    void loadLevelsFailed(const char* key, int) override;
    void setupPageInfo(gd::string, const char*) override;
    void setIDPopupClosed(SetIDPopup*, int) override;
};