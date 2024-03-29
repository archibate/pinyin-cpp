#pragma once

#include <cmath>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <pinyincpp/resources.hpp>
#include <pinyincpp/debug.hpp>
#include <pinyincpp/utf8.hpp>
#include <pinyincpp/bytes_reader.hpp>
#include <pinyincpp/resources.hpp>
#include <pinyincpp/inline_vector.hpp>

namespace pinyincpp {

using Pid = std::int32_t;

inline bool isSpecialPid(Pid pid) {
    return pid < 0;
}

inline char32_t extractSpecialPid(Pid pid) {
    return pid < 0 ? -1 - pid : 0;
}

inline Pid makeSpecialPid(char32_t c) {
    return -1 - c;
}

struct PidTone {
    Pid pid: 13;
    std::uint8_t tone: 3;
    PidTone(Pid pid, std::uint8_t tone) noexcept : pid(pid), tone(tone) {}
    auto tuplify() const { return std::tie(pid, tone); }
};

using PidSet = InlineVector<Pid, 2>;
using PidToneSet = InlineVector<PidTone, 4>;

struct PinyinDB {
    struct CharInfo {
        char32_t character;
        float logFrequency;
        auto tuplify() const { return std::tie(character, logFrequency); }
    };

    struct PinyinInfo {
        float logFrequency;
        PidToneSet pinyin;
        auto tuplify() const { return std::tie(logFrequency, pinyin); }
    };

private:
    struct CharData : CharInfo {
        PidToneSet pinyin;
        auto tuplify() const { return std::tuple_cat(CharInfo::tuplify(), std::tie(pinyin)); }
    };

    std::vector<CharData> charData;
    std::vector<std::string> pinyinData;
    std::unordered_map<std::string, Pid> lookupPinyinToPid;
    std::unordered_set<std::string> lookupPinyinPrefixSet;
    std::unordered_map<Pid, std::vector<CharInfo>> lookupPinyinToChar;
    std::unordered_map<char32_t, PinyinInfo> lookupCharToPinyin;

public:
    PinyinDB() {
        {
            BytesReader f = CMakeResource("data/pinyin.bin").view();
            auto nPids = f.read32();
            pinyinData.reserve(nPids);
            for (Pid pid = 0; pid < nPids; pid++) {
                auto pinyin = f.reads(6);
                pinyin.resize(std::strlen(pinyin.c_str()));
                lookupPinyinToPid.emplace(pinyin, pid);
                pinyinData.push_back(std::move(pinyin));
            }
            auto nChars = f.read32();
            charData.reserve(nChars);
            for (std::size_t i = 0; i < nChars; ++i) {
                char32_t c = f.read24();
                double logProb = (double)f.read16() / 4096;
                auto nPidTones = f.read8();
                PidToneSet pidToneSet;
                pidToneSet.reserve(nPidTones);
                for (std::size_t j = 0; j < nPidTones; ++j) {
                    std::uint16_t pidTone = f.read16();
                    Pid pid = pidTone >> 3;
                    std::uint8_t tone = pidTone & 7;
                    pidToneSet.push_back({pid, tone});
                }
                charData.push_back({
                    c, static_cast<float>(logProb),
                    std::move(pidToneSet),
                });
            }
        }
        for (const auto &c : charData) {
            std::unordered_set<Pid> addedPinyin;
            for (auto const &p : c.pinyin) {
                if (addedPinyin.insert(p.pid).second) {
                    lookupPinyinToChar[p.pid].push_back(static_cast<CharInfo const &>(c));
                }
            }
            auto &pinInfo = lookupCharToPinyin[c.character];
            pinInfo.logFrequency = c.logFrequency;
            for (auto const &p: c.pinyin) {
                pinInfo.pinyin.push_back(p);
            }
        }
        for (const auto &[p, pid] : lookupPinyinToPid) {
            for (std::size_t i = 1; i <= p.size(); ++i) {
                lookupPinyinPrefixSet.insert(p.substr(0, i));
            }
        }
    }

    Pid pinyinId(std::string const &pinyin) {
        auto it = lookupPinyinToPid.find(pinyin);
        if (it == lookupPinyinToPid.end()) [[unlikely]] {
            return makeSpecialPid(0);
        } else {
            return it->second;
        }
    }

    Pid pinyinPidLimit() const noexcept {
        return pinyinData.size();
    }

