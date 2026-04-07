#include "../DDLIntegration.hpp"
#include <Geode/binding/SetIDPopupDelegate.hpp>
#include <Geode/ui/TextInput.hpp>

class IDPackLayer : public cocos2d::CCLayer, SetIDPopupDelegate {
public:
    static IDPackLayer* create();
    static cocos2d::CCScene* scene();

    void page(int);
    void keyDown(cocos2d::enumKeyCodes, double) override;
    void keyBackClicked() override;
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
    CCMenuItemSpriteExtra* m_leftButton;
    CCMenuItemSpriteExtra* m_rightButton;
    CCMenuItemSpriteExtra* m_pageButton;
    CCMenuItemSpriteExtra* m_randomButton;
    CCMenuItemSpriteExtra* m_firstButton;
    CCMenuItemSpriteExtra* m_lastButton;
    CCMenuItemSpriteExtra* m_searchButton;
    CCMenuItemSpriteExtra* m_starToggle;
    CCMenuItemSpriteExtra* m_moonToggle;
    int m_page = 0;
    std::string m_query;
    std::vector<IDDemonPack> m_fullSearchResults;
    geode::CopyableFunction<void(int)> m_ddlFailure;
    geode::CopyableFunction<void(int)> m_dclFailure;
    bool dclEnabled = false;

    bool init() override;
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
    void setIDPopupClosed(SetIDPopup*, int) override;
};