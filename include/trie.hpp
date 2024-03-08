#pragma once

#include <map>
#include <vector>

namespace pinyincpp {

template <class K, class V, class Ks = std::vector<K>, class Vs = std::vector<V>>
struct Trie {
    struct Node {
        std::map<K, Node> children;
        Vs values;
    };

    Node root;

public:
    void insert(Ks const &keys, V value) {
        Node *current = &root;
        for (K const &key : keys) {
            auto result = current->children.emplace(std::piecewise_construct, std::make_tuple(key), std::make_tuple());
            current = &result.first->second;
        }
        current->values.push_back(std::move(value));
    }

    void batchedInsert(Ks const &keys, Vs values) {
        Node *current = &root;
        for (K const &key : keys) {
            auto result = current->children.emplace(std::piecewise_construct, std::make_tuple(key), std::make_tuple());
            current = &result.first->second;
        }
        if (current->values.empty()) {
            current->values = std::move(values);
        } else {
            current->values.insert(current->values.end(), values.begin(), values.end());
        }
    }

    bool erase(Ks const &keys) {
        Node *current = &root;
        Node *parent = nullptr;
        typename std::unordered_map<K, Node>::iterator it;
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

    Vs find(Ks const &keys) const {
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

    template <class Visit>
    bool findPrefix(Ks const &keys, Visit &&visit, std::size_t depthLimit = (std::size_t)-1) const {
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

    std::vector<std::pair<Ks, V>> getItems() const {
        std::vector<std::pair<Ks, V>> values;
        visitItems(root, Ks(), [&] (Ks const &keys, V const &value) {
            values.emplace_back(keys, value);
            return false;
        });
        return values;
    }

private:
    template <class Visit>
    static bool visitItems(Node const &node, Ks const &keys, Visit &&visit, std::size_t depthLimit = (std::size_t)-1) {
        if (depthLimit == 0) {
            return false;
        }
        for (V const &v: node.values) {
            if (visit(keys, v)) {
                return true;
            }
        }
        for (auto const &child : node.children) {
            Ks newKeys = keys;
            newKeys.push_back(child.first);
            if (visitItems(child.second, newKeys, visit, depthLimit - 1)) {
                return true;
            }
        }
        return false;
    }
};

}
