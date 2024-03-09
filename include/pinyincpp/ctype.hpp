#pragma once

#include <cctype>
#include <cstdint>
#include <cuchar>
#include <tuple>
#include <string>
#include <string_view>

namespace pinyincpp {

inline bool isChineseCharacter(char32_t c) {
    return 0x4e00 <= c && c <= 0x9fff;
}

inline bool isChinesePunctuation(char32_t c) {
    return (0x3000 <= c && c <= 0x303f) || (0xff00 <= c && c <= 0xffef);
}

inline bool isEnglishCharacter(char32_t c) {
    return (0x0041 <= c && c <= 0x005a) || (0x0061 <= c && c <= 0x007a);
}

inline bool isEnglishPuncation(char32_t c) {
    return (0x0021 <= c && c <= 0x002f) || (0x003a <= c && c <= 0x0040) || (0x005b <= c && c <= 0x0060) || (0x007b <= c && c <= 0x007e);
}

inline bool isFullWidth(char32_t c) {
    // U+2e80~U+9fff U+a960~U+a97f U+ac00~U+d7ff U+f900~U+faff U+fe30~U+fe6f U+ff01~U+ff60 U+ffe0~U+ffe6 and U+20000~U+2fa1f have display width 2
    if (0x2e80 <= c && c <= 0x9fff) {
        return true;
    }
    if (0xa960 <= c && c <= 0xa97f) {
        return true;
    }
    if (0xac00 <= c && c <= 0xd7ff) {
        return true;
    }
    if (0xf900 <= c && c <= 0xfaff) {
        return true;
    }
    if (0xfe30 <= c && c <= 0xfe6f) {
        return true;
    }
    if (0xff01 <= c && c <= 0xff60) {
        return true;
    }
    if (0xffe0 <= c && c <= 0xffe6) {
        return true;
    }
    if (0x20000 <= c && c <= 0x2fa1f) {
        return true;
    }
    return false;
}

inline char32_t halfWidthToFullWidth(char32_t c) {
    if (c == '.') {
        return U'。';
    }
    if (c == '[') {
        return U'【';
    }
    if (c == ']') {
        return U'】';
    }
    if (0x0021 <= c && c <= 0x007e) {
        return c + 0xff00 - 0x0020;
    }
    return c;
}

inline char32_t fullWidthToHalfWidth(char32_t c) {
    if (c == U'。') {
        return '.';
    }
    if (c == U'【') {
        return '[';
    }
    if (c == U'】') {
        return ']';
    }
    if (0xff01 <= c && c <= 0xff5e) {
        return c - 0xff00 + 0x0020;
    }
    return c;
}

inline std::pair<int, int> chineseEnglishFraction(std::u32string_view str) {
    int chineseCount = 0;
    int englishCount = 0;
    for (char32_t c : str) {
        if (isChineseCharacter(c) || isChinesePunctuation(c)) {
            chineseCount++;
        } else if (isEnglishCharacter(c) || isEnglishPuncation(c)) {
            englishCount++;
        }
    }
    return std::make_pair(chineseCount, englishCount);
}

inline bool guessIsChineseSentence(std::u32string_view str) {
    auto [chineseCount, englishCount] = chineseEnglishFraction(str);
    return chineseCount * 4 > englishCount;
}

constexpr char32_t chineseDigitsTable[] = U"零一二三四五六七八九十";

inline int chineseToSingleDigit(char32_t c) {
    for (int i = 0; i < 10; i++) {
        if (c == chineseDigitsTable[i]) {
            return i;
        }
    }
    return -1;
}

inline char32_t singleDigitToChinese(int num) {
    if (0 <= num && num <= 10) {
        return chineseDigitsTable[num];
    }
    return 0;
}

}
