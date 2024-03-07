#include "pinyin.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using namespace pinyincpp;

int main() {
    PinyinDB db;
    {
        std::ifstream fin("/tmp/extract.txt");
        db.setSampleString(std::string(std::istreambuf_iterator<char>{fin}, std::istreambuf_iterator<char>{}));
    }
    /* for (auto c: db.pinyinCandidates(U"迭代", db.pinyinSplit(U"ma").front())) { */
    /*     std::cout << utf32toC(c.word) << ' ' << c.score << std::endl; */
    /* } */
    for (auto c: db.suggestCandidates(U"迭代")) {
        std::cout << (c.word == '\n' ? "\\n" : utf32toC(c.word)) << ' ' << c.score << std::endl;
    }
    return 0;
}
