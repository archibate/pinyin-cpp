#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <variant>
#include <vector>
#include <unordered_map>

namespace pinyincpp {

#if 1
template <class K, class V, class Cmp = std::less<K>, class BaseVector = std::vector<std::pair<K, V>>>
struct SmallMap {
    using key_type = K;
    using mapped_type = V;
    using key_compare = Cmp;
    using base_vector_type = BaseVector;
    using value_type = std::pair<K, V>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = typename base_vector_type::iterator;
    using const_iterator = typename base_vector_type::const_iterator;

private:
    BaseVector m_store;

    auto value_key_comp() const {
        return [this](const value_type& lhs, const key_type& rhs) {
            return Cmp{}(lhs.first, rhs);
        };
    }

public:
    SmallMap() noexcept {}

    template <class ...Args>
    std::pair<iterator, bool> try_emplace(K const &key, Args &&...args) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        if (it != m_store.end() && it->first == key) {
            return {it, false};
        } else {
            return {m_store.emplace(it, key, V(std::forward<Args>(args)...)), true};
        }
    }

    std::pair<iterator, bool> insert(const value_type &value) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), value.first, value_key_comp());
        if (it != m_store.end() && it->first == value.first) {
            return {it, false};
        } else {
            return {m_store.insert(it, value), true};
        }
    }

    std::pair<iterator, bool> insert(value_type &&value) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), value.first, value_key_comp());
        if (it != m_store.end() && it->first == value.first) {
            return {it, false};
        } else {
            return {m_store.insert(it, std::move(value)), true};
        }
    }

    template <class InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    void insert(std::initializer_list<value_type> ilist) {
        insert(ilist.begin(), ilist.end());
    }

    iterator find(const key_type &key) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        if (it != m_store.end() && it->first == key) {
            return it;
        } else {
            return end();
        }
    }

    const_iterator find(const key_type &key) const {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        if (it != m_store.end() && it->first == key) {
            return it;
        } else {
            return cend();
        }
    }

    iterator begin() noexcept {
        return m_store.begin();
    }

    const_iterator begin() const noexcept {
        return m_store.begin();
    }

    const_iterator cbegin() const noexcept {
        return m_store.cbegin();
    }

    iterator end() noexcept {
        return m_store.end();
    }

    const_iterator end() const noexcept {
        return m_store.end();
    }

    const_iterator cend() const noexcept {
        return m_store.cend();
    }

    bool empty() const noexcept {
        return m_store.empty();
    }

    size_type size() const noexcept {
        return m_store.size();
    }

    size_type max_size() const noexcept {
        return m_store.max_size();
    }

    void clear() noexcept {
        m_store.clear();
    }

    iterator erase(const_iterator pos) {
        return m_store.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last) {
        return m_store.erase(first, last);
    }

    size_type erase(const key_type &key) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        if (it != m_store.end() && it->first == key) {
            m_store.erase(it);
            return 1;
        } else {
            return 0;
        }
    }

    reference operator[](const key_type &key) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        if (it != m_store.end() && it->first == key) {
            return it->second;
        } else {
            it = m_store.emplace(it, key, V());
            return it->second;
        }
    }

    std::pair<iterator, bool> insert_or_assign(const key_type &key, const mapped_type &value) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        if (it != m_store.end() && it->first == key) {
            it->second = value;
            return {it, false};
        } else {
            return {m_store.emplace(it, key, value), true};
        }
    }

    std::pair<iterator, bool> insert_or_assign(key_type &&key, mapped_type &&value) {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        if (it != m_store.end() && it->first == key) {
            it->second = std::move(value);
            return {it, false};
        } else {
            return {m_store.emplace(it, std::move(key), std::move(value)), true};
        }
    }

    bool contains(const key_type &key) const {
        auto it = std::lower_bound(m_store.begin(), m_store.end(), key, value_key_comp());
        return (it != m_store.end() && it->first == key);
    }

    friend bool operator==(const SmallMap &lhs, const SmallMap &rhs) {
        return lhs.m_store == rhs.m_store;
    }

    friend bool operator!=(const SmallMap &lhs, const SmallMap &rhs) {
        return !(lhs == rhs);
    }

    auto key_comp() const {
        return Cmp{};
    }
};
#else
template <class K, class V>
using SmallMap = std::unordered_map<K, V>;
#endif

}
