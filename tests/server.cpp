#include "pinyin_server.hpp"
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
    auto res = ps.onInput("我是可爱的", "xiao1da1peng2laoshi", 100);
    std::cout << '[' << res.fixedPrefix << ']' << res.fixedEatBytes << '\n';
    for (auto c: res.candidates) {
        std::cout << '[' << c.text << '|' << c.enggy << ']' << c.eatBytes << ' ' << c.score << '\n';
    }
    return 0;
}
