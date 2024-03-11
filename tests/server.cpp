#include <pinyincpp/pinyin_server.hpp>
#include <fstream>
#include <iostream>

using namespace pinyincpp;

int main() {
    PinyinServer ps;
    {
        std::ifstream fin("/tmp/extract.txt");
        ps.onLoadSample(std::string(std::istreambuf_iterator<char>{fin}, std::istreambuf_iterator<char>{}), 1.0);
    }
    {
        std::ifstream fin("/tmp/words.txt");
        ps.onDefineWords(std::string(std::istreambuf_iterator<char>{fin}, std::istreambuf_iterator<char>{}), 1.0);
    }
    /* std::cout << ps.db.pinyinConcat(ps.db.pinyinSplit(U"laoshi")) << '\n'; */
    auto res = ps.onInput("我是可爱的小彭", "laoshi", 10);
    std::cout << '[' << res.fixedPrefix << ']' << res.fixedEatBytes << '\n';
    for (auto c: res.candidates) {
        std::cout << '[' << c.text << '|' << c.enggy << ']' << c.eatBytes << ' ' << c.score << '\n';
    }
    /* std::cout << ps.db.charLogFrequency(U'啊') << '\n'; */
    /* std::cout << ps.db.charLogFrequency(U'阿') << '\n'; */
    /* for (auto pids: ps.db.stringToPinyin(U"的确")) { */
    /*     std::cout << ps.db.pinyinConcat(pids) << '\n'; */
    /* } */
    return 0;
}
