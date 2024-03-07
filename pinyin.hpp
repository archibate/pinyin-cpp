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

namespace pinyincpp {

extern const char chars_csv[];
extern const unsigned long long chars_csv_size;

struct PinyinDB {
    using Pid = std::int32_t;
    using PidSet = std::vector<Pid>;

    struct WordInfo {
        char32_t word;
        double logFreq;
    };

    struct WordCandidate {
        char32_t word;
        double score;
    };

    struct WordData : WordInfo {
        PidSet pinyin;
    };
    struct PinyinData {
        std::string pinyin;
    };

    std::vector<WordData> wordData;
    std::vector<PinyinData> pinyinData;

    explicit PinyinDB(bool withTone = false) {
        double sumLogFreq = 0;
        std::istringstream fin(std::string(chars_csv, chars_csv_size));
        std::string line;
        while (std::getline(fin, line)) {
            std::string tmp;
            std::istringstream iss(line);
            std::getline(iss, tmp, ',');
            char32_t word = std::stoul(tmp);
            std::getline(iss, tmp, ',');
            std::uint32_t freq = std::stoul(tmp);
            PidSet pinyin;
            std::unordered_set<Pid> addedPinyin;
            while (std::getline(iss, tmp, '/')) {
                if (!withTone) {
                    if ('0' <= tmp.back() && tmp.back() <= '9') {
                        tmp.pop_back();
                    }
                } else {
                    if (!('0' <= tmp.back() && tmp.back() <= '9')) {
                        tmp.push_back('0');
                    }
                }
                auto [it, success] = lookupPinyinToPid.insert({tmp, pinyinData.size()});
                Pid pid = it->second;
                if (addedPinyin.insert(pid).second) {
                    pinyin.push_back(pid);
                }
                if (success) {
                    pinyinData.push_back({tmp});
                }
            }
            auto logFreq = std::log(freq + 2);
            wordData.push_back({word, logFreq, std::move(pinyin)});
            sumLogFreq += logFreq;
        }
        auto scaleLogFreq = 1 / (1 + sumLogFreq / wordData.size() + 1);
        for (auto &c : wordData) {
            c.logFreq = c.logFreq * scaleLogFreq;
        }
        for (const auto &c : wordData) {
            for (auto p : c.pinyin) {
                lookupPinyinToWord[p].push_back(static_cast<WordInfo>(c));
            }
        }
        for (const auto &c : wordData) {
            lookupWordToPinyin[c.word] = c.pinyin;
        }
        for (const auto &[p, pid] : lookupPinyinToPid) {
            for (std::size_t i = 1; i <= p.size(); ++i) {
                lookupPinyinPrefixSet.insert(p.substr(0, i));
            }
        }
    }

    std::unordered_map<std::string, Pid> lookupPinyinToPid;
    std::unordered_set<std::string> lookupPinyinPrefixSet;
    std::unordered_map<Pid, std::vector<WordInfo>> lookupPinyinToWord;
    std::unordered_map<char32_t, PidSet> lookupWordToPinyin;
    std::u32string sampleString;
    double sampleEffectivity = 1.0;

    void setSampleString(std::u32string const &sample, double effectivity = 1.0) {
        sampleString = sample;
        sampleEffectivity = effectivity;
    }

