#pragma once

#include <cmath>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <pinyincpp/utf8.hpp>
#include <pinyincpp/pinyin.hpp>
#include <pinyincpp/sort_top_n.hpp>
#include <pinyincpp/pinyin_words.hpp>
#include <pinyincpp/ctype.hpp>

namespace pinyincpp {

struct PinyinInput {
public:
    struct CharCandidate {
        char32_t character;
        double score;
    };

    struct WordCandidate {
        double score;
        std::u32string word;
        std::vector<Pid> pinyin;
    };

private:
    struct SampleString {
        std::u32string content;
        double effectivity;
    };

    std::vector<SampleString> sampleStrings;
    double prefixEffectivity = 5.0;

public:
    void setPrefixEffectivity(double effectivity = 1.0) {
        prefixEffectivity = effectivity;
    }

    void addSampleString(std::u32string const &sample, double effectivity = 1.0) {
        sampleStrings.push_back({sample, effectivity});
    }

    void addSampleString(std::string const &sample, double effectivity = 1.0) {
        addSampleString(utfCto32(sample), effectivity);
    }

    void clearSampleStrings() {
        sampleStrings.clear();
    }

private:
    static std::unordered_map<std::size_t, std::size_t> searchStringPostfix(std::u32string const &sample, std::u32string const &postfix) {
        // find postfix occurance in sample
        std::unordered_map<std::size_t, std::size_t> occurance;
        if (postfix.empty()) [[unlikely]] {
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
                pos += num;
                occurance.insert_or_assign(pos, num);
            }
        }
        return occurance;
    }

    static std::unordered_map<char32_t, double> charOccurances(
        std::u32string const &sample, std::u32string const &prefix, std::vector<double> const &scoreTable) {
        std::unordered_map<char32_t, double> occurance;
        if (sample.empty() || scoreTable.empty()) [[unlikely]] {
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
                        occurance[sample[match]] += scoreTable[std::min(count, scoreTable.size() - 1)];
                    }
                }
            }
        }
        for (auto &[character, logProb] : occurance) {
            logProb = std::log(logProb + 1);
        }
        return occurance;
    }

    static void mulScoreWordOccurances(std::vector<WordCandidate> &words, double effectivity,
        std::u32string const &sample, std::u32string const &prefix, std::vector<double> const &scoreTable) {
        std::vector<double> occurance(words.size());
        if (sample.empty() || scoreTable.empty()) [[unlikely]] {
            return;
        }
        auto maxScore = std::min(prefix.size(), scoreTable.size());
        // find occurance in in sampleString
        for (std::size_t w = 0; w < words.size(); ++w) {
            auto &word = words[w];
            double count = 0;
            for (std::size_t pos = 0; (pos = sample.find(word.word, pos)) != std::u32string::npos; pos += word.word.size()) {
                std::size_t s = 0;
                for (; s < std::min(maxScore, pos + 1); ++s) {
                    if (prefix[prefix.size() - 1 - s] == sample[pos - s]) {
                        break;
                    }
                }
                count += scoreTable[std::min(s, scoreTable.size())];
            }
            auto logProb = std::log(count + 1);
            word.score += effectivity * logProb;
        }
    }

    std::unordered_map<char32_t, double> suggestCharCandidatesMap(std::u32string const &prefix) {
        std::u32string lastPrefix;
        std::u32string beforePrefix;
        constexpr std::size_t kMaxPrefix = 4;
        if (prefix.size() > kMaxPrefix) {
            lastPrefix = prefix.substr(prefix.size() - kMaxPrefix);
            beforePrefix = prefix.substr(0, prefix.size() - kMaxPrefix + 1);
        } else {
            lastPrefix = prefix;
        }
        std::unordered_map<char32_t, double> occurance;
        const std::vector<double> scoreTable = {0.08, 1, 4, 8, 10};
        for (auto const &sample: sampleStrings) {
            for (auto const &[character, logProb]: charOccurances(sample.content, lastPrefix, scoreTable)) {
                occurance[character] += logProb * sample.effectivity;
            }
        }
        if (!beforePrefix.empty()) {
            for (auto const &[character, logProb]: charOccurances(beforePrefix, lastPrefix, scoreTable)) {
                occurance[character] += logProb * prefixEffectivity;
            }
        }
        return occurance;
    }

