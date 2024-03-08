#pragma once

#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include "utf8.hpp"
#include "pinyin.hpp"
#include "trie.hpp"

namespace pinyincpp {

extern const char words_csv[];
extern const unsigned long long words_csv_size;

struct PinyinWordsDB {
    struct WordData {
        std::u32string word;
        double score;
        std::vector<Pid> pinyin;
    };

    std::vector<WordData> wordData;
    Trie<Pid, std::size_t> triePinyinToWord;

    explicit PinyinWordsDB(PinyinDB &db) {
        {
            std::istringstream wordsCsvIn(std::string(words_csv, words_csv_size));
            std::string line, tmp;
            std::vector<Pid> pinyin;
            std::vector<std::size_t> words;
            while (std::getline(wordsCsvIn, line)) {
                std::istringstream lineIn(std::move(line));
                std::getline(lineIn, tmp, '\t');
                std::istringstream pinyinIn(std::move(tmp));
                pinyin.clear();
                while (std::getline(pinyinIn, tmp, ' ')) {
                    Pid pid = db.pinyinId(tmp);
                    pinyin.push_back(pid);
                }
                std::getline(lineIn, tmp, '\t');
                std::istringstream wordsIn(std::move(tmp));
                words.clear();
                while (std::getline(wordsIn, tmp, ' ')) {
                    auto word = utfCto32(tmp);
                    double score = 0;
                    std::size_t num = 0;
                    for (char32_t c: word) {
                        auto logProb = db.charLogFrequency(c);
                        num += logProb > 0 ? 1 : 0;
                        score += logProb;
                    }
                    if (num != 0) {
                        score /= num;
                    }
                    words.push_back(wordData.size());
                    wordData.push_back({word, score, pinyin});
                }
                triePinyinToWord.batchedInsert(pinyin, std::move(words));
            }
        }
    }
};

}
