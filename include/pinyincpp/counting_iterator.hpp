#pragma once

#include <iterator>
#include <memory>

namespace pinyincpp {

template <class T>
struct CountingIterator {
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T const &;
    using iterator_category = std::random_access_iterator_tag;

    constexpr CountingIterator() noexcept : value{} {}
    constexpr CountingIterator(T value) noexcept : value{value} {}
    constexpr reference operator*() const { return value; }
    constexpr CountingIterator &operator++() { ++value; return *this; }
    constexpr CountingIterator operator++(int) { return {value++}; }
    constexpr bool operator==(CountingIterator const &other) const { return value == other.value; }
    constexpr bool operator!=(CountingIterator const &other) const { return value != other.value; }
    constexpr CountingIterator &operator+=(difference_type n) { value += n; return *this; }
    constexpr CountingIterator &operator-=(difference_type n) { value -= n; return *this; }
    constexpr CountingIterator operator+(difference_type n) const { return {value + n}; }
    friend constexpr CountingIterator operator+(difference_type n, CountingIterator it) { return {it.value + n}; }
    constexpr CountingIterator operator-(difference_type n) const { return {value - n}; }
    constexpr difference_type operator-(CountingIterator const &other) const { return value - other.value; }
    constexpr pointer operator->() const { return std::addressof(value); }
    constexpr reference operator[](difference_type n) const { return value + n; }
    constexpr CountingIterator &operator--() { --value; return *this; }
    constexpr CountingIterator operator--(int) { return {value--}; }
    constexpr bool operator<(CountingIterator const &other) const { return value < other.value; }
    constexpr bool operator>(CountingIterator const &other) const { return value > other.value; }
    constexpr bool operator<=(CountingIterator const &other) const { return value <= other.value; }
    constexpr bool operator>=(CountingIterator const &other) const { return value >= other.value; }

    T value;
};

template <class T>
CountingIterator(T) -> CountingIterator<T>;

}
