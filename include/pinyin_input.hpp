#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "utf8.hpp"
#include "pinyin.hpp"

namespace pinyincpp {

struct PinyinInput {
    struct WordCandidate {
        char32_t word;
        double score;
    };

    struct SampleString {
        std::u32string content;
        double effectivity;
    };

    std::vector<SampleString> sampleStrings;

    void addSampleString(std::u32string const &sample, double effectivity) {
        sampleStrings.push_back({sample, effectivity});
    }

    void addSampleString(std::string const &sample, double effectivity) {
        addSampleString(utfCto32(sample), effectivity);
    }

    static std::unordered_map<std::size_t, std::size_t> searchStringPostfix(std::u32string const &sample, std::u32string const &postfix) {
        // find postfix occurance in sample
        std::unordered_map<std::size_t, std::size_t> occurance;
        if (postfix.empty()) {
            return occurance;
        }
        for (std::size_t num = 1; num <= postfix.size(); ++num) {
            std::size_t pos = 0;
            auto s = postfix.substr(postfix.size() - num, num);
            while (true) {
                pos = sample.find(s, pos);
                if (pos == std::u32string::npos) {
                    break;
                }
                /* std::cout << utf32toC(s) << ' ' << utf32toC(sample.substr(pos, num)) << '\n'; */
                pos += num;
                occurance.insert_or_assign(pos, num);
            }
        }
        return occurance;
    }

    static std::unordered_map<char32_t, double> wordOccurances(
        std::u32string const &sample, std::u32string const &prefix, std::vector<double> const &scoreTable) {
        std::unordered_map<char32_t, double> occurance;
        if (sample.empty() || scoreTable.empty()) {
            return occurance;
        }
        // find occurance in in sampleString
        for (std::size_t i = 0; i < sample.size(); ++i) {
            occurance[sample[i]] += scoreTable[0];
        }
        if (!prefix.empty()) {
            auto matches = searchStringPostfix(sample, prefix);
            if (!matches.empty()) {
                for (auto const &[match, count] : matches) {
                    if (count && match < sample.size()) {
                        /* std::cout << utf32toC(prefix) << '|' << count << '|' << utf32toC(sample[match]) << '\n'; */
                        occurance[sample[match]] += scoreTable[std::min(count, scoreTable.size() - 1)];
                    }
                }
            }
        }
        for (auto &[word, logProb] : occurance) {
            logProb = std::log(logProb + 1);
            /* std::cout << '-' << utf32toC(word) << ' ' << logProb << '\n'; */
        }
        return occurance;
    }

    std::unordered_map<char32_t, double> suggestWordCandidatesMap(std::u32string const &prefix) {
        constexpr std::size_t kMaxPrefix = 4;
        std::u32string lastPrefix;
        std::u32string beforePrefix;
        if (prefix.size() > kMaxPrefix) {
            lastPrefix = prefix.substr(prefix.size() - kMaxPrefix);
            beforePrefix = prefix.substr(0, prefix.size() - kMaxPrefix + 1);
        } else {
            lastPrefix = prefix;
        }
        std::unordered_map<char32_t, double> occurance;
        const std::vector<double> scoreTable = {0.2, 1, 4, 8, 10};
        for (auto const &sample: sampleStrings) {
            for (auto const &[word, logProb]: wordOccurances(sample.content, lastPrefix, scoreTable)) {
                occurance[word] += logProb * sample.effectivity;
            }
            if (!beforePrefix.empty()) {
                for (auto const &[word, logProb]: wordOccurances(beforePrefix, lastPrefix, scoreTable)) {
                    occurance[word] += logProb * sample.effectivity;
                }
            }
        }
        return occurance;
    }

    std::vector<WordCandidate> suggestWordCandidates(std::u32string const &prefix) {
        std::vector<WordCandidate> result;
        auto occurance = suggestWordCandidatesMap(prefix);
        for (auto const &[word, logProb] : occurance) {
            result.push_back({word, logProb});
        }
        std::sort(result.begin(), result.end(), [](WordCandidate const &a, WordCandidate const &b) {
            return a.score > b.score;
        });
        return result;
    }

    std::vector<WordCandidate> pinyinWordCandidates(PinyinDB &db, std::u32string const &prefix, Pid pid) {
        auto occurance = suggestWordCandidatesMap(prefix);
        auto wordProbability = [&](char32_t word) -> double {
            auto it = occurance.find(word);
            double logProb = 0;
            if (it != occurance.end()) {
                logProb = it->second;
                occurance.erase(it);
            }
            return logProb;
        };
        std::vector<WordCandidate> candidates;
        if (pid < 0) {
            auto c = (char32_t)-pid;
            candidates.push_back({c, wordProbability(c)});
        } else {
            auto it = db.lookupPinyinToWord.find(pid);
            if (it != db.lookupPinyinToWord.end()) {
                for (auto &c : it->second) {
                    /* std::cout << utf32toC(c.word) << ' ' << wordProbability(c.word) << '\n'; */
                    candidates.push_back({c.word, c.logFreq + wordProbability(c.word)});
                }
            }
        }
        std::sort(candidates.begin(), candidates.end(), [](WordCandidate const &a, WordCandidate const &b) {
            return a.score > b.score;
        });
        return candidates;
    }
};

}
