#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <bit>
#include <pinyincpp/debug.hpp>

namespace pinyincpp {

struct BytesReader {
    const unsigned char *begin, *end;

    BytesReader(std::string_view view) : begin((const unsigned char *)view.data()), end((const unsigned char *)view.data() + view.size()) {
    }

    template <class T, std::endian endian = std::endian::little>
    T read() {
        static_assert(std::is_arithmetic_v<T>);
        debug().check(end - begin) >= sizeof(T);
        T result;
        unsigned char *buffer = reinterpret_cast<unsigned char *>(std::addressof(result));
        std::memcpy(buffer, begin, sizeof(T));
        if constexpr (endian != std::endian::native && sizeof(T) > 1) {
            std::reverse(buffer, buffer + sizeof(T));
        }
        begin += sizeof(T);
        return result;
    }

    template <std::endian endian = std::endian::little>
    std::uint64_t read64() {
        return read<std::uint64_t, endian>();
    }

    template <std::endian endian = std::endian::little>
    std::uint32_t read32() {
        return read<std::uint32_t, endian>();
    }

    template <std::endian endian = std::endian::little>
    std::uint32_t read24() {
        debug().check(end - begin) >= 3;
        std::uint32_t r = read8();
        std::uint32_t g = read8();
        std::uint32_t b = read8();
        if constexpr (endian == std::endian::little) {
            return (b << 16) | (g << 8) | r;
        } else {
            return (r << 16) | (g << 8) | b;
        }
    }

    template <std::endian endian = std::endian::little>
    std::uint16_t read16() {
        return read<std::uint16_t, endian>();
    }

    template <std::endian endian = std::endian::little>
    std::uint8_t read8() {
        debug().check(end - begin) >= 1;
        return *begin++;
    }

    std::string reads(std::size_t n) {
        debug().check(end - begin) >= n;
        std::string result((const char *)begin, n);
        begin += n;
        return result;
    }
};

}