public:
    std::vector<CharCandidate> suggestCharCandidates(std::u32string const &prefix, std::size_t numResults = 100, bool chineseOnly = true) {
        std::vector<CharCandidate> candidates;
        auto occurance = suggestCharCandidatesMap(prefix);
        for (auto const &[character, logProb] : occurance) {
            if (!chineseOnly || isChineseCharacter(character)) {
                candidates.push_back({character, logProb});
            }
        }
        sortTopN(candidates, numResults, &CharCandidate::score);
        return candidates;
    }

    std::vector<WordCandidate> pinyinWordCandidates(PinyinDB &db, PinyinWordsDB &wd, std::u32string const &prefix, std::vector<Pid> const &pids, std::size_t numResults = 100, std::size_t depthLimit = 2) {
        std::vector<WordCandidate> candidates;
        if (!pids.empty()) {
            wd.triePinyinToWord.visitPrefix(pids, [&] (std::vector<Pid> const &fullPids, std::size_t const &wordIndex) {
                auto const &w = wd.wordData[wordIndex];
                candidates.push_back({w.score * pids.size() / (fullPids.size() + 1), utf16to32(w.word), fullPids});
                return false;
            }, depthLimit);
        } else {
            wd.triePinyinToWord.visitItems([&] (std::vector<Pid> const &fullPids, std::size_t const &wordIndex) {
                auto const &w = wd.wordData[wordIndex];
                candidates.push_back({w.score * pids.size() / (fullPids.size() + 1), utf16to32(w.word), fullPids});
                return false;
            }, depthLimit);
        }
        std::u32string lastPrefix;
        std::u32string beforePrefix;
        constexpr std::size_t kMaxPrefix = 4;
        if (prefix.size() > kMaxPrefix) {
            lastPrefix = prefix.substr(prefix.size() - kMaxPrefix);
            beforePrefix = prefix.substr(0, prefix.size() - kMaxPrefix + 1);
        } else {
            lastPrefix = prefix;
        }
        const std::vector<double> scoreTable = {0.08, 1, 4, 8, 10};
        for (auto const &sample: sampleStrings) {
            mulScoreWordOccurances(candidates, sample.effectivity, sample.content, lastPrefix, scoreTable);
        }
        if (!beforePrefix.empty()) {
            mulScoreWordOccurances(candidates, prefixEffectivity, beforePrefix, lastPrefix, scoreTable);
        }
        /* std::erase_if(candidates, [](WordCandidate const &a) { */
        /*     return a.score <= 0; */
        /* }); */
        sortTopN(candidates, numResults, &WordCandidate::score);
        return candidates;
    }

    std::vector<CharCandidate> pinyinCharCandidates(PinyinDB &db, std::u32string const &prefix, Pid pid, std::size_t numResults = 100) {
        auto occurance = suggestCharCandidatesMap(prefix);
        auto charProbability = [&](char32_t character) -> double {
            auto it = occurance.find(character);
            double logProb = 0;
            if (it != occurance.end()) {
                logProb = it->second;
                occurance.erase(it);
            }
            return logProb;
        };
        std::vector<CharCandidate> candidates;
        if (pid < 0) {
            auto c = extractSpecialPid(pid);
            candidates.push_back({c, charProbability(c)});
        } else {
            for (auto const &c : db.pinyinToChar(pid)) {
                candidates.push_back({c.character, c.logFrequency + charProbability(c.character)});
            }
        }
        sortTopN(candidates, numResults, &CharCandidate::score);
        return candidates;
    }
};

}