    void setSampleString(std::string const &sample, double effectivity = 1.0) {
        setSampleString(utfCto32(sample), effectivity);
    }

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
            return "";
        }
        return pinyinData[pinyin].pinyin;
    }

    std::vector<WordInfo> pinyinToWord(Pid p) {
        auto it = lookupPinyinToWord.find(p);
        if (it == lookupPinyinToWord.end()) {
            return {};
        } else {
            return it->second;
        }
    }

    PidSet wordToPinyin(char32_t word) {
        auto it = lookupWordToPinyin.find(word);
        if (it == lookupWordToPinyin.end()) {
            return {};
        } else {
            return it->second;
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
            result.emplace_back(wordToPinyin(c)).push_back(-(Pid)c);
        }
        return result;
    }

    static std::unordered_map<char32_t, double> wordOccurances(std::u32string const &sample, std::u32string const &prefix) {
        std::unordered_map<char32_t, double> occurance;
        if (sample.empty()) {
            return occurance;
        }
        // find occurance in in sampleString
        if (prefix.empty()) {
            for (std::size_t i = 0; i < sample.size(); ++i) {
                occurance[sample[i]] += 0.25;
            }
        } else {
            auto matches = searchStringPostfix(sample, prefix);
            if (!matches.empty()) {
                for (auto const &[match, count] : matches) {
                    if (count && match < sample.size()) {
                        /* std::cout << utf32toC(prefix) << ' ' << count << ' ' << utf32toC(sample[match]) << '\n'; */
                        occurance[sample[match]] += std::min(count * count, (std::size_t)10);
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

    std::unordered_map<char32_t, double> suggestCandidatesMap(std::u32string const &prefix) {
        constexpr std::size_t kMaxPrefix = 4;
        std::u32string lastPrefix;
        std::u32string beforePrefix;
        if (prefix.size() > kMaxPrefix) {
            lastPrefix = prefix.substr(prefix.size() - kMaxPrefix);
            beforePrefix = prefix.substr(0, prefix.size() - kMaxPrefix + 1);
        } else {
            lastPrefix = prefix;
        }
        auto occurance = wordOccurances(sampleString, lastPrefix);
        if (!beforePrefix.empty()) {
            for (auto const &[word, logProb]: wordOccurances(beforePrefix, lastPrefix)) {
                occurance[word] += logProb;
            }
        }
        return occurance;
    }

    std::vector<WordCandidate> suggestCandidates(std::u32string const &prefix) {
        std::vector<WordCandidate> result;
        auto occurance = suggestCandidatesMap(prefix);
        for (auto const &[word, logProb] : occurance) {
            result.push_back({word, logProb * sampleEffectivity});
        }
        std::sort(result.begin(), result.end(), [](WordCandidate const &a, WordCandidate const &b) {
            return a.score > b.score;
        });
        return result;
    }

    std::vector<WordCandidate> pinyinCandidates(std::u32string const &prefix, Pid pid) {
        auto occurance = suggestCandidatesMap(prefix);
        auto wordProbability = [&](char32_t word) -> double {
            auto it = occurance.find(word);
            double logProb = 0;
            if (it != occurance.end()) {
                logProb = it->second * sampleEffectivity;
                occurance.erase(it);
            }
            return logProb;
        };
        std::vector<WordCandidate> candidates;
        if (pid < 0) {
            auto c = (char32_t)-pid;
            candidates.push_back({c, wordProbability(c)});
        } else {
            auto it = lookupPinyinToWord.find(pid);
            if (it != lookupPinyinToWord.end()) {
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

    std::vector<std::vector<std::string>> simpleStringToPinyin(std::string const &str) {
        std::vector<std::vector<std::string>> result;
        auto str32 = utfCto32(str);
        result.reserve(str32.size());
        for (char32_t c : str32) {
            auto &back = result.emplace_back();
            auto pidSet = wordToPinyin(c);
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

    static std::vector<std::size_t> matchPinyin(std::vector<PidSet> const &p1, std::vector<Pid> const &p2) {
        // find max sub-sequence length
        std::size_t l1 = p1.size();
        std::size_t l2 = p2.size();
        std::vector<std::size_t> dp((l1 + 1) * (l2 + 1));
        for (std::size_t i = 0; i < l1; ++i) {
            std::size_t *dp0 = dp.data() + i * (l2 + 1);
            std::size_t *dp1 = dp0 + (l2 + 1);
            for (std::size_t j = 0; j < l2; ++j) {
                if (std::find(p1[i].begin(), p1[i].end(), p2[j]) != p1[i].end()) {
                    dp1[j + 1] = dp0[j] + 1;
                } else {
                    dp1[j + 1] = std::max(dp0[j + 1], dp1[j]);
                }
            }
        }
        // trace back to get matches
        std::size_t i = l1, j = l2;
        std::vector<std::size_t> matches;
        while (i > 0 && j > 0) {
            const std::size_t *dp1 = dp.data() + (i - 1) * (l2 + 1);
            const std::size_t *dp0 = dp1 + (l2 + 1);
            if (dp0[j] == dp1[j]) {
                --i;
            } else if (dp0[j] == dp0[j - 1]) {
                --j;
            } else {
                matches.push_back(i - 1);
                --i;
                --j;
            }
        }
        std::reverse(matches.begin(), matches.end());
        return matches;
    }

    static std::vector<std::size_t> matchString(std::u32string const &s1, std::u32string const &s2) {
        // find max sub-sequence length
        std::size_t l1 = s1.size();
        std::size_t l2 = s2.size();
        std::vector<std::size_t> dp((l1 + 1) * (l2 + 1));
        for (std::size_t i = 0; i < l1; ++i) {
            std::size_t *dp0 = dp.data() + i * (l2 + 1);
            std::size_t *dp1 = dp0 + (l2 + 1);
            for (std::size_t j = 0; j < l2; ++j) {
                if (s1[i] == s2[j]) {
                    dp1[j + 1] = dp0[j] + 1;
                } else {
                    dp1[j + 1] = std::max(dp0[j + 1], dp1[j]);
                }
            }
        }
        // trace back to get matches
        std::size_t i = l1, j = l2;
        std::vector<std::size_t> matches;
        while (i > 0 && j > 0) {
            const std::size_t *dp1 = dp.data() + (i - 1) * (l2 + 1);
            const std::size_t *dp0 = dp1 + (l2 + 1);
            if (dp0[j] == dp1[j]) {
                --i;
            } else if (dp0[j] == dp0[j - 1]) {
                --j;
            } else {
                matches.push_back(i - 1);
                --i;
                --j;
            }
        }
        std::reverse(matches.begin(), matches.end());
        return matches;
    }

    static double matchScore(std::vector<std::size_t> const &matches, std::size_t targetLen, std::size_t expectSize) {
        if (matches.empty()) return 0;
        // first, compute the sum of differences squared
        // e.g. 2,5,6,7 -> (5-2)**2 + (6-5)**2 + (7-6)**2 = 9
        std::uint32_t sumDiff = 1;
        for (auto it = matches.begin(); it != matches.end(); ++it) {
            auto jt = it;
            ++jt;
            if (jt != matches.end()) {
                std::uint32_t delta = *jt - *it;
                sumDiff += delta * delta;
            }
        }
        if (targetLen && expectSize) {
            std::uint32_t delta = expectSize - matches.size() + 1;
            sumDiff += delta * delta;
            sumDiff += matches.front();
            sumDiff += targetLen - matches.back();
        }
        // then, the score is 1 / the sum of differences squared
        return 1.0 / sumDiff;
    }

    struct MatchResult {
        std::size_t index;
        double score;
        std::vector<std::size_t> highlights;
    };

    static std::vector<MatchResult> batchedMatchPinyin(std::vector<std::vector<PidSet>> const &p1s, std::vector<Pid> const &p2) {
        std::vector<MatchResult> matches;
        for (std::size_t i = 0; i < p1s.size(); ++i) {
            auto highlights = matchPinyin(p1s[i], p2);
            double score = matchScore(highlights, p1s[i].size(), p2.size());
            if (score > 0) {
                matches.push_back({i, score, highlights});
            }
        }
        return matches;
    }

    static void sortMatchResults(std::vector<MatchResult> &matches) {
        std::sort(matches.begin(), matches.end(), [] (MatchResult const &a, MatchResult const &b) {
            return a.score > b.score;
        });
    }

    std::vector<Pid> pinyinSplit(std::u32string const &pinyin) {
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
                    if (c && c != U'\'' && c != U' ') {
                        if ('A' <= c && c <= 'Z') {
                            c += 'a' - 'A';
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

    std::vector<std::string> simplePinyinSplit(std::string const &pinyin) {
        auto pinyin32 = utfCto32(pinyin);
        auto pids = pinyinSplit(pinyin32);
        std::vector<std::string> result;
        result.reserve(pids.size());
        for (auto p : pids) {
            result.push_back(pinyinName(p));
        }
        return result;
    }

    std::vector<std::size_t> simpleMatchPinyin(
        std::vector<std::string> const &candidates, std::string const &query,
        std::size_t limit = (std::size_t)-1) {
        auto qPids = pinyinSplit(utfCto32(query));
        std::vector<std::vector<PidSet>> cPidSets;
        cPidSets.reserve(candidates.size());
        for (auto &c : candidates) {
            cPidSets.emplace_back(stringToPinyin(utfCto32(c)));
        }
        auto matches = batchedMatchPinyin(cPidSets, qPids);
        sortMatchResults(matches);
        std::vector<std::size_t> indices;
        indices.reserve(std::min(limit, matches.size()));
        for (auto const &m: matches) {
            indices.push_back(m.index);
            if (indices.size() >= limit) break;
        }
        return indices;
    }

    template <class Id>
    std::vector<Id> simpleAliasedMatchPinyin(
        std::vector<std::pair<Id, std::vector<std::string>>> const &candidates, std::string const &query,
        std::size_t limit = (std::size_t)-1) {
        auto qPids = pinyinSplit(utfCto32(query));
        std::vector<std::vector<PidSet>> cPidSets;
        std::vector<std::size_t> aliasTargets;
        cPidSets.reserve(candidates.size());
        aliasTargets.reserve(candidates.size());
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            for (auto &c : candidates[i].second) {
                cPidSets.emplace_back(stringToPinyin(utfCto32(c)));
                aliasTargets.emplace_back(i);
            }
        }
        auto matches = batchedMatchPinyin(cPidSets, qPids);
        std::vector<MatchResult> uniqueMatches;
        std::unordered_map<std::size_t, std::size_t> visitedMatches;
        uniqueMatches.reserve(matches.size());
        for (auto &m: matches) {
            m.index = aliasTargets[m.index];
            auto [it, success] = visitedMatches.insert({m.index, uniqueMatches.size()});
            if (success) {
                uniqueMatches.push_back(m);
            } else {
                auto &um = uniqueMatches[it->second];
                if (m.score > um.score) {
                    um = m;
                }
            }
        }
        sortMatchResults(uniqueMatches);
        std::vector<Id> ids;
        ids.reserve(std::min(limit, uniqueMatches.size()));
        for (auto const &m: uniqueMatches) {
            /* std::cout << "index: " << m.index << " score: " << m.score << " text: " << candidates[m.index].first << std::endl; */
            ids.push_back(candidates[m.index].first);
            if (ids.size() >= limit) break;
        }
        return ids;
    }

    struct HighlightMatchResult {
        std::size_t index;
        double score;
        std::string text;
    };

    std::vector<HighlightMatchResult> simpleHighlightMatchPinyin(
        std::vector<std::string> const &candidates, std::string const &query, std::size_t limit = (std::size_t)-1,
        std::string const &hlBegin = "<em>", std::string const &hlEnd = "</em>") {
        auto qPids = pinyinSplit(utfCto32(query));
        std::vector<std::u32string> candidates32;
        for (auto s : candidates) {
            candidates32.emplace_back(utfCto32(s));
        }
        std::vector<std::vector<PidSet>> results;
        results.reserve(candidates.size());
        for (auto &c : candidates32) {
            results.emplace_back(stringToPinyin(c));
        }
        auto matches = batchedMatchPinyin(results, qPids);
        sortMatchResults(matches);
        std::vector<HighlightMatchResult> result;
        result.reserve(std::min(limit, matches.size()));
        for (auto const &m: matches) {
            std::string text;
            std::u32string source = candidates32[m.index];
            std::vector<bool> activated(source.size());
            for (std::size_t highlight : m.highlights) {
                activated[highlight] = true;
            }
            bool active = false;
            for (std::size_t i = 0; i < source.size(); ++i) {
                if (activated[i]) {
                    if (!active) {
                        text.append(hlBegin);
                    }
                } else {
                    if (active) {
                        text.append(hlEnd);
                    }
                }
                active = activated[i];
                text.append(utf32toC(source[i]));
            }
            if (active) {
                text.append(hlEnd);
            }
            result.push_back({m.index, m.score, text});
            if (result.size() >= limit) break;
        }
        return result;
    }
};

}
