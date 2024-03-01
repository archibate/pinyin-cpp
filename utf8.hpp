#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace pinyincpp {

inline std::u8string utf32to8(char32_t c) {
    std::u8string result;
    if (c < 0x80) {
        result.resize(1);
        result[0] = (char8_t)c;
    } else if (c < 0x800) {
        result.resize(2);
        result[0] = (char8_t)(0xC0 | (c >> 6));
        result[1] = (char8_t)(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        result.resize(3);
        result[0] = (char8_t)(0xE0 | (c >> 12));
        result[1] = (char8_t)(0x80 | ((c >> 6) & 0x3F));
        result[2] = (char8_t)(0x80 | (c & 0x3F));
    } else if (c < 0x200000) {
        result.resize(4);
        result[0] = (char8_t)(0xF0 | (c >> 18));
        result[1] = (char8_t)(0x80 | ((c >> 12) & 0x3F));
        result[2] = (char8_t)(0x80 | ((c >> 6) & 0x3F));
        result[3] = (char8_t)(0x80 | (c & 0x3F));
    } else {
        result.resize(1);
        result[0] = '?';
    }
    return result;
}

inline std::u32string utf8to32(std::u8string_view s) {
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

inline std::u8string utf32to8(std::u32string_view s) {
    std::u8string result;
    result.reserve(s.size());
    for (char32_t c : s) {
        if (c < 0x80) {
            result.push_back(c);
        } else if (c < 0x800) {
            result.push_back((char8_t)(0xC0 | (c >> 6)));
            result.push_back((char8_t)(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            result.push_back((char8_t)(0xE0 | (c >> 12)));
            result.push_back((char8_t)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char8_t)(0x80 | (c & 0x3F)));
        } else if (c < 0x200000) {
            result.push_back((char8_t)(0xF0 | (c >> 18)));
            result.push_back((char8_t)(0x80 | ((c >> 12) & 0x3F)));
            result.push_back((char8_t)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char8_t)(0x80 | (c & 0x3F)));
        }
    }
    return result;
}

inline std::u8string utf16to8(std::u16string_view s) {
    std::u8string result;
    result.reserve(s.size());
    int phase = 0;
    char16_t surrogate;
    for (char16_t c : s) {
        if (c < 0x80) {
            result.push_back((char8_t)c);
        } else if (c < 0x800) {
            result.push_back((char8_t)(0xC0 | (c >> 6)));
            result.push_back((char8_t)(0x80 | (c & 0x3F)));
        } else if (c < 0xD800 || c > 0xDFFF) {
            result.push_back((char8_t)(0xE0 | (c >> 12)));
            result.push_back((char8_t)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char8_t)(0x80 | (c & 0x3F)));
        } else {
            if (phase == 0) {
                phase = 1;
                surrogate = c;
            } else {
                phase = 0;
                std::uint32_t codepoint = 0x10000 + (((std::uint32_t)surrogate - 0xD800) << 10) | (((std::uint32_t)c - 0xDC00) & 0x3FF);
                result.push_back((char8_t)(0xF0 | (codepoint >> 18)));
                result.push_back((char8_t)(0x80 | ((codepoint >> 12) & 0x3F)));
                result.push_back((char8_t)(0x80 | ((codepoint >> 6) & 0x3F)));
                result.push_back((char8_t)(0x80 | (codepoint & 0x3F)));
            }
        }
    }
    return result;
}

inline std::u16string utf8to16(std::u8string_view s) {
    std::u16string result;
    result.reserve(s.size());
    std::uint32_t state = 0;
    std::uint32_t codepoint = 0;
    for (auto c : s) {
        std::uint8_t byte = (std::uint8_t)c;
        if (state == 0) {
            if (byte < 0x80) {
                result.push_back(byte);
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
                if (codepoint >= 0x10000) {
                    codepoint -= 0x10000;
                    char16_t high = (char16_t)(0xD800 | ((codepoint >> 10) & 0x3FF));
                    char16_t low = (char16_t)(0xDC00 | (codepoint & 0x3FF));
                    result.push_back(high);
                    result.push_back(low);
                } else {
                    result.push_back((char16_t)codepoint);
                }
            }
        }
    }
    return result;
}

inline std::u32string utf16to32(std::u16string_view s) {
    std::u32string result;
    result.reserve(s.size());
    int phase = 0;
    char16_t surrogate;
    for (char16_t c : s) {
        if (c < 0xD800 || c > 0xDFFF) {
            result.push_back(c);
            phase = 0;
        } else {
            if (phase == 0) {
                phase = 1;
                surrogate = c;
            } else {
                phase = 0;
                result.push_back(0x10000 + (((std::uint32_t)surrogate - 0xD800) << 10) | (((std::uint32_t)c - 0xDC00) & 0x3FF));
            }
        }
    }
    return result;
}

inline std::u16string utf32to16(std::u32string_view s) {
    std::u16string result;
    result.reserve(s.size());
    for (char32_t c : s) {
        if (c < 0x10000) {
            result.push_back((char16_t)c);
        } else {
            c -= 0x10000;
            char16_t high = (char16_t)(0xD800 | ((c >> 10) & 0x3FF));
            char16_t low = (char16_t)(0xDC00 | (c & 0x3FF));
            result.push_back(high);
            result.push_back(low);
        }
    }
    return result;
}

inline std::u8string utfCto8(std::string_view s) {
    return std::u8string((const char8_t *)s.data(), s.size());
}

inline std::string utf8toC(std::u8string_view s) {
    return std::string((const char *)s.data(), s.size());
}

inline std::u32string utfCto32(std::string_view s) {
    return utf8to32(utfCto8(s));
}

inline std::u16string utf8to16(std::string_view s) {
    return utf8to16(utfCto8(s));
}

inline std::string utf32toC(char32_t c) {
    return utf8toC(utf32to8(c));
}

inline std::string utf32toC(std::u32string_view s) {
    return utf8toC(utf32to8(s));
}

inline std::string utf16toC(std::u16string_view s) {
    return utf8toC(utf16to8(s));
}

inline std::wstring utf8toW(std::u8string_view s) {
    if constexpr (sizeof(wchar_t) == 2) {
    auto u16 = utf8to16(s);
    return std::wstring((const wchar_t *)u16.data(), u16.size());
    } else if constexpr (sizeof(wchar_t) == 4) {
        auto u32 = utf8to32(s);
        return std::wstring((const wchar_t *)u32.data(), u32.size());
    } else {
        return std::wstring(s.begin(), s.end());
    }
}

inline std::u8string utfWto8(std::wstring_view s) {
    if constexpr (sizeof(wchar_t) == 2) {
        return utf16to8(std::u16string_view((const char16_t *)s.data(), s.size()));
    } else if constexpr (sizeof(wchar_t) == 4) {
        return utf32to8(std::u32string_view((const char32_t *)s.data(), s.size()));
    } else {
        return std::u8string(s.begin(), s.end());
    }
}

inline std::wstring utf32toW(std::u32string_view s) {
    if constexpr (sizeof(wchar_t) == 2) {
        auto u16 = utf32to16(s);
        return std::wstring((const wchar_t *)u16.data(), u16.size());
    } else if constexpr (sizeof(wchar_t) == 4) {
        return std::wstring((const wchar_t *)s.data(), s.size());
    } else {
        return std::wstring(s.begin(), s.end());
    }
}

inline std::u32string utfWto32(std::wstring_view s) {
    if constexpr (sizeof(wchar_t) == 2) {
        return utf16to32(std::u16string_view((const char16_t *)s.data(), s.size()));
    } else if constexpr (sizeof(wchar_t) == 4) {
        return std::u32string((const char32_t *)s.data(), s.size());
    } else {
        return std::u32string(s.begin(), s.end());
    }
}

inline std::wstring utf16toW(std::u16string_view s) {
    if constexpr (sizeof(wchar_t) == 2) {
        return std::wstring((const wchar_t *)s.data(), s.size());
    } else if constexpr (sizeof(wchar_t) == 4) {
        auto u32 = utf16to32(s);
        return std::wstring((const wchar_t *)u32.data(), u32.size());
    } else {
        return std::wstring(s.begin(), s.end());
    }
}

inline std::u16string utfWto16(std::wstring_view s) {
    if constexpr (sizeof(wchar_t) == 2) {
        return std::u16string((const char16_t *)s.data(), s.size());
    } else if constexpr (sizeof(wchar_t) == 4) {
        return utf32to16(std::u32string_view((const char32_t *)s.data(), s.size()));
    } else {
        return std::u16string(s.begin(), s.end());
    }
}

inline std::wstring utfCtoW(std::string_view s) {
    return utf8toW(utfCto8(s));
}

inline std::string utfWtoC(std::wstring_view s) {
    return utf8toC(utfWto8(s));
}

}
