#include <pinyincpp/pinyin_match.hpp>
#include <iostream>

using namespace pinyincpp;

int main() {
    PinyinDB db;
    PinyinMatch ma;
    std::vector<std::pair<int, std::vector<std::string>>> list = {
        {114, {"妈刀", "妈妈的菜刀", "马刀", "Mom's knife"}},
        {115, {"小妈刀", "妈妈的剃须刀", "Mom's blade"}},
        {118, {"硫磺火", "牛黄火", "Brimstone"}},
        {119, {"炼金硫磺", "硫磺", "Sulfur"}},
    };
    std::string query = "Su";
    auto results = ma.simpleAliasedMatchPinyin(db, list, query);
    for (auto index : results) {
        std::cout << index << std::endl;
    }
    return 0;
}
