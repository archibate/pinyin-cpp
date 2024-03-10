#pragma once

#include <cstddef>
#include <initializer_list>
#include <concepts>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <variant>
#include <vector>
#include <unordered_map>

namespace pinyincpp {

#if 0
template <class K, class V>
struct SmallMap {
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    struct Store {
        union {
            T m_data[N];
        };
        std::size_t m_size = 0;

        Store() noexcept {}
    };

    std::variant<Store, std::vector<T>> m_variant;

    InlineVector() noexcept {
        auto &store = m_variant.template emplace<0>();
        store.m_size = 0;
    }

    InlineVector(std::initializer_list<T> l) {
        if (l.size() <= N) {
            auto &store = m_variant.template emplace<0>();
            store.m_size = 0;
            for (auto const& x : l) {
                new (store.m_data + store.m_size) T(x);
                ++store.m_size;
            }
        } else {
            m_variant.template emplace<1>(l);
        }
    }

    template <std::input_iterator It, std::sentinel_for<It> Ite>
    explicit InlineVector(It first, Ite last) {
        if (std::distance(first, last) <= N) {
            auto &store = m_variant.template emplace<0>();
            store.m_size = 0;
            for (; first != last; ++first) {
                new (store.m_data + store.m_size) T(*first);
                ++store.m_size;
            }
        } else {
            m_variant.template emplace<1>(first, last);
        }
    }

    InlineVector(InlineVector const &that) {
        std::visit([&that, this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                auto& this_store = m_variant.template emplace<0>();
                this_store.m_size = that.size();
                for (std::size_t i = 0; i < this_store.m_size; ++i) {
                    new (this_store.m_data + i) T(arg.m_data[i]);
                }
            } else {
                m_variant.template emplace<1>(arg);
            }
        }, that.m_variant);
    }

