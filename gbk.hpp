#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "utf8.hpp"

namespace pinyincpp {

extern const char gbk_bin[];
extern const unsigned long long gbk_bin_size;
extern const char gbk_inv_bin[];
extern const unsigned long long gbk_inv_bin_size;

inline char16_t gbkToUnicode(std::uint16_t gbk) {
    std::uint16_t hi = static_cast<std::uint8_t>(gbk >> 8);
    std::uint16_t lo = static_cast<std::uint8_t>(gbk & 0xFF);

    if (0x81 <= hi && hi <= 0xFE && 0x40 <= lo && lo <= 0xFE) {
        auto base = reinterpret_cast<const char16_t *>(gbk_bin);
        auto index = (hi - 0x81) * (0xFE - 0x40) + lo - 0x40;
        return base[index];
    } else {
        return 0xFFFD;
    }
}

inline std::uint16_t unicodeToGbk(char16_t code) {
    auto base = reinterpret_cast<const char16_t *>(gbk_inv_bin);
    auto index = static_cast<std::uint16_t>(code);
    return base[index];
}

template <typename Wide = char16_t>
inline std::basic_string<Wide> gbkToUnicode(std::string_view str) {
    std::basic_string<Wide> result;
    int phase = 0;
    std::uint8_t hi;
    for (std::size_t i = 0; i < str.size();) {
        std::uint8_t ch = static_cast<std::uint8_t>(str[i]);
        if (0x81 <= ch && ch <= 0xFE) {
            phase = 1;
            hi = ch;
            i++;
        } else {
            if (phase == 1) {
                auto lo = ch;
                char32_t code = 0xFFFD;
                if (0x40 <= lo && lo <= 0xFE) {
                    auto gbk = (hi << 8) | lo;
                    code = gbkToUnicode(gbk);
                }
                result.push_back(code);
            } else {
                result.push_back(ch);
            }
            phase = 0;
            i++;
        }
    }
    if (phase == 1) {
        result.push_back(0xFFFD);
    }
    return result;
}

template <typename Wide = char16_t>
inline std::string unicodeToGbk(std::basic_string_view<Wide> str) {
    std::string result;
    for (std::size_t i = 0; i < str.size(); i++) {
        auto gbk = unicodeToGbk(str[i]);
        result.push_back(static_cast<std::uint8_t>(gbk & 0xFF));
        if (gbk > 0xFF) {
            result.push_back(static_cast<std::uint8_t>(gbk >> 8));
        }
    }
    return result;
}

inline std::u8string gbkToUtf8(std::string_view str) {
    return utf32to8(gbkToUnicode<char32_t>(str));
}

inline std::u16string gbkToUtf16(std::string_view str) {
    return gbkToUnicode<char16_t>(str);
}

inline std::u32string gbkToUtf32(std::string_view str) {
    return gbkToUnicode<char32_t>(str);
}

inline std::string utf8ToGbk(std::u8string_view str) {
    return unicodeToGbk<char32_t>(utf8to32(str));
}

inline std::string utf16ToGbk(std::u16string_view str) {
    return unicodeToGbk<char16_t>(str);
}

inline std::string utf32ToGbk(std::u32string_view str) {
    return unicodeToGbk<char32_t>(str);
}

}
