#include <pinyincpp/pinyin_englify.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using namespace pinyincpp;

int main() {
    PinyinDB db;
    PinyinWordsDB wd(db);
    PinyinEnglifyDB ed(db);
    PinyinInput im;
    {
        std::ifstream fin("/tmp/extract.txt");
        im.addSampleString(std::string(std::istreambuf_iterator<char>{fin}, std::istreambuf_iterator<char>{}));
    }
    std::string in = "wo1shi1xiao1pengyou";
    std::size_t pos;
    auto s = ed.enggyToString(in, &pos);
    std::cout << "- input: " << in << std::endl;
    std::cout << "- output: " << s << std::endl;
    std::cout << "- pos: " << pos << std::endl;
    std::string past = s.substr(0, pos);
    std::string rest = s.substr(pos);
    auto pids = db.pinyinSplit(utfCto32(rest));
    for (auto c: im.pinyinWordCandidates(db, wd, utfCto32(past), pids)) {
        std::string enggy;
        for (std::size_t i = 0; i < c.word.size(); i++) {
            enggy += ed.charToEnggy(c.word[i], i < c.pinyin.size() ? c.pinyin[i] : makeSpecialPid(c.word[i]));
        }
        std::cout << utf32toC(c.word) << '[' << enggy << ']' << c.score << std::endl;
    }
    return 0;
}
