#include <cmath>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::string utf32to8(char32_t c) {
    std::string result;
    if (c < 0x80) {
        result.resize(1);
        result[0] = (char)c;
    } else if (c < 0x800) {
        result.resize(2);
        result[0] = (char)(0xC0 | (c >> 6));
        result[1] = (char)(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        result.resize(3);
        result[0] = (char)(0xE0 | (c >> 12));
        result[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        result[2] = (char)(0x80 | (c & 0x3F));
    } else if (c < 0x200000) {
        result.resize(4);
        result[0] = (char)(0xF0 | (c >> 18));
        result[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        result[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        result[3] = (char)(0x80 | (c & 0x3F));
    } else {
        result.resize(1);
        result[0] = '?';
    }
    return result;
}

std::u32string utf8to32(const std::string &s) {
    std::u32string result;
    result.reserve(s.size());
    std::uint32_t state = 0;
    std::uint32_t codepoint = 0;
    for (auto c : s) {
        std::uint8_t byte = (std::uint8_t)c;
        if (state == 0) {
            if (byte < 0x80) {
                result.push_back(codepoint = byte);
            } else if (0xC0 <= byte && byte < 0xE0) {
                codepoint = byte & 0x1F;
                state = 1;
            } else if (0xE0 <= byte && byte < 0xF0) {
                codepoint = byte & 0x0F;
                state = 2;
            } else if (0xF0 <= byte && byte < 0xF8) {
                codepoint = byte & 0x07;
                state = 3;
            }
        } else {
            codepoint = (codepoint << 6) | (byte & 0x3F);
            state -= 1;
            if (state == 0) {
                result.push_back(codepoint);
            }
        }
    }
    return result;
}

std::string utf32to8(std::u32string const &s) {
    std::string result;
    result.reserve(s.size());
    for (char32_t c : s) {
        if (c < 0x80) {
            result.push_back(c);
        } else if (c < 0x800) {
            result.push_back((char)(0xC0 | (c >> 6)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            result.push_back((char)(0xE0 | (c >> 12)));
            result.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c < 0x200000) {
            result.push_back((char)(0xF0 | (c >> 18)));
            result.push_back((char)(0x80 | ((c >> 12) & 0x3F)));
            result.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        }
    }
    return result;
}

struct PinyinDB {
    PinyinDB() = default;

    using Pid = std::int32_t;
    using PidSet = std::vector<Pid>;

    struct WordData {
        char32_t word;
        PidSet pinyin;
        std::uint32_t freq;
    };
    struct PinyinData {
        std::string pinyin;
    };

    std::vector<WordData> wordData;
    std::vector<PinyinData> pinyinData;

    void loadFromCSV(std::ifstream &fin) {
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
                if ('0' <= tmp.back() && tmp.back() <= '9') {
                    tmp.pop_back();
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
            wordData.push_back({word, std::move(pinyin), freq});
        }
        for (const auto &c : wordData) {
            for (auto p : c.pinyin) {
                lookupPinyinToWord[p].push_back(c.word);
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
    std::unordered_map<Pid, std::vector<char32_t>> lookupPinyinToWord;
    std::unordered_map<char32_t, PidSet> lookupWordToPinyin;

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
            return utf32to8((char32_t)-pinyin);
        }
        if (pinyin >= pinyinData.size()) [[unlikely]] {
            return "";
        }
        return pinyinData[pinyin].pinyin;
    }

    std::vector<char32_t> const &pinyinToWord(Pid p) {
        return lookupPinyinToWord[p];
    }

    PidSet const &wordToPinyin(char32_t word) {
        return lookupWordToPinyin[word];
    }

    std::vector<PidSet> stringToPinyin(std::u32string const &str) {
        std::vector<PidSet> result;
        result.reserve(str.size());
        for (char32_t c : str) {
            result.emplace_back(wordToPinyin(c)).push_back(-(Pid)c);
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

    static double matchScore(std::vector<std::size_t> const &matches, std::size_t targetLen, std::size_t expectSize) {
        if (matches.empty()) return 0;
        // first, compute the sum of differences squared
        // e.g. 2,5,6,7 -> (5-2)**2 + (6-5)**2 + (7-6)**2 = 9
        std::uint32_t sumDiff = 0;
        for (auto it = matches.begin(); it != matches.end(); ++it) {
            auto jt = it;
            ++jt;
            if (jt != matches.end()) {
                std::uint32_t delta = *jt - *it;
                sumDiff += delta * delta;
            }
        }
        std::uint32_t delta = expectSize - matches.size() + 1;
        sumDiff += delta * delta;
        sumDiff += matches.front();
        sumDiff += targetLen - matches.back();
        // then, the score is the num of matches / the sum of differences squared
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
            token.push_back(pinyin[i]);
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
                    if (pinyin[i] && pinyin[i] != U'\'' && pinyin[i] != U' ') {
                        result.push_back(-(Pid)pinyin[i]);
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

    std::vector<std::size_t> simpleMatchPinyin(
        std::vector<std::string> const &candidates, std::string const &query,
        std::size_t limit = (std::size_t)-1) {
        auto qPids = pinyinSplit(utf8to32(query));
        std::vector<std::vector<PidSet>> cPidSets;
        cPidSets.reserve(candidates.size());
        for (auto &c : candidates) {
            cPidSets.emplace_back(stringToPinyin(utf8to32(c)));
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
    std::vector<Id> aliasedMatchPinyin(
        std::vector<std::pair<Id, std::vector<std::string>>> const &candidates, std::string const &query,
        std::size_t limit = (std::size_t)-1) {
        auto qPids = pinyinSplit(utf8to32(query));
        std::vector<std::vector<PidSet>> cPidSets;
        std::vector<std::size_t> aliasTargets;
        cPidSets.reserve(candidates.size());
        aliasTargets.reserve(candidates.size());
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            for (auto &c : candidates[i].second) {
                cPidSets.emplace_back(stringToPinyin(utf8to32(c)));
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

    std::vector<HighlightMatchResult> highlightMatchPinyin(
        std::vector<std::string> const &candidates, std::string const &query, std::size_t limit = (std::size_t)-1,
        std::string const &hlBegin = "<em>", std::string const &hlEnd = "</em>") {
        auto qPids = pinyinSplit(utf8to32(query));
        std::vector<std::u32string> candidates32;
        for (auto s : candidates) {
            candidates32.emplace_back(utf8to32(s));
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
                text.append(utf32to8(source[i]));
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

int main() {
    std::ifstream fin("chars.csv");
    PinyinDB db;
    db.loadFromCSV(fin);
    std::vector<std::pair<int, std::vector<std::string>>> list = {
        {114, {"妈刀", "妈妈的菜刀", "马刀", "Mom's knife"}},
        {115, {"小妈刀", "妈妈的剃须刀", "Mom's blade"}},
        {118, {"硫磺火", "牛黄火", "Brimstone"}},
        {119, {"炼金硫磺", "硫磺", "Sulfur"}},
    };
    std::string query = "mamadetixudao";
    auto results = db.aliasedMatchPinyin(list, query);
    for (auto index : results) {
        std::cout << index << std::endl;
    }
    return 0;
}