    InlineVector(InlineVector &&that) noexcept {
        std::visit([&that, this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                auto &this_store = m_variant.template emplace<0>();
                this_store.m_size = that.size();
                for (std::size_t i = 0; i < this_store.m_size; ++i) {
                    new (this_store.m_data + i) T(std::move(arg.m_data[i]));
                }
            } else {
                m_variant.template emplace<1>(std::move(arg));
            }
        }, std::move(that.m_variant));
    }

    InlineVector &operator=(InlineVector const &that) {
        this->~InlineVector();
        new (this) InlineVector(that);
        return *this;
    }

    InlineVector &operator=(InlineVector &&that) noexcept {
        this->~InlineVector();
        new (this) InlineVector(std::move(that));
        return *this;
    }

    void push_back(const T& x) {
        std::visit([&x, this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                if (arg.m_size + 1 >= N) {
                    std::vector<T> v;
                    v.reserve(arg.m_size + 1);
                    v.assign(arg.m_data, arg.m_data + arg.m_size);
                    for (std::size_t i = 0; i < N; ++i) {
                        arg.m_data[i].~T();
                    }
                    v.push_back(x);
                    m_variant.template emplace<1>(std::move(v));
                } else {
                    new (arg.m_data + arg.m_size) T(x);
                    ++arg.m_size;
                }
            } else {
                arg.push_back(x);
            }
        }, m_variant);
    }

    void push_back(T&& x) {
        std::visit([&x, this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                if (arg.m_size + 1 >= N) {
                    std::vector<T> v;
                    v.reserve(arg.m_size + 1);
                    v.assign(std::make_move_iterator(arg.m_data), std::make_move_iterator(arg.m_data + arg.m_size));
                    for (std::size_t i = 0; i < N; ++i) {
                        arg.m_data[i].~T();
                    }
                    v.push_back(std::move(x));
                    m_variant.template emplace<1>(std::move(v));
                } else {
                    new (arg.m_data + arg.m_size) T(std::move(x));
                    ++arg.m_size;
                }
            } else {
                arg.push_back(std::move(x));
            }
        }, m_variant);
    }

    void pop_back() noexcept {
        std::visit([this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                --arg.m_size;
                arg.m_data[arg.m_size].~T();
            } else {
                arg.pop_back();
            }
        }, m_variant);
    }

    T *begin() noexcept {
        return std::visit([](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                return arg.m_data;
            } else {
                return &*arg.begin();
            }
        }, m_variant);
    }

    T *end() noexcept {
        return std::visit([](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                return arg.m_data + arg.m_size;
            } else {
                return &*arg.end();
            }
        }, m_variant);
    }

    T const *begin() const noexcept {
        return std::visit([](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                return arg.m_data;
            } else {
                return &*arg.begin();
            }
        }, m_variant);
    }

    T const *end() const noexcept {
        return std::visit([](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                return arg.m_data + arg.m_size;
            } else {
                return &*arg.end();
            }
        }, m_variant);
    }

    T const *cbegin() const noexcept { return begin(); }
    T const *cend() const noexcept { return end(); }

    T &front() noexcept { return *begin(); }
    T const &front() const noexcept { return *cbegin(); }

    T &back() noexcept { return *(end() - 1); }
    T const &back() const noexcept { return *(cend() - 1); }

    T *data() noexcept { return begin(); }
    T const *data() const noexcept { return cbegin(); }
    T const *cdata() const noexcept { return cbegin(); }

    T &operator[](std::size_t index) noexcept { return data()[index]; }
    T const &operator[](std::size_t index) const noexcept { return cdata()[index]; }

    T &at(std::size_t index) {
        if (index >= size()) [[unlikely]] throw std::out_of_range("InlineVector::at");
        return data()[index];
    }

    T const &at(std::size_t index) const {
        if (index >= size()) [[unlikely]] throw std::out_of_range("InlineVector::at");
        return cdata()[index];
    }

    void clear() noexcept {
        std::visit([this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                for (std::size_t i = 0; i < arg.m_size; ++i) {
                    arg.m_data[i].~T();
                }
                arg.m_size = 0;
            } else {
                arg.clear();
            }
        }, m_variant);
    }

    ~InlineVector() noexcept {
        clear();
    }

    [[nodiscard]] bool empty() const noexcept { return size() == 0; }
    [[nodiscard]] std::size_t size() const noexcept {
        return std::visit([](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                return arg.m_size;
            } else {
                return arg.size();
            }
        }, m_variant);
    }

    void resize(std::size_t new_size) {
        std::visit([new_size, this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                for (std::size_t i = new_size; i < arg.m_size; ++i) {
                    arg.m_data[i].~T();
                }
                for (std::size_t i = arg.m_size; i < new_size; ++i) {
                    new (arg.m_data + i) T();
                }
                arg.m_size = new_size;
            } else {
                arg.resize(new_size);
            }
        }, m_variant);
    }

    void resize(std::size_t new_size, T const &value) {
        std::visit([new_size, &value](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                for (std::size_t i = 0; i < new_size && i < arg.m_size; ++i) {
                    arg.m_data[i].~T();
                }
                for (std::size_t i = arg.m_size; i < new_size; ++i) {
                    new (arg.m_data + i) T(value);
                }
                arg.m_size = new_size;
            } else {
                arg.resize(new_size, value);
            }
        }, m_variant);
    }

    void reserve(std::size_t new_size) {
        std::visit([new_size, this](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                if (new_size > N) {
                    auto &v = m_variant.template emplace<1>();
                    v.reserve(new_size);
                }
            } else {
                arg.reserve(new_size);
            }
        }, m_variant);
    }

    template <std::input_iterator It, std::sentinel_for<It> Ite>
    void insert(const_iterator pos, It first, Ite last) {
        std::visit([&pos, this, &first, &last](auto&& arg){
            using S = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<S, Store>) {
                auto n = pos - cbegin();
                for (; first != last; ++first) {
                    push_back(*first);
                }
                for (std::size_t i = n; i < size(); ++i) {
                    std::swap(begin()[i], back());
                }
            } else {
                arg.insert(arg.begin() + (pos - cbegin()), first, last);
            }
        }, m_variant);
    }
};
#else
template <class K, class V>
using SmallMap = std::unordered_map<K, V>;
#endif

}
