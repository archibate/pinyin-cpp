#pragma once

#include <vector>
#include <pinyincpp/small_map.hpp>

namespace pinyincpp {

template <class K, class V, class Ks = std::vector<K>, class Vs = std::vector<V>>
struct TrieMultimap {
private:
    struct Node {
        SmallMap<K, Node> children;
        Vs values;
    };

    Node root;

public:
    template <class Kss>
    void insert(Kss const &keys, V value) {
        Node *current = &root;
        for (K const &key : keys) {
            auto result = current->children.try_emplace(key);
            current = &result.first->second;
        }
        current->values.push_back(std::move(value));
    }

    template <class Kss, class Vit>
    void batchedInsert(Kss const &keys, Vit first, Vit last) {
        Node *current = &root;
        for (K const &key : keys) {
            auto result = current->children.try_emplace(key);
            current = &result.first->second;
        }
        current->values.insert(current->values.end(), first, last);
    }

    template <class Kss>
    bool erase(Kss const &keys) {
        Node *current = &root;
        Node *parent = nullptr;
        decltype(current->children.find(keys[0])) it;
        for (std::size_t i = 0; i < keys.size(); ++i) {
            K const &key = keys[i];
            it = current->children.find(key);
            if (it == current->children.end()) {
                return false;
            }
            parent = current;
            current = &it->second;
        }
        if (!parent) {
            return false;
        }
        parent->children.erase(it);
        return true;
    }

    template <class Kss>
    Vs find(Kss const &keys) const {
        Node const *current = &root;
        for (K const &key : keys) {
            auto it = current->children.find(key);
            if (it == current->children.end()) {
                return {};
            }
            current = &it->second;
        }
        if (!current->values) {
            return {};
        }
        return current->values;
    }

    template <class Kss, class Visit>
    bool visitPrefix(Kss const &keys, Visit &&visit, std::size_t depthLimit = (std::size_t)-1) const {
        Node const *current = &root;
        for (K const &key : keys) {
            auto it = current->children.find(key);
            if (it == current->children.end()) {
                return false;
            }
            current = &it->second;
        }
        return visitItems(*current, keys, visit, depthLimit);
    }

    template <class Kss = Ks, class Visit>
    bool visitItems(Visit &&visit, std::size_t depthLimit = (std::size_t)-1) const {
        return visitItems(root, Kss(), visit, depthLimit);
    }

    std::vector<std::pair<Ks, V>> getItems() const {
        std::vector<std::pair<Ks, V>> values;
        visitItems(root, Ks(), [&] (Ks const &keys, V const &value) {
            values.emplace_back(keys, value);
            return false;
        });
        return values;
    }

private:
    template <class Kss, class Visit>
    static bool visitItems(Node const &node, Kss const &keys, Visit &&visit, std::size_t depthLimit = (std::size_t)-1) {
        if (depthLimit == 0) {
            return false;
        }
        for (V const &v: node.values) {
            if (visit(keys, v)) {
                return true;
            }
        }
        if (depthLimit <= 1) {
            return false;
        }
        for (auto const &child : node.children) {
            Kss newKeys = keys;
            newKeys.push_back(child.first);
            if (visitItems(child.second, newKeys, visit, depthLimit - 1)) {
                return true;
            }
        }
        return false;
    }
};

}
