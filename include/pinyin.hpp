#pragma once

#include <cmath>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "utf8.hpp"

namespace pinyincpp {

extern const char chars_csv[];
extern const unsigned long long chars_csv_size;

using Pid = std::int32_t;

struct PidTone {
    Pid pid;
    std::uint8_t tone;
};

using PidSet = std::vector<Pid>;
using PidToneSet = std::vector<PidTone>;

struct PinyinDB {
    struct CharInfo {
        char32_t character;
        double logFrequency;
    };

    struct CharData : CharInfo {
        PidToneSet pinyin;
    };

    struct PinyinInfo {
        double logFrequency;
        PidSet pinyin;
    };

    std::vector<CharData> charData;
    std::vector<std::string> pinyinData;

    PinyinDB() {
        {
            std::istringstream charsCsvIn(std::string(chars_csv, chars_csv_size));
            std::string line;
            while (std::getline(charsCsvIn, line)) {
                std::string tmp;
                std::istringstream lineIn(line);
                std::getline(lineIn, tmp, ',');
                char32_t character = std::stoul(tmp);
                std::getline(lineIn, tmp, ',');
                std::uint32_t frequency = std::stoul(tmp);
                PidToneSet pinyinToned;
                std::unordered_set<Pid> addedPinyin;
                while (std::getline(lineIn, tmp, '/')) {
                    std::uint8_t tone = 0;
                    if ('0' <= tmp.back() && tmp.back() <= '9') {
                        tone = tmp.back() - '0';
                        tmp.pop_back();
                    }
                    auto [it, success] = lookupPinyinToPid.try_emplace(tmp, pinyinData.size());
                    Pid pid = it->second;
                    if (addedPinyin.insert(pid).second) {
                        pinyinToned.push_back({pid, tone});
                    }
                    if (success) {
                        pinyinData.push_back(tmp);
                    }
                }
                auto logFrequency = std::log(frequency + 2);
                charData.push_back({character, logFrequency, std::move(pinyinToned)});
            }
        }
        for (const auto &c : charData) {
            for (auto const &p : c.pinyin) {
                lookupPinyinToChar[p.pid].push_back(static_cast<CharInfo const &>(c));
            }
        }
        for (const auto &c : charData) {
            auto &pinInfo = lookupCharToPinyin[c.character];
            pinInfo.logFrequency = c.logFrequency;
            std::unordered_set<Pid> addedPinyin;
            for (auto const &p: c.pinyin) {
                if (addedPinyin.insert(p.pid).second) {
                    pinInfo.pinyin.push_back(p.pid);
                }
            }
        }
        for (const auto &[p, pid] : lookupPinyinToPid) {
            for (std::size_t i = 1; i <= p.size(); ++i) {
                lookupPinyinPrefixSet.insert(p.substr(0, i));
            }
        }
    }

    std::unordered_map<std::string, Pid> lookupPinyinToPid;
    std::unordered_set<std::string> lookupPinyinPrefixSet;
    std::unordered_map<Pid, std::vector<CharInfo>> lookupPinyinToChar;
    std::unordered_map<char32_t, PinyinInfo> lookupCharToPinyin;

    Pid pinyinId(std::string const &pinyin) {
        auto it = lookupPinyinToPid.find(pinyin);
        if (it == lookupPinyinToPid.end()) [[unlikely]] {
            return (Pid)-1;
        } else {
            return it->second;
        }
    }

    std::string pinyinName(Pid pinyin) {
        if (pinyin < 0) {
            return utf32toC((char32_t)-pinyin);
        }
        if (pinyin >= pinyinData.size()) [[unlikely]] {
            return {};
        }
        return pinyinData[pinyin];
    }

    std::vector<CharInfo> pinyinToChar(Pid p) {
        auto it = lookupPinyinToChar.find(p);
        if (it == lookupPinyinToChar.end()) {
            return {};
        } else {
            return it->second;
        }
    }

    PidSet charToPinyin(char32_t character) {
        auto it = lookupCharToPinyin.find(character);
        if (it == lookupCharToPinyin.end()) {
            return {};
        } else {
            return it->second.pinyin;
        }
    }

    double charLogFrequency(char32_t character) {
        auto it = lookupCharToPinyin.find(character);
        if (it == lookupCharToPinyin.end()) {
            return 0;
        } else {
            return it->second.logFrequency;
        }
    }

    std::vector<PidSet> stringToPinyin(std::u32string const &str) {
        std::vector<PidSet> result;
        result.reserve(str.size());
        for (char32_t c : str) {
            if ('A' <= c && c <= 'Z') {
                c += 'a' - 'A';
            } else if (c == 0) {
                c = ' ';
            }
            result.emplace_back(charToPinyin(c)).push_back(-(Pid)c);
        }
        return result;
    }

    std::vector<std::vector<std::string>> simpleStringToPinyin(std::string const &str) {
        std::vector<std::vector<std::string>> result;
        auto str32 = utfCto32(str);
        result.reserve(str32.size());
        for (char32_t c : str32) {
            auto &back = result.emplace_back();
            auto pidSet = charToPinyin(c);
            if (pidSet.empty()) {
                back.push_back(utf32toC(c));
            } else {
                back.reserve(pidSet.size());
                for (auto p: pidSet) {
                    back.push_back(pinyinName(p));
                }
            }
        }
        return result;
    }

    std::vector<Pid> pinyinSplit(std::u32string const &pinyin, bool ignoreCase = false, char32_t sepChar = U'\'') {
        std::vector<Pid> result;
        std::string token;
        bool status = false;
        for (std::size_t i = 0; i < pinyin.size(); ++i) {
            auto c = pinyin[i];
            token.push_back(c);
            auto it = lookupPinyinPrefixSet.find(token);
            if (it != lookupPinyinPrefixSet.end()) {
                if (!status) {
                    status = true;
                }
            } else {
                if (status) {
                    token.pop_back();
                    auto it = lookupPinyinToPid.find(token);
                    if (it != lookupPinyinToPid.end()) {
                        result.push_back(it->second);
                    }
                    status = false;
                    --i;
                } else {
                    if (c && c != sepChar) {
                        if (ignoreCase) {
                            if ('A' <= c && c <= 'Z') {
                                c += 'a' - 'A';
                            }
                        }
                        result.push_back(-(Pid)c);
                    }
                }
                token.clear();
            }
        }
        if (status != 0) {
            auto it = lookupPinyinToPid.find(token);
            if (it != lookupPinyinToPid.end()) {
                result.push_back(it->second);
            }
            status = 0;
        }
        return result;
    }

    std::vector<std::string> simplePinyinSplit(std::string const &pinyin, bool ignoreCase = false, char32_t sepChar = U'\'') {
        auto pinyin32 = utfCto32(pinyin);
        auto pids = pinyinSplit(pinyin32, ignoreCase, sepChar);
        std::vector<std::string> result;
        result.reserve(pids.size());
        for (auto p : pids) {
            result.push_back(pinyinName(p));
        }
        return result;
    }
};

}
