#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <pinyincpp/utf8.hpp>
#include <pinyincpp/pinyin.hpp>

namespace pinyincpp {

struct PinyinEnglifyDB {
private:
    struct PairHash {
        std::size_t operator()(const std::pair<char32_t, Pid> &pair) const {
            return std::hash<char32_t>()(pair.first) ^ std::hash<Pid>()(pair.second);
        }
    };

public:
    std::unordered_map<std::string, char32_t> lookupEnggyToChar;
    std::unordered_map<std::pair<char32_t, Pid>, std::string, PairHash> lookupCharPidToEnggy;
    std::unordered_map<char32_t, std::string> lookupCharToEnggy;

    static std::string numberEncode(std::size_t num) {
        return std::to_string(num + 1);
    }

    explicit PinyinEnglifyDB(PinyinDB &db) {
        for (Pid pid = 0; pid < db.pinyinPidLimit(); ++pid) {
            auto chars = db.pinyinToChar(pid);
            auto pinyin = db.pinyinName(pid);
            std::size_t num = 0;
            for (auto const &c : chars) {
                auto enggy = pinyin + numberEncode(num);
                lookupEnggyToChar.emplace(enggy, c.character);
                lookupCharToEnggy.emplace(c.character, enggy);
                lookupCharPidToEnggy.emplace(std::make_pair(c.character, pid), enggy);
                ++num;
            }
        }
    }

    char32_t enggyToChar(std::string const &enggy) {
        auto it = lookupEnggyToChar.find(enggy);
        if (it != lookupEnggyToChar.end()) {
            return it->second;
        }
        return 0;
    }

    std::string charToEnggy(char32_t c) {
        auto it = lookupCharToEnggy.find(c);
        if (it != lookupCharToEnggy.end()) {
            return it->second;
        }
        return utf32toC(c);
    }

    std::string charToEnggy(char32_t c, Pid pid) {
        auto it = lookupCharPidToEnggy.find(std::make_pair(c, pid));
        if (it != lookupCharPidToEnggy.end()) {
            return it->second;
        }
        return charToEnggy(c);
    }

    std::string enggyToString(std::string const &enggy, std::size_t *ppos = nullptr, std::size_t *dppos = nullptr) {
        std::size_t pos, bpos, epos;
        std::string result;
        if (ppos) *ppos = 0;
        if (dppos) *dppos = 0;
        for (pos = 0; pos != std::u32string::npos; pos = epos) {
            bpos = enggy.find_first_of("abcdefghijklmnopqrstuvwxyz", pos);
            if (bpos != pos) {
                result.append(enggy.substr(pos, bpos != std::u32string::npos ? bpos - pos : std::u32string::npos));
            }
            if (bpos != std::u32string::npos) {
                epos = enggy.find_first_of("0123456789", bpos);
                if (epos != std::u32string::npos) {
                    epos = enggy.find_first_not_of("0123456789", epos);
                }
                auto chunk = enggy.substr(bpos, epos != std::u32string::npos ? epos - bpos : std::u32string::npos);
                char32_t c = enggyToChar(chunk);
                if (c) {
                    result.append(utf32toC(c));
                    if (ppos) *ppos = epos;
                    if (dppos) *dppos = result.size();
                } else {
                    result.append(chunk);
                }
            } else {
                epos = bpos;
            }
        }
        return result;
    }
};

}
