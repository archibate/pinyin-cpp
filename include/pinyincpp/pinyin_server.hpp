#pragma once

#include <pinyincpp/pinyin.hpp>
#include <pinyincpp/pinyin_words.hpp>
#include <pinyincpp/pinyin_input.hpp>
#include <pinyincpp/pinyin_englify.hpp>
#include <pinyincpp/ctype.hpp>
#include <fstream>
#include <string>

namespace pinyincpp {

struct PinyinServer {
    PinyinDB db;
    PinyinWordsDB wd{db};
    PinyinEnglifyDB ed{db};
    PinyinInput im;

    void onLoadSample(std::string const &in, double factor = 1.0) {
        // todo: remove english chunks in it...
        im.addSampleString(in, factor);
    }

    void onDefineWords(std::string const &in, double factor = 1.0) {
        // format e.g:
        // ni hao=你好
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
        std::size_t eatBytes;
    };

    struct InputResult {
        std::vector<Candidate> candidates;
        std::string fixedPrefix;
        std::size_t fixedEatBytes;
    };

    InputResult onInput(std::string const &prefix, std::string const &in, std::size_t num = 100) {
        InputResult result{};
        std::size_t inpos, pos;
        auto s = ed.enggyToString(in, &inpos, &pos);
        if (inpos == std::string::npos) {
            inpos = in.size();
        }
        std::string past = s.substr(0, pos);
        std::string rest = s.substr(pos);
        auto prefixUtf32 = utfCto32(prefix + past);
        if (rest.empty()) {
            if (!prefixUtf32.empty() && isChineseCharacter(prefixUtf32.back())) {
                result.fixedPrefix = past;
                result.fixedEatBytes = inpos;
                for (auto c: im.suggestCharCandidates(prefixUtf32, num)) {
                    auto enggy = ed.charToEnggy(c.character);
                    result.candidates.push_back({utf32toC(c.character), enggy, c.score});
                }
            }
        } else {
            std::vector<std::size_t> indices;
            auto restUtf32 = utfCto32(rest);
            auto pids = db.pinyinSplit(restUtf32, false, U'0', &indices);
            if (!pids.empty()) {
                int englishTendency = 0;
                auto [numCnPrefix, numEnPrefix] = chineseEnglishFraction(prefixUtf32);
                if (numCnPrefix * 2 > numEnPrefix * 3) {
                    englishTendency -= 1;
                } else if (numEnPrefix * 2 > numCnPrefix * 3) {
                    englishTendency += 1;
                }
                if (numCnPrefix > numEnPrefix * 3) {
                    englishTendency -= 2;
                } else if (numEnPrefix > numCnPrefix * 4) {
                    englishTendency += 1;
                }
                if (numEnPrefix > numCnPrefix * 8) {
                    englishTendency += 1;
                }
                int pinyinCount = 0, specialCount = 0;
                bool seemsPinyin = true;
                /* auto foundSep = restUtf32.find(U'0'); */
                /* if (foundSep == 0 || (!restUtf32.empty() && foundSep == restUtf32.size() - 1)) { */
                    for (auto const &pid: pids) {
                        if (isSpecialPid(pid)) {
                            char32_t c = extractSpecialPid(pid);
                            if (c != U' ' && c != U'_') {
                                if (isEnglishCharacter(c)) {
                                    specialCount += 34;
                                } else if (isEnglishPuncation(c)) {
                                    specialCount += 4;
                                } else {
                                    specialCount += 1;
                                }
                            }
                        } else {
                            auto pinName = db.pinyinName(pid);
                            auto pinLen = pinName.size();
                            if (pinLen >= 2) {
                                if (std::string("wxyzgqjy").find(pinName.front()) != std::string::npos) {
                                    pinLen += 1;
                                }
                                if (pinLen == 2 && std::string("iuv").find(pinName.back()) != std::string::npos) {
                                    pinyinCount += 10;
                                }
                                if (pinLen == 2 && std::string("ao").find(pinName.back()) != std::string::npos) {
                                    pinyinCount += 8;
                                }
                                if (pinLen >= 3 && std::string("zcs").find(pinName.front()) != std::string::npos && pinName[1] == 'h') {
                                    pinLen += 1;
                                    pinyinCount += 6;
                                }
                            }
                            pinyinCount += pinLen * 10;
                            if (pinLen >= 5) {
                                pinyinCount += 12;
                            } else if (pinLen >= 4) {
                                pinyinCount += 7;
                            }
                            if (pinLen <= 1) {
                                specialCount += 8;
                            } else if (pinLen <= 2) {
                                specialCount += 5;
                            } else if (pinLen <= 3) {
                                specialCount += 2;
                            }
                        }
                    }
                    /* printf("%d, %d\n", specialCount, pinyinCount); */
                    seemsPinyin = pinyinCount != 0 && specialCount * (5 + englishTendency) < pinyinCount + 13;
                /* } */
                if (seemsPinyin) {
                    result.fixedPrefix = past;
                    result.fixedEatBytes = inpos;
                    std::vector<std::size_t> byteIndices;
                    byteIndices.reserve(indices.size());
                    for (std::size_t i: indices) {
                        byteIndices.push_back(utf32toC(restUtf32.substr(0, i)).size());
                    }
                    if (pids.size() == 1 && num > result.candidates.size()) {
                        for (auto c: im.pinyinCharCandidates(db, prefixUtf32, pids.front(), num - result.candidates.size())) {
                            auto enggy = ed.charToEnggy(c.character, pids.front());
                            result.candidates.push_back({utf32toC(c.character), enggy, c.score, byteIndices[0]});
                        }
                    }
                    if (pids.size() > 1 && num > result.candidates.size()) {
                        for (auto c: im.pinyinWordCandidates(db, wd, prefixUtf32, pids, num - result.candidates.size())) {
                            std::string enggy;
                            for (std::size_t i = 0; i < c.word.size(); i++) {
                                enggy += ed.charToEnggy(c.word[i], i < c.pinyin.size() ? c.pinyin[i] : makeSpecialPid(c.word[i]));
                            }
                            result.candidates.push_back({utf32toC(c.word), enggy, c.score, byteIndices[std::min(pids.size() - 1, byteIndices.size() - 1)]});
                        }
                        if (result.candidates.size() == 0 && num != 0 && pids.size() > 2) {
                            pids.pop_back();
                            for (auto c: im.pinyinWordCandidates(db, wd, prefixUtf32, pids, num - result.candidates.size())) {
                                std::string enggy;
                                for (std::size_t i = 0; i < c.word.size(); i++) {
                                    enggy += ed.charToEnggy(c.word[i], i < c.pinyin.size() ? c.pinyin[i] : makeSpecialPid(c.word[i]));
                                }
                                result.candidates.push_back({utf32toC(c.word), enggy, c.score, byteIndices[std::min(pids.size() - 1, byteIndices.size() - 1)]});
                            }
                        }
                        if (num > result.candidates.size()) {
                            for (auto c: im.pinyinCharCandidates(db, prefixUtf32, pids.front(), num - result.candidates.size())) {
                                auto enggy = ed.charToEnggy(c.character, pids.front());
                                result.candidates.push_back({utf32toC(c.character), enggy, c.score, byteIndices[0]});
                            }
                        }
                    } }
            }
        }
        return result;
    }
};

}
