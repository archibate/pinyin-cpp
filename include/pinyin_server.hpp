#pragma once

#include "pinyin.hpp"
#include "pinyin_words.hpp"
#include "pinyin_input.hpp"
#include "pinyin_englify.hpp"
#include "ctype.hpp"
#include <fstream>
#include <string>

namespace pinyincpp {

struct PinyinServer {
    PinyinDB db;
    PinyinWordsDB wd{db};
    PinyinEnglifyDB ed{db};
    PinyinInput im;

    void onLoadSample(std::string const &in, double factor = 1.0) {
        im.addSampleString(in, factor);
    }

    void onDefineWords(std::string const &in, double factor = 1.0) {
        std::istringstream iss(in);
        std::string line;
        std::vector<std::pair<std::string, std::string>> pinyinAndWords;
        while (std::getline(iss, line)) {
            std::istringstream ss(line);
            std::string pinyin;
            std::string words;
            if (std::getline(ss, pinyin, '=') && std::getline(ss, words)) {
                pinyinAndWords.push_back({pinyin, words});
            }
        }
        wd.addCustomWords(db, pinyinAndWords, factor);
    }

    struct Candidate {
        std::string text;
        std::string enggy;
        double score;
    };

    struct InputResult {
        std::vector<Candidate> candidates;
        std::string fixedPrefix;
    };

    InputResult onInput(std::string const &prefix, std::string const &in, std::size_t num = 100) {
        InputResult result;
        std::size_t pos;
        auto s = ed.enggyToString(in, &pos);
        std::string past = s.substr(0, pos);
        std::string rest = s.substr(pos);
        result.fixedPrefix = past;
        auto prefixUtf32 = utfCto32(prefix + past);
        if (rest.empty()) {
            for (auto c: im.suggestCharCandidates(prefixUtf32, num)) {
                auto enggy = ed.charToEnggy(c.character);
                result.candidates.push_back({utf32toC(c.character), enggy, c.score});
            }
        } else {
            auto pids = db.pinyinSplit(utfCto32(rest), false, U'0');
            if (pids.size() == 1 && num > result.candidates.size()) {
                for (auto c: im.pinyinCharCandidates(db, prefixUtf32, pids.front(), num - result.candidates.size())) {
                    auto enggy = ed.charToEnggy(c.character, pids.front());
                    result.candidates.push_back({utf32toC(c.character), enggy, c.score});
                }
            }
            if (pids.size() > 1 && num > result.candidates.size()) {
                for (auto c: im.pinyinWordCandidates(db, wd, prefixUtf32, pids, num - result.candidates.size())) {
                    std::string enggy;
                    for (std::size_t i = 0; i < c.word.size(); i++) {
                        enggy += ed.charToEnggy(c.word[i], i < c.pinyin.size() ? c.pinyin[i] : makeSpecialPid(c.word[i]));
                    }
                    result.candidates.push_back({utf32toC(c.word), enggy, c.score});
                }
                if (num > result.candidates.size()) {
                    for (auto c: im.pinyinCharCandidates(db, prefixUtf32, pids.front(), num - result.candidates.size())) {
                        auto enggy = ed.charToEnggy(c.character, pids.front());
                        result.candidates.push_back({utf32toC(c.character), enggy, c.score});
                    }
                }
            }
        }
        return result;
    }
};

}
