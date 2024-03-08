#include "pinyin.hpp"
#include "pinyin_input.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using namespace pinyincpp;

int main() {
    PinyinInput im;
    PinyinDB db;
    {
        std::ifstream fin("/tmp/extract.txt");
        im.addSampleString(std::string(std::istreambuf_iterator<char>{fin}, std::istreambuf_iterator<char>{}), 5.0);
    }
    for (auto c: im.pinyinWordCandidates(db, U"我们小", db.pinyinSplit(U"peng").front())) {
        std::cout << (c.word == '\n' ? "\\n" : utf32toC(c.word)) << ' ' << c.score << std::endl;
    }
    return 0;
}