    std::string pinyinName(Pid pinyin) {
        if (isSpecialPid(pinyin)) {
            return utf32toC(extractSpecialPid(pinyin));
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

    PidToneSet *charToPinyinToned(char32_t character) {
        auto it = lookupCharToPinyin.find(character);
        if (it == lookupCharToPinyin.end()) {
            return nullptr;
        } else {
            return &it->second.pinyin;
        }
    }

    PidSet charToPinyin(char32_t character) {
        auto it = lookupCharToPinyin.find(character);
        if (it == lookupCharToPinyin.end()) {
            return {};
        } else {
            PidSet pidSet;
            std::unordered_set<Pid> addedPinyin;
            for (auto p: it->second.pinyin) {
                if (addedPinyin.insert(p.pid).second) {
                    pidSet.push_back(p.pid);
                }
            }
            return pidSet;
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
    std::vector<PidToneSet> stringToPinyinToned(std::u32string const &str, bool ignoreCase = false) {
        std::vector<PidToneSet> result;
        result.reserve(str.size());
        for (char32_t c : str) {
            if (ignoreCase) {
                if ('A' <= c && c <= 'Z') {
                    c += 'a' - 'A';
                }
            }
            auto p = charToPinyinToned(c);
            auto &e = p ? result.emplace_back(*p) : result.emplace_back();
            e.push_back({makeSpecialPid(c), 0});
        }
        return result;
    }

    std::vector<PidSet> stringToPinyin(std::u32string const &str, bool ignoreCase = false) {
        std::vector<PidSet> result;
        result.reserve(str.size());
        for (char32_t c : str) {
            if (ignoreCase) {
                if ('A' <= c && c <= 'Z') {
                    c += 'a' - 'A';
                }
            }
            result.emplace_back(charToPinyin(c)).push_back(makeSpecialPid(c));
        }
        return result;
    }

    std::vector<std::vector<std::string>> simpleStringToPinyin(std::string const &str) {
        std::vector<std::vector<std::string>> result;
        auto str32 = utfCto32(str);
        result.reserve(str32.size());
        for (char32_t c : str32) {
            auto &back = result.emplace_back();
            auto pidToneSet = charToPinyinToned(c);
            if (!pidToneSet || pidToneSet->empty()) {
                back.push_back(utf32toC(c));
            } else {
                back.reserve(pidToneSet->size());
                for (auto p: *pidToneSet) {
                    back.push_back(pinyinName(p.pid));
                }
            }
        }
        return result;
    }

    std::vector<Pid> pinyinSplit(std::u32string const &pinyin, bool ignoreCase = false,
                                 char32_t sepChar = U' ', std::vector<std::size_t> *indices = nullptr) {
        std::vector<Pid> result;
        std::string token;
        bool status = false;
        for (std::size_t i = 0; i < pinyin.size(); ++i) {
            auto c = pinyin[i];
            if ('a' <= c && c <= 'z') {
                token.push_back(c);
                auto it = lookupPinyinPrefixSet.find(token);
                if (it != lookupPinyinPrefixSet.end()) {
                    if (!status) {
                        status = true;
                    } else {
                        auto it = lookupPinyinPrefixSet.find(token);
                        if (it == lookupPinyinPrefixSet.end()) {
                            std::size_t off = token.size();
                            for (char32_t c : token) {
                                result.push_back(makeSpecialPid(c));
                                --off;
                                if (indices) indices->push_back(pinyin.size() - off);
                            }
                        }
                    }
                } else {
                    if (status) {
                        token.pop_back();
                        auto it = lookupPinyinToPid.find(token);
                        if (it != lookupPinyinToPid.end()) {
                            if (token.size() >= 2) {
                                auto b = token.back();
                                if (b == 'n' || b == 'g') {
                                    if (std::string("aeiouv").find((char)c) != std::string::npos) {
                                        char tmpToken[] = {b, (char)c, '\0'};
                                        auto tmpIt = lookupPinyinToPid.find(tmpToken);
                                        if (tmpIt != lookupPinyinToPid.end()) {
                                            token.pop_back();
                                            tmpIt = lookupPinyinToPid.find(token);
                                            if (tmpIt != lookupPinyinToPid.end()) {
                                                it = tmpIt;
                                                --i;
                                            } else {
                                                token.push_back(b);
                                            }
                                        }
                                    }
                                }
                            }
                            result.push_back(it->second);
                            if (indices) indices->push_back(i);
                        } else {
                            std::size_t off = token.size();
                            for (char32_t c : token) {
                                result.push_back(makeSpecialPid(c));
                                --off;
                                if (indices) indices->push_back(i - off);
                            }
                        }
                        status = false;
                        --i;
                    } else {
                        if (c != sepChar) {
                            if (ignoreCase) {
                                if ('A' <= c && c <= 'Z') {
                                    c += 'a' - 'A';
                                }
                            }
                            result.push_back(makeSpecialPid(c));
                            if (indices) indices->push_back(i + 1);
                        }
                    }
                    token.clear();
                }
            } else {
                if (status) {
                    auto it = lookupPinyinToPid.find(token);
                    if (it != lookupPinyinToPid.end()) {
                        result.push_back(it->second);
                        if (indices) indices->push_back(i);
                        token.clear();
                    }
                    std::size_t off = token.size();
                    for (char32_t c : token) {
                        result.push_back(makeSpecialPid(c));
                        --off;
                        if (indices) indices->push_back(i - off);
                    }
                    status = false;
                }
                if (c != sepChar) {
                    if (ignoreCase) {
                        if ('A' <= c && c <= 'Z') {
                            c += 'a' - 'A';
                        }
                    }
                    result.push_back(makeSpecialPid(c));
                    if (indices) indices->push_back(i + 1);
                }
            }
        }
        if (status != 0) {
            auto it = lookupPinyinToPid.find(token);
            if (it != lookupPinyinToPid.end()) {
                result.push_back(it->second);
                if (indices) indices->push_back(pinyin.size());
            } else {
                std::size_t off = token.size();
                for (char32_t c : token) {
                    if (c != sepChar) {
                        if (ignoreCase) {
                            if ('A' <= c && c <= 'Z') {
                                c += 'a' - 'A';
                            }
                        }
                        result.push_back(makeSpecialPid(c));
                    }
                    --off;
                    if (indices) indices->push_back(pinyin.size() - off);
                }
            }
            status = 0;
        }
        return result;
    }

    std::vector<std::string> simplePinyinSplit(std::string const &pinyin, bool ignoreCase = false, char32_t sepChar = U' ') {
        auto pinyin32 = utfCto32(pinyin);
        auto pids = pinyinSplit(pinyin32, ignoreCase, sepChar);
        std::vector<std::string> result;
        result.reserve(pids.size());
        for (auto p : pids) {
            result.push_back(pinyinName(p));
        }
        return result;
    }

    std::string pinyinConcat(std::vector<Pid> const &pids, char32_t sepChar = U' ') {
        std::string result;
        bool first = true;
        for (auto p: pids) {
            if (first) {
                first = false;
            } else {
                result.push_back(sepChar);
            }
            result.append(pinyinName(p));
        }
        return result;
    }
};

}
