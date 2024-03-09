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
#include <pinyincpp/sort_top_n.hpp>
#include <pinyincpp/pinyin.hpp>

namespace pinyincpp {

struct PinyinMatch {
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
        if (matches.empty()) [[unlikely]] return 0;
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
            if (score > 0) [[likely]] {
                matches.push_back({i, score, highlights});
            }
        }
        return matches;
    }

    std::vector<std::size_t> simpleMatchPinyin(PinyinDB &db,
        std::vector<std::string> const &candidates, std::string const &query,
        std::size_t numResults = (std::size_t)-1) {
        auto qPids = db.pinyinSplit(utfCto32(query), true);
        std::vector<std::vector<PidSet>> cPidSets;
        cPidSets.reserve(candidates.size());
        for (auto &c : candidates) {
            cPidSets.emplace_back(db.stringToPinyin(utfCto32(c), true));
        }
        auto matches = batchedMatchPinyin(cPidSets, qPids);
        sortTopN(matches, numResults, &MatchResult::score);
        std::vector<std::size_t> indices;
        indices.reserve(std::min(numResults, matches.size()));
        for (auto const &m: matches) {
            indices.push_back(m.index);
            if (indices.size() >= numResults) break;
        }
        return indices;
    }

    template <class Id>
    std::vector<Id> simpleAliasedMatchPinyin(PinyinDB &db,
        std::vector<std::pair<Id, std::vector<std::string>>> const &candidates, std::string const &query,
        std::size_t numResults = (std::size_t)-1) {
        auto qPids = db.pinyinSplit(utfCto32(query), true);
        std::vector<std::vector<PidSet>> cPidSets;
        std::vector<std::size_t> aliasTargets;
        cPidSets.reserve(candidates.size());
        aliasTargets.reserve(candidates.size());
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            for (auto &c : candidates[i].second) {
                cPidSets.emplace_back(db.stringToPinyin(utfCto32(c), true));
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
        sortTopN(uniqueMatches, numResults, &MatchResult::score);
        std::vector<Id> ids;
        ids.reserve(std::min(numResults, uniqueMatches.size()));
        for (auto const &m: uniqueMatches) {
            ids.push_back(candidates[m.index].first);
            if (ids.size() >= numResults) break;
        }
        return ids;
    }

    struct HighlightMatchResult {
        std::size_t index;
        double score;
        std::string text;
    };

    std::vector<HighlightMatchResult> simpleHighlightMatchPinyin(PinyinDB &db,
        std::vector<std::string> const &candidates, std::string const &query, std::size_t numResults = (std::size_t)-1,
        std::string const &hlBegin = "<em>", std::string const &hlEnd = "</em>") {
        auto qPids = db.pinyinSplit(utfCto32(query), true);
        std::vector<std::u32string> candidatesUtf32;
        for (auto s : candidates) {
            candidatesUtf32.emplace_back(utfCto32(s));
        }
        std::vector<std::vector<PidSet>> results;
        results.reserve(candidates.size());
        for (auto &c : candidatesUtf32) {
            results.emplace_back(db.stringToPinyin(c, true));
        }
        auto matches = batchedMatchPinyin(results, qPids);
        sortTopN(matches, numResults, &MatchResult::score);
        std::vector<HighlightMatchResult> result;
        result.reserve(std::min(numResults, matches.size()));
        for (auto const &m: matches) {
            std::string text;
            std::u32string source = candidatesUtf32[m.index];
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
            if (result.size() >= numResults) break;
        }
        return result;
    }
};

}
