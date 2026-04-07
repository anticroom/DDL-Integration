#include <Geode/utils/web.hpp>

struct IDListDemon {
    int id = 0;
    int position = 0;
    std::string name;
    std::string author;
    std::string uid;

    IDListDemon(int id, int position, std::string name, std::string author, std::string uid) 
        : id(id), position(position), name(name), author(author), uid(uid) {}

    bool operator==(const IDListDemon& other) const {
        return id == other.id && position == other.position;
    }
};

struct IDDemonPack {
    std::string name;
    std::string color;
    std::vector<int> levels;
    double points = 0.0;

    IDDemonPack(std::string name, std::string color, std::vector<int> levels, double points)
        : name(name), color(color), levels(levels), points(points) {}
};

namespace DDLIntegration {
    extern std::vector<IDListDemon> ddl;
    extern std::vector<IDDemonPack> ddlPacks;
    extern std::vector<IDListDemon> dcl;
    extern std::vector<IDDemonPack> dclPacks;
    extern bool ddlLoaded;
    extern bool dclLoaded;

    void loadDDL(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadDDLPacks(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadDCL(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadDCLPacks(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    double calculateScore(int rank);
}