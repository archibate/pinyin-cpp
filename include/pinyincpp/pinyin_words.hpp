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
        InlineVector<PidTone, 6> pinyin;
        float score;
    };

#if 0
    template <class, class NodePtr>
    struct TrieKVPair {
#ifdef __x86_64__
    private:
        struct Second {
            constexpr auto &operator*() const noexcept {
                auto *that = reinterpret_cast<TrieKVPair const *>(this);
                return *that->get_second();
            }
        };
    public:
        [[no_unique_address]] Second second;

    private:
        std::intptr_t real_second: 48;
    public:
        Pid first: 16;

    public:
        constexpr auto *get_second() const noexcept {
            return reinterpret_cast<typename NodePtr::element_type *>(real_second);
        }
    public:
        constexpr TrieKVPair(Pid first, NodePtr second) noexcept
        : first(first), real_second(reinterpret_cast<std::intptr_t>(second.release())) {
        }
        constexpr TrieKVPair(TrieKVPair &&that) noexcept
        : first(that.first), real_second(that.real_second) {
            that.real_second = 0;
        }
        constexpr TrieKVPair &operator=(TrieKVPair &&that) noexcept {
            first = that.first;
            delete get_second();
            real_second = that.real_second;
            that.real_second = 0;
            return *this;
        }
        constexpr ~TrieKVPair() {
            delete get_second();
        }
#else
        Pid first;
        NodePtr second;
#endif
    };
#ifdef __x86_64__
    static_assert(sizeof(TrieKVPair<Pid, std::unique_ptr<char>>) == 8);
#endif

    std::vector<WordData> wordData;
    TrieMultimap<Pid, std::size_t, InlineVector<Pid, 6>, InlineVector<std::size_t, 3>, TrieKVPair> triePinyinToWord;
#else
    std::vector<WordData> wordData;
    TrieMultimap<Pid, std::size_t, InlineVector<Pid, 6>, InlineVector<std::size_t, 3>> triePinyinToWord;
#endif

    explicit PinyinWordsDB() {
        BytesReader f = CMakeResource("data/pinyin-words.bin").view();
        auto nWords = f.read32();
        wordData.reserve(nWords);
        for (std::size_t i = 0; i < nWords; ++i) {
            auto nPidTones = f.read8();
            decltype(WordData{}.pinyin) pidTones;
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
                wordData.push_back({std::move(word), std::move(pidTones), static_cast<float>(logProb)});
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
            decltype(WordData{}.pinyin) pidToned;
            pidToned.reserve(pinyin.size());
            for (auto pid: pinyin) {
                pidToned.push_back({pid, 0});
            }
            triePinyinToWord.insert(pinyin, wordData.size());
            wordData.push_back({utf32to16(wordUtf32), std::move(pidToned), static_cast<float>(score * effectivity)});
        }
    }
};

}
