#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include <pinyincpp/resources.hpp>
#include <pinyincpp/utf8.hpp>
#include <pinyincpp/pinyin.hpp>
#include <pinyincpp/trie.hpp>
#include <pinyincpp/counting_iterator.hpp>
#include <pinyincpp/inline_vector.hpp>

namespace pinyincpp {

struct PinyinWordsDB {
    struct WordData {
        std::u16string word;
        float score;
        InlineVector<PidTone, 6> pinyin;
    };

    std::vector<WordData> wordData;
    TrieMultimap<Pid, std::size_t, InlineVector<Pid, 6>, InlineVector<std::size_t, 3>> triePinyinToWord;

    explicit PinyinWordsDB() {
        BytesReader f = CMakeResource("data/pinyin-words.bin").view();
        auto nWords = f.read32();
        wordData.reserve(nWords);
        for (std::size_t i = 0; i < nWords; ++i) {
            auto nPidTones = f.read8();
            InlineVector<PidTone, 6> pidTones;
            InlineVector<Pid, 6> pidUntoned;
            pidTones.reserve(nPidTones);
            pidUntoned.reserve(nPidTones);
            for (std::size_t j = 0; j < nPidTones; ++j) {
                std::uint16_t pidTone = f.read16();
                Pid pid = pidTone >> 3;
                std::uint8_t tone = pidTone & 7;
                pidTones.push_back({pid, tone});
                pidUntoned.push_back(pid);
            }
            std::size_t wordBaseId = wordData.size();
            auto lenWords = f.read8();
            while (lenWords) {
                double logProb = (double)f.read16() / 2048;
                auto word = f.reads<std::u16string>(lenWords);
                wordData.push_back({std::move(word), static_cast<float>(logProb), std::move(pidTones)});
                lenWords = f.read8();
            }
            triePinyinToWord.batchedInsert(pidUntoned, CountingIterator{wordBaseId}, CountingIterator{wordData.size()});
        }
    }

    void addCustomWords(PinyinDB &db, std::vector<std::pair<std::string, std::string>> const &pinyinAndWords, double effectivity = 0.0) {
        for (auto &[pinyinStr, wordStr]: pinyinAndWords) {
            auto wordUtf32 = utfCto32(wordStr);
            double score = 1;
            std::size_t num = 0;
            for (char32_t c: wordUtf32) {
                auto logProb = db.charLogFrequency(c);
                num += logProb > 0 ? 1 : 0;
                score += logProb;
            }
            score /= std::max(1.0, (double)num);
            auto pinyin = db.pinyinSplit(utfCto32(pinyinStr), U' ');
            InlineVector<Pid, 6> pinyinInl(pinyin.begin(), pinyin.end());
            InlineVector<PidTone, 6> pidToned;
            pidToned.reserve(pinyin.size());
            for (auto pid: pinyin) {
                pidToned.push_back({pid, 0});
            }
            triePinyinToWord.insert(pinyinInl, wordData.size());
            wordData.push_back({utf32to16(wordUtf32), static_cast<float>(score * effectivity), std::move(pidToned)});
        }
    }
};

}
