#include "pinyin.hpp"
#include "pinyin_words.hpp"
#include "pinyin_input.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using namespace pinyincpp;

int main() {
    PinyinInput im;
    PinyinDB db;
    PinyinWordsDB wd(db);
    {
        std::ifstream fin("/tmp/extract.txt");
        im.addSampleString(std::string(std::istreambuf_iterator<char>{fin}, std::istreambuf_iterator<char>{}));
    }
    for (auto c: im.pinyinWordCandidates(db, wd, U"我们小", db.pinyinSplit(U"peng"))) {
        std::cout << utf32toC(c.word) << ' ' << c.score << std::endl;
    }
    return 0;
}
